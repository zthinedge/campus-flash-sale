#include "dao/ActivityDao.h"
#include "dao/InventoryLogDao.h"
#include "dao/OrderDao.h"
#include "dao/ProductDao.h"
#include "dao/UserDao.h"
#include "db/PreparedStatement.h"
#include "db/Transaction.h"
#include "utils/Config.h"

#include <cassert>
#include <filesystem>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[])
{
    assert(argc == 2);

    const auto config = campus::utils::Config::loadFromFile(std::filesystem::path(argv[1]));
    campus::db::MySqlConnection connection;
    connection.connect(config);

    campus::dao::UserDao userDao(connection);
    campus::dao::ProductDao productDao(connection);
    campus::dao::ActivityDao activityDao(connection);
    campus::dao::OrderDao orderDao(connection);
    campus::dao::InventoryLogDao inventoryLogDao(connection);

    const auto seededUser = userDao.findByUsername("student01");
    assert(seededUser.has_value());

    bool parameterCountRejected = false;
    try
    {
        campus::db::PreparedStatement statement(connection, "SELECT ?");
        static_cast<void>(statement.query());
    }
    catch (const std::invalid_argument&)
    {
        parameterCountRejected = true;
    }
    assert(parameterCountRejected);

    const std::string suffix =
        connection.queryScalar("SELECT LEFT(REPLACE(UUID(), '-', ''), 16)");
    const std::string username = "dao'user_" + suffix;
    const std::string productNo = "DAO-P-" + suffix;
    const std::string activityNo = "DAO-A-" + suffix;
    const std::string orderNo = "DAO-O-" + suffix;

    {
        campus::db::Transaction transaction(connection);

        const std::uint64_t userId = userDao.create(campus::model::CreateUser {
            username,
            "test_password_hash",
            campus::model::UserRole::User,
            campus::model::UserStatus::Active
        });

        const auto createdUser = userDao.findByUsername(username);
        assert(createdUser.has_value());
        assert(createdUser->id == userId);
        assert(createdUser->username == username);

        const std::uint64_t productId = productDao.create(campus::model::CreateProduct {
            productNo,
            userId,
            "DAO integration product",
            "Created inside a transaction and rolled back after verification.",
            5000,
            campus::model::ProductStatus::InFlashSale
        });

        const auto product = productDao.findByProductNo(productNo);
        assert(product.has_value());
        assert(product->id == productId);
        assert(product->sellerId == userId);

        const std::uint64_t activityId =
            activityDao.create(campus::model::CreateFlashSaleActivity {
                activityNo,
                productId,
                3900,
                2,
                2,
                "2020-01-01 00:00:00.000",
                "2099-01-01 00:00:00.000",
                campus::model::ActivityStatus::Active
            });

        assert(activityDao.deductOne(activityId));
        const auto activity = activityDao.findById(activityId);
        assert(activity.has_value());
        assert(activity->availableStock == 1);

        const std::uint64_t orderId = orderDao.create(campus::model::CreateOrder {
            orderNo,
            userId,
            activityId,
            productId,
            product->title,
            3900,
            1,
            3900,
            campus::model::OrderStatus::Created
        });

        const std::uint64_t logId =
            inventoryLogDao.create(campus::model::CreateInventoryLog {
                activityId,
                orderId,
                -1,
                1,
                campus::model::InventoryReason::FlashSaleDeduct
            });

        assert(orderId > 0);
        assert(logId > 0);
        assert(orderDao.countByActivityId(activityId) == 1);
        assert(inventoryLogDao.countByActivityId(activityId) == 1);

        const auto orders = orderDao.findByUserId(userId);
        assert(orders.size() == 1);
        assert(orders.front().orderNo == orderNo);

        const auto logs = inventoryLogDao.findByActivityId(activityId);
        assert(logs.size() == 1);
        assert(logs.front().orderId == orderId);
        assert(logs.front().stockAfter == 1);
    }

    assert(!userDao.findByUsername(username).has_value());
    assert(!productDao.findByProductNo(productNo).has_value());
    assert(!activityDao.findByActivityNo(activityNo).has_value());
    assert(!orderDao.findByOrderNo(orderNo).has_value());
    return 0;
}
