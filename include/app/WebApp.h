#pragma once

#include "db/MySqlConnection.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "model/User.h"
#include "utils/Config.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace campus::app
{

class WebApp
{
public:
    WebApp(db::MySqlConnection& connection,
           utils::Config config,
           std::filesystem::path staticRoot);

    int run();

private:
    using HttpRequest = reactor_http_kit::http::HttpRequest;
    using HttpResponse = reactor_http_kit::http::HttpResponse;

    HttpResponse health(const HttpRequest& request);
    HttpResponse registerUser(const HttpRequest& request);
    HttpResponse login(const HttpRequest& request);
    HttpResponse logout(const HttpRequest& request);
    HttpResponse me(const HttpRequest& request);
    HttpResponse listProducts(const HttpRequest& request);
    HttpResponse getProduct(const HttpRequest& request);
    HttpResponse publishProduct(const HttpRequest& request);
    HttpResponse listActivities(const HttpRequest& request);
    HttpResponse createActivity(const HttpRequest& request);
    HttpResponse purchase(const HttpRequest& request);
    HttpResponse listOrders(const HttpRequest& request);
    HttpResponse adminSummary(const HttpRequest& request);

    std::string createSession(const model::User& user);
    std::optional<model::User> currentUser(const HttpRequest& request);
    model::User requireUser(const HttpRequest& request);
    model::User requireAdmin(const HttpRequest& request);

    db::MySqlConnection& connection_;
    utils::Config config_;
    std::filesystem::path staticRoot_;
    std::mutex sessionsMutex_;
    std::unordered_map<std::string, model::User> sessions_;
};

} // namespace campus::app
