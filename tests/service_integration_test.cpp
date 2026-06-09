#include "dao/InventoryLogDao.h"
#include "dao/ProductDao.h"
#include "db/PreparedStatement.h"
#include "service/ActivityService.h"
#include "service/AdminService.h"
#include "service/AuthService.h"
#include "service/FlashSaleService.h"
#include "service/OrderService.h"
#include "service/ProductService.h"
#include "service/ServiceError.h"
#include "utils/Config.h"

#include <cassert>
#include <filesystem>
#include <string>

namespace
{

class TestDataCleanup
{
public:
    TestDataCleanup(campus::db::MySqlConnection& connection,
                    std::string username1,
                    std::string username2,
                    std::string productNo,
                    std::string activityNo)
        : connection_(connection),
          username1_(std::move(username1)),
          username2_(std::move(username2)),
          productNo_(std::move(productNo)),
          activityNo_(std::move(activityNo))
    {
    }

    ~TestDataCleanup()
    {
        try
        {
            execute(
                "DELETE l FROM inventory_logs l "
                "JOIN flash_sale_activities a ON a.id = l.activity_id "
                "WHERE a.activity_no = ?",
                campus::db::SqlParams { activityNo_ });
            execute(
                "DELETE o FROM orders o "
                "JOIN flash_sale_activities a ON a.id = o.activity_id "
                "WHERE a.activity_no = ?",
                campus::db::SqlParams { activityNo_ });
            execute(
                "DELETE FROM flash_sale_activities WHERE activity_no = ?",
                campus::db::SqlParams { activityNo_ });
            execute(
                "DELETE FROM products WHERE product_no = ?",
                campus::db::SqlParams { productNo_ });
            execute(
                "DELETE FROM users WHERE username IN (?, ?)",
                campus::db::SqlParams { username1_, username2_ });
        }
        catch (...)
        {
        }
    }

private:
    void execute(const std::string& sql, const campus::db::SqlParams& params)
    {
        campus::db::PreparedStatement statement(connection_, sql);
        statement.execute(params);
    }

    campus::db::MySqlConnection& connection_;
    std::string username1_;
    std::string username2_;
    std::string productNo_;
    std::string activityNo_;
};

void expectServiceError(
    campus::service::ServiceErrorCode expectedCode, const auto& operation)
{
    bool rejected = false;
    try
    {
        operation();
    }
    catch (const campus::service::ServiceError& error)
    {
        rejected = error.code() == expectedCode;
    }
    assert(rejected);
}

} // namespace

int main(int argc, char* argv[])
{
    assert(argc == 2);

    const auto config = campus::utils::Config::loadFromFile(std::filesystem::path(argv[1]));
    campus::db::MySqlConnection connection;
    connection.connect(config);

    const std::string suffix =
        connection.queryScalar("SELECT LEFT(REPLACE(UUID(), '-', ''), 12)");
    const std::string username1 = "svc_user1_" + suffix;
    const std::string username2 = "svc_user2_" + suffix;
    const std::string productNo = "SVC-P-" + suffix;
    const std::string activityNo = "SVC-A-" + suffix;
    TestDataCleanup cleanup(connection, username1, username2, productNo, activityNo);

    campus::service::AuthService authService(connection);
    campus::service::ProductService productService(connection);
    campus::service::ActivityService activityService(connection);
    campus::service::FlashSaleService flashSaleService(connection);
    campus::service::OrderService orderService(connection);
    campus::service::AdminService adminService(connection);

    const std::uint64_t userId1 = authService.registerUser(username1, "pass1234");
    const std::uint64_t userId2 = authService.registerUser(username2, "pass5678");
    assert(authService.login(username1, "pass1234").id == userId1);
    expectServiceError(
        campus::service::ServiceErrorCode::InvalidCredentials,
        [&] { static_cast<void>(authService.login(username1, "wrong-password")); });
    expectServiceError(
        campus::service::ServiceErrorCode::UsernameTaken,
        [&] { static_cast<void>(authService.registerUser(username1, "pass1234")); });

    const auto admin = authService.login("admin", "admin123");
    assert(admin.role == campus::model::UserRole::Admin);

    const auto product = productService.publish(
        userId1,
        campus::service::PublishProductRequest {
            productNo,
            "Service integration product",
            "Created by ProductService.",
            5000
        });

    const campus::service::CreateActivityRequest activityRequest {
        activityNo,
        product.id,
        3900,
        1,
        "2020-01-01 00:00:00.000",
        "2099-01-01 00:00:00.000",
        campus::model::ActivityStatus::Active
    };

    expectServiceError(
        campus::service::ServiceErrorCode::Forbidden,
        [&] { static_cast<void>(activityService.create(userId1, activityRequest)); });

    const auto activity = activityService.create(admin.id, activityRequest);
    assert(activity.totalStock == 1);
    assert(activity.availableStock == 1);

    campus::dao::ProductDao productDao(connection);
    assert(
        productDao.findById(product.id)->status == campus::model::ProductStatus::InFlashSale);

    const auto purchase = flashSaleService.purchase(userId1, activity.id);
    assert(purchase.remainingStock == 0);
    assert(purchase.order.userId == userId1);
    assert(purchase.order.activityId == activity.id);

    expectServiceError(
        campus::service::ServiceErrorCode::DuplicateOrder,
        [&] { static_cast<void>(flashSaleService.purchase(userId1, activity.id)); });
    expectServiceError(
        campus::service::ServiceErrorCode::SoldOut,
        [&] { static_cast<void>(flashSaleService.purchase(userId2, activity.id)); });

    const auto orders = orderService.listMyOrders(userId1);
    assert(orders.size() == 1);
    assert(orders.front().id == purchase.order.id);

    campus::dao::InventoryLogDao inventoryLogDao(connection);
    assert(inventoryLogDao.countByActivityId(activity.id) == 1);

    expectServiceError(
        campus::service::ServiceErrorCode::Forbidden,
        [&] { static_cast<void>(adminService.listActivitySummaries(userId1)); });

    const auto summaries = adminService.listActivitySummaries(admin.id);
    bool foundSummary = false;
    for (const auto& summary : summaries)
    {
        if (summary.activity.id == activity.id)
        {
            foundSummary = true;
            assert(summary.orderCount == 1);
            assert(summary.inventoryLogCount == 1);
            assert(summary.activity.availableStock == 0);
        }
    }
    assert(foundSummary);
    return 0;
}
