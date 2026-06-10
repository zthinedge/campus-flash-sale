#include "dao/ActivityDao.h"
#include "dao/InventoryLogDao.h"
#include "dao/OrderDao.h"
#include "db/PreparedStatement.h"
#include "service/ActivityService.h"
#include "service/AuthService.h"
#include "service/FlashSaleService.h"
#include "service/ProductService.h"
#include "service/ServiceError.h"
#include "utils/Config.h"

#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{

class TestDataCleanup
{
public:
    TestDataCleanup(campus::db::MySqlConnection& connection,
                    std::string usernamePrefix,
                    std::string productNo,
                    std::string activityNo)
        : connection_(connection),
          usernamePrefix_(std::move(usernamePrefix)),
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
                "DELETE FROM users WHERE username LIKE ?",
                campus::db::SqlParams { usernamePrefix_ + "%" });
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
    std::string usernamePrefix_;
    std::string productNo_;
    std::string activityNo_;
};

} // namespace

int main(int argc, char* argv[])
{
    assert(argc == 2);

    constexpr std::uint32_t stock = 10;
    constexpr std::size_t contenderCount = 30;

    const auto config = campus::utils::Config::loadFromFile(std::filesystem::path(argv[1]));
    campus::db::MySqlConnection setupConnection;
    setupConnection.connect(config);

    const std::string suffix =
        setupConnection.queryScalar("SELECT LEFT(REPLACE(UUID(), '-', ''), 10)");
    const std::string usernamePrefix = "load_" + suffix + "_";
    const std::string productNo = "LOAD-P-" + suffix;
    const std::string activityNo = "LOAD-A-" + suffix;
    TestDataCleanup cleanup(setupConnection, usernamePrefix, productNo, activityNo);

    campus::service::AuthService authService(setupConnection);
    std::vector<std::uint64_t> userIds;
    userIds.reserve(contenderCount);
    for (std::size_t index = 0; index < contenderCount; ++index)
    {
        userIds.push_back(
            authService.registerUser(usernamePrefix + std::to_string(index), "load-test-password"));
    }

    campus::service::ProductService productService(setupConnection);
    const auto product = productService.publish(
        userIds.front(),
        campus::service::PublishProductRequest {
            productNo,
            "Concurrent flash sale product",
            "Created by flash_sale_concurrency_test.",
            5000
        });

    const auto admin = authService.login("admin", "admin123");
    campus::service::ActivityService activityService(setupConnection);
    const auto activity = activityService.create(
        admin.id,
        campus::service::CreateActivityRequest {
            activityNo,
            product.id,
            3900,
            stock,
            "2020-01-01 00:00:00.000",
            "2099-01-01 00:00:00.000",
            campus::model::ActivityStatus::Active
        });

    std::barrier startLine(static_cast<std::ptrdiff_t>(contenderCount));
    std::atomic<std::uint32_t> successCount { 0 };
    std::atomic<std::uint32_t> soldOutCount { 0 };
    std::mutex errorMutex;
    std::vector<std::string> unexpectedErrors;
    std::vector<std::uint64_t> successfulUsers;
    std::mutex successfulUsersMutex;

    std::vector<std::thread> workers;
    workers.reserve(contenderCount);
    const auto startedAt = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < contenderCount; ++index)
    {
        workers.emplace_back(
            [&, index]
            {
                try
                {
                    campus::db::MySqlConnection connection;
                    connection.connect(config);
                    campus::service::FlashSaleService flashSaleService(connection);
                    startLine.arrive_and_wait();
                    static_cast<void>(flashSaleService.purchase(userIds[index], activity.id));
                    successCount.fetch_add(1, std::memory_order_relaxed);

                    std::lock_guard lock(successfulUsersMutex);
                    successfulUsers.push_back(userIds[index]);
                }
                catch (const campus::service::ServiceError& error)
                {
                    if (error.code() == campus::service::ServiceErrorCode::SoldOut
                        || error.code()
                            == campus::service::ServiceErrorCode::ActivityUnavailable)
                    {
                        soldOutCount.fetch_add(1, std::memory_order_relaxed);
                        return;
                    }

                    std::lock_guard lock(errorMutex);
                    unexpectedErrors.push_back(error.what());
                }
                catch (const std::exception& error)
                {
                    std::lock_guard lock(errorMutex);
                    unexpectedErrors.push_back(error.what());
                }
            });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }
    const auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - startedAt)
                                         .count();

    assert(unexpectedErrors.empty());
    assert(successCount.load() == stock);
    assert(soldOutCount.load() == contenderCount - stock);

    campus::dao::ActivityDao activityDao(setupConnection);
    campus::dao::OrderDao orderDao(setupConnection);
    campus::dao::InventoryLogDao inventoryLogDao(setupConnection);
    const auto finalActivity = activityDao.findById(activity.id);
    assert(finalActivity.has_value());
    assert(finalActivity->availableStock == 0);
    assert(orderDao.countByActivityId(activity.id) == stock);
    assert(inventoryLogDao.countByActivityId(activity.id) == stock);

    campus::db::PreparedStatement duplicateCheck(
        setupConnection,
        "SELECT COUNT(*) AS duplicate_groups FROM ("
        "SELECT user_id FROM orders WHERE activity_id = ? "
        "GROUP BY user_id HAVING COUNT(*) > 1"
        ") AS duplicates");
    const auto duplicateRows =
        duplicateCheck.query(campus::db::SqlParams { activity.id });
    assert(duplicateRows.front().getUInt64("duplicate_groups") == 0);

    assert(!successfulUsers.empty());
    campus::service::FlashSaleService duplicateService(setupConnection);
    bool duplicateRejected = false;
    try
    {
        static_cast<void>(duplicateService.purchase(successfulUsers.front(), activity.id));
    }
    catch (const campus::service::ServiceError& error)
    {
        duplicateRejected =
            error.code() == campus::service::ServiceErrorCode::DuplicateOrder;
    }
    assert(duplicateRejected);

    std::cout << "[CONCURRENCY] contenders=" << contenderCount << " initial_stock=" << stock
              << " success=" << successCount.load()
              << " rejected=" << soldOutCount.load()
              << " final_stock=" << finalActivity->availableStock
              << " orders=" << orderDao.countByActivityId(activity.id)
              << " inventory_logs=" << inventoryLogDao.countByActivityId(activity.id)
              << " duplicate_groups=0"
              << " elapsed_ms=" << elapsedMilliseconds << '\n';

    return 0;
}
