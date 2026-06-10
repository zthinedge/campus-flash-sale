#include "app/WebApp.h"

#include "db/MySqlError.h"
#include "http/HttpServer.h"
#include "model/Enums.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "service/ActivityService.h"
#include "service/AdminService.h"
#include "service/AuthService.h"
#include "service/FlashSaleService.h"
#include "service/OrderService.h"
#include "service/ProductService.h"
#include "service/ServiceError.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace campus::app
{

namespace
{

using Json = nlohmann::json;
using HttpRequest = reactor_http_kit::http::HttpRequest;
using HttpResponse = reactor_http_kit::http::HttpResponse;

class ApiError : public std::runtime_error
{
public:
    ApiError(int status, std::string message)
        : std::runtime_error(std::move(message)),
          status_(status)
    {
    }

    int status() const noexcept
    {
        return status_;
    }

private:
    int status_;
};

Json parseBody(const HttpRequest& request)
{
    try
    {
        const Json body = Json::parse(request.body);
        if (!body.is_object())
        {
            throw ApiError(400, "请求体必须是 JSON 对象");
        }
        return body;
    }
    catch (const Json::parse_error&)
    {
        throw ApiError(400, "请求体不是合法 JSON");
    }
}

template <typename Value>
Value required(const Json& body, std::string_view key)
{
    const auto iterator = body.find(std::string(key));
    if (iterator == body.end())
    {
        throw ApiError(400, "缺少字段: " + std::string(key));
    }
    try
    {
        return iterator->get<Value>();
    }
    catch (const Json::exception&)
    {
        throw ApiError(400, "字段类型错误: " + std::string(key));
    }
}

std::uint64_t parseId(const std::string& value, std::string_view field)
{
    try
    {
        std::size_t consumed = 0;
        const std::uint64_t id = std::stoull(value, &consumed);
        if (id == 0 || consumed != value.size())
        {
            throw std::invalid_argument("invalid id");
        }
        return id;
    }
    catch (const std::exception&)
    {
        throw ApiError(400, std::string(field) + " 必须是正整数");
    }
}

std::string bearerToken(const HttpRequest& request)
{
    constexpr std::string_view prefix = "Bearer ";
    const std::string authorization = request.header("Authorization");
    if (!authorization.starts_with(prefix))
    {
        return {};
    }
    return authorization.substr(prefix.size());
}

std::string normalizeDateTime(std::string value)
{
    if (value.size() > 10 && value[10] == 'T')
    {
        value[10] = ' ';
    }
    if (value.size() == 16)
    {
        value += ":00.000";
    }
    else if (value.size() == 19)
    {
        value += ".000";
    }
    return value;
}

Json userJson(const model::User& user)
{
    return {
        { "id", user.id },
        { "username", user.username },
        { "role", std::string(model::toString(user.role)) },
        { "status", std::string(model::toString(user.status)) }
    };
}

Json productJson(const model::Product& product)
{
    return {
        { "id", product.id },
        { "productNo", product.productNo },
        { "sellerId", product.sellerId },
        { "title", product.title },
        { "description", product.description },
        { "originalPriceCents", product.originalPriceCents },
        { "status", std::string(model::toString(product.status)) },
        { "createdAt", product.createdAt }
    };
}

Json activityJson(const model::FlashSaleActivity& activity,
                  const std::optional<model::Product>& product = std::nullopt)
{
    Json json {
        { "id", activity.id },
        { "activityNo", activity.activityNo },
        { "productId", activity.productId },
        { "flashPriceCents", activity.flashPriceCents },
        { "totalStock", activity.totalStock },
        { "availableStock", activity.availableStock },
        { "startTime", activity.startTime },
        { "endTime", activity.endTime },
        { "status", std::string(model::toString(activity.status)) },
        { "createdAt", activity.createdAt }
    };
    if (product.has_value())
    {
        json["product"] = productJson(*product);
    }
    return json;
}

Json orderJson(const model::Order& order)
{
    return {
        { "id", order.id },
        { "orderNo", order.orderNo },
        { "userId", order.userId },
        { "activityId", order.activityId },
        { "productId", order.productId },
        { "productTitle", order.productTitleSnapshot },
        { "purchasePriceCents", order.purchasePriceCents },
        { "quantity", order.quantity },
        { "totalAmountCents", order.totalAmountCents },
        { "status", std::string(model::toString(order.status)) },
        { "createdAt", order.createdAt }
    };
}

HttpResponse jsonResponse(int status, Json body)
{
    body["ok"] = status >= 200 && status < 300;
    return HttpResponse::json(status, body.dump());
}

int serviceStatus(service::ServiceErrorCode code)
{
    switch (code)
    {
    case service::ServiceErrorCode::InvalidArgument:
        return 400;
    case service::ServiceErrorCode::InvalidCredentials:
    case service::ServiceErrorCode::UserDisabled:
        return 401;
    case service::ServiceErrorCode::Forbidden:
        return 403;
    case service::ServiceErrorCode::UserNotFound:
    case service::ServiceErrorCode::ProductNotFound:
    case service::ServiceErrorCode::ActivityNotFound:
        return 404;
    case service::ServiceErrorCode::UsernameTaken:
    case service::ServiceErrorCode::ActivityUnavailable:
    case service::ServiceErrorCode::SoldOut:
    case service::ServiceErrorCode::DuplicateOrder:
        return 409;
    }
    return 400;
}

template <typename Action>
HttpResponse handleApi(Action action)
{
    try
    {
        return action();
    }
    catch (const ApiError& error)
    {
        return jsonResponse(error.status(), { { "error", error.what() } });
    }
    catch (const service::ServiceError& error)
    {
        return jsonResponse(serviceStatus(error.code()), { { "error", error.what() } });
    }
    catch (const db::MySqlError& error)
    {
        return jsonResponse(error.code() == 1062 ? 409 : 500, { { "error", error.what() } });
    }
    catch (const std::exception& error)
    {
        return jsonResponse(500, { { "error", error.what() } });
    }
}

} // namespace

WebApp::WebApp(db::MySqlConnection& connection,
               utils::Config config,
               std::filesystem::path staticRoot)
    : connection_(connection),
      config_(std::move(config)),
      staticRoot_(std::move(staticRoot))
{
}

int WebApp::run()
{
    reactor_http_kit::net::EventLoop loop;
    reactor_http_kit::http::HttpServer server(
        &loop,
        reactor_http_kit::net::InetAddress(config_.serverHost, config_.serverPort),
        "campus-flash-sale");

    server.addRoute("GET", "/api/health", [this](const HttpRequest& request) {
        return health(request);
    });
    server.addRoute("POST", "/api/register", [this](const HttpRequest& request) {
        return registerUser(request);
    });
    server.addRoute("POST", "/api/login", [this](const HttpRequest& request) {
        return login(request);
    });
    server.addRoute("POST", "/api/logout", [this](const HttpRequest& request) {
        return logout(request);
    });
    server.addRoute("GET", "/api/me", [this](const HttpRequest& request) {
        return me(request);
    });
    server.addRoute("GET", "/api/products", [this](const HttpRequest& request) {
        return listProducts(request);
    });
    server.addRoute("GET", "/api/products/{id}", [this](const HttpRequest& request) {
        return getProduct(request);
    });
    server.addRoute("POST", "/api/products", [this](const HttpRequest& request) {
        return publishProduct(request);
    });
    server.addRoute("GET", "/api/activities", [this](const HttpRequest& request) {
        return listActivities(request);
    });
    server.addRoute("POST", "/api/activities", [this](const HttpRequest& request) {
        return createActivity(request);
    });
    server.addRoute("POST", "/api/activities/{id}/purchase", [this](const HttpRequest& request) {
        return purchase(request);
    });
    server.addRoute("GET", "/api/orders", [this](const HttpRequest& request) {
        return listOrders(request);
    });
    server.addRoute("GET", "/api/admin/summary", [this](const HttpRequest& request) {
        return adminSummary(request);
    });
    server.setStaticRoot(staticRoot_.string());

    server.start();
    std::cout << "[INFO] web interface: http://127.0.0.1:" << config_.serverPort << '\n'
              << "[INFO] administrator: admin / admin123" << '\n'
              << "[INFO] student: student01 / 123456" << '\n';
    loop.loop();
    return 0;
}

WebApp::HttpResponse WebApp::health(const HttpRequest&)
{
    return handleApi([this] {
        connection_.ping();
        return jsonResponse(200, {
            { "service", "campus-flash-sale" },
            { "database", "mysql" },
            { "status", "running" }
        });
    });
}

WebApp::HttpResponse WebApp::registerUser(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const Json body = parseBody(request);
        const std::string username = required<std::string>(body, "username");
        const std::string password = required<std::string>(body, "password");
        service::AuthService authService(connection_);
        const auto userId = authService.registerUser(username, password);
        const auto user = authService.login(username, password);
        return jsonResponse(201, {
            { "userId", userId },
            { "user", userJson(user) },
            { "token", createSession(user) }
        });
    });
}

WebApp::HttpResponse WebApp::login(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const Json body = parseBody(request);
        const auto user = service::AuthService(connection_).login(
            required<std::string>(body, "username"),
            required<std::string>(body, "password"));
        return jsonResponse(200, {
            { "user", userJson(user) },
            { "token", createSession(user) }
        });
    });
}

WebApp::HttpResponse WebApp::logout(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const std::string token = bearerToken(request);
        if (!token.empty())
        {
            std::lock_guard lock(sessionsMutex_);
            sessions_.erase(token);
        }
        return jsonResponse(200, { { "message", "已退出登录" } });
    });
}

WebApp::HttpResponse WebApp::me(const HttpRequest& request)
{
    return handleApi([this, &request] {
        return jsonResponse(200, { { "user", userJson(requireUser(request)) } });
    });
}

WebApp::HttpResponse WebApp::listProducts(const HttpRequest&)
{
    return handleApi([this] {
        Json products = Json::array();
        for (const auto& product : service::ProductService(connection_).listVisible())
        {
            products.push_back(productJson(product));
        }
        return jsonResponse(200, { { "products", std::move(products) } });
    });
}

WebApp::HttpResponse WebApp::getProduct(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto id = parseId(request.pathParams.at("id"), "商品 ID");
        const auto product = service::ProductService(connection_).getById(id);
        return jsonResponse(200, { { "product", productJson(product) } });
    });
}

WebApp::HttpResponse WebApp::publishProduct(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto user = requireUser(request);
        const Json body = parseBody(request);
        const auto product = service::ProductService(connection_).publish(
            user.id,
            service::PublishProductRequest {
                required<std::string>(body, "productNo"),
                required<std::string>(body, "title"),
                body.value("description", ""),
                required<std::uint32_t>(body, "originalPriceCents")
            });
        return jsonResponse(201, { { "product", productJson(product) } });
    });
}

WebApp::HttpResponse WebApp::listActivities(const HttpRequest&)
{
    return handleApi([this] {
        service::ProductService productService(connection_);
        Json activities = Json::array();
        for (const auto& activity : service::ActivityService(connection_).listActive())
        {
            activities.push_back(activityJson(
                activity, productService.getById(activity.productId)));
        }
        return jsonResponse(200, { { "activities", std::move(activities) } });
    });
}

WebApp::HttpResponse WebApp::createActivity(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto admin = requireAdmin(request);
        const Json body = parseBody(request);
        const auto activity = service::ActivityService(connection_).create(
            admin.id,
            service::CreateActivityRequest {
                required<std::string>(body, "activityNo"),
                required<std::uint64_t>(body, "productId"),
                required<std::uint32_t>(body, "flashPriceCents"),
                required<std::uint32_t>(body, "totalStock"),
                normalizeDateTime(required<std::string>(body, "startTime")),
                normalizeDateTime(required<std::string>(body, "endTime")),
                model::ActivityStatus::Active
            });
        return jsonResponse(201, { { "activity", activityJson(activity) } });
    });
}

WebApp::HttpResponse WebApp::purchase(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto user = requireUser(request);
        const auto activityId = parseId(request.pathParams.at("id"), "活动 ID");
        const auto result = service::FlashSaleService(connection_).purchase(user.id, activityId);
        return jsonResponse(201, {
            { "order", orderJson(result.order) },
            { "remainingStock", result.remainingStock }
        });
    });
}

WebApp::HttpResponse WebApp::listOrders(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto user = requireUser(request);
        Json orders = Json::array();
        for (const auto& order : service::OrderService(connection_).listMyOrders(user.id))
        {
            orders.push_back(orderJson(order));
        }
        return jsonResponse(200, { { "orders", std::move(orders) } });
    });
}

WebApp::HttpResponse WebApp::adminSummary(const HttpRequest& request)
{
    return handleApi([this, &request] {
        const auto admin = requireAdmin(request);
        service::ProductService productService(connection_);
        Json summaries = Json::array();
        std::uint64_t totalOrders = 0;
        std::uint64_t totalLogs = 0;
        std::uint64_t totalStock = 0;

        for (const auto& summary :
             service::AdminService(connection_).listActivitySummaries(admin.id))
        {
            totalOrders += summary.orderCount;
            totalLogs += summary.inventoryLogCount;
            totalStock += summary.activity.availableStock;
            summaries.push_back({
                { "activity", activityJson(
                    summary.activity,
                    productService.getById(summary.activity.productId)) },
                { "orderCount", summary.orderCount },
                { "inventoryLogCount", summary.inventoryLogCount }
            });
        }

        return jsonResponse(200, {
            { "summary", {
                { "activityCount", summaries.size() },
                { "orderCount", totalOrders },
                { "inventoryLogCount", totalLogs },
                { "availableStock", totalStock }
            } },
            { "activities", std::move(summaries) }
        });
    });
}

std::string WebApp::createSession(const model::User& user)
{
    static std::atomic<std::uint64_t> sequence { 0 };
    std::random_device randomDevice;
    std::ostringstream token;
    token << std::hex
          << std::chrono::steady_clock::now().time_since_epoch().count()
          << randomDevice()
          << sequence.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard lock(sessionsMutex_);
    sessions_[token.str()] = user;
    return token.str();
}

std::optional<model::User> WebApp::currentUser(const HttpRequest& request)
{
    const std::string token = bearerToken(request);
    if (token.empty())
    {
        return std::nullopt;
    }

    std::lock_guard lock(sessionsMutex_);
    const auto iterator = sessions_.find(token);
    if (iterator == sessions_.end())
    {
        return std::nullopt;
    }
    return iterator->second;
}

model::User WebApp::requireUser(const HttpRequest& request)
{
    const auto user = currentUser(request);
    if (!user.has_value())
    {
        throw ApiError(401, "请先登录");
    }
    return *user;
}

model::User WebApp::requireAdmin(const HttpRequest& request)
{
    const auto user = requireUser(request);
    if (user.role != model::UserRole::Admin)
    {
        throw ApiError(403, "需要管理员权限");
    }
    return user;
}

} // namespace campus::app
