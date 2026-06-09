#include "service/FlashSaleService.h"

#include "dao/ActivityDao.h"
#include "dao/InventoryLogDao.h"
#include "dao/OrderDao.h"
#include "dao/ProductDao.h"
#include "dao/UserDao.h"
#include "db/MySqlError.h"
#include "db/Transaction.h"
#include "service/ServiceError.h"

#include <atomic>
#include <chrono>
#include <string>

namespace campus::service
{

namespace
{

std::string generateOrderNo()
{
    static std::atomic<std::uint32_t> sequence { 0 };
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
    const std::uint32_t suffix = sequence.fetch_add(1, std::memory_order_relaxed) % 1000000;
    return "ORD-" + std::to_string(milliseconds) + "-" + std::to_string(suffix);
}

} // namespace

FlashSaleService::FlashSaleService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

PurchaseResult FlashSaleService::purchase(std::uint64_t userId, std::uint64_t activityId)
{
    try
    {
        db::Transaction transaction(connection_);
        dao::UserDao userDao(connection_);
        dao::ProductDao productDao(connection_);
        dao::ActivityDao activityDao(connection_);
        dao::OrderDao orderDao(connection_);
        dao::InventoryLogDao inventoryLogDao(connection_);

        const auto user = userDao.findById(userId);
        if (!user.has_value())
        {
            throw ServiceError(ServiceErrorCode::UserNotFound, "user does not exist");
        }
        if (user->status != model::UserStatus::Active)
        {
            throw ServiceError(ServiceErrorCode::UserDisabled, "user account is disabled");
        }
        if (orderDao.findByActivityAndUser(activityId, userId).has_value())
        {
            throw ServiceError(
                ServiceErrorCode::DuplicateOrder, "user has already purchased this activity");
        }

        const auto activityBefore = activityDao.findById(activityId);
        if (!activityBefore.has_value())
        {
            throw ServiceError(ServiceErrorCode::ActivityNotFound, "activity does not exist");
        }
        const auto product = productDao.findById(activityBefore->productId);
        if (!product.has_value())
        {
            throw ServiceError(ServiceErrorCode::ProductNotFound, "product does not exist");
        }

        if (!activityDao.deductOne(activityId))
        {
            const auto current = activityDao.findById(activityId);
            if (current.has_value() && current->availableStock == 0)
            {
                throw ServiceError(ServiceErrorCode::SoldOut, "activity stock is sold out");
            }
            throw ServiceError(
                ServiceErrorCode::ActivityUnavailable, "activity is not available");
        }

        const auto activityAfter = activityDao.findById(activityId);
        if (!activityAfter.has_value())
        {
            throw ServiceError(ServiceErrorCode::ActivityNotFound, "activity disappeared");
        }

        const std::string orderNo = generateOrderNo();
        const std::uint64_t orderId = orderDao.create(model::CreateOrder {
            orderNo,
            userId,
            activityId,
            product->id,
            product->title,
            activityBefore->flashPriceCents,
            1,
            activityBefore->flashPriceCents,
            model::OrderStatus::Created
        });

        inventoryLogDao.create(model::CreateInventoryLog {
            activityId,
            orderId,
            -1,
            activityAfter->availableStock,
            model::InventoryReason::FlashSaleDeduct
        });

        const auto order = orderDao.findByOrderNo(orderNo);
        if (!order.has_value())
        {
            throw std::runtime_error("created order could not be loaded");
        }

        transaction.commit();
        return PurchaseResult { *order, activityAfter->availableStock };
    }
    catch (const db::MySqlError& error)
    {
        if (error.code() == 1062)
        {
            throw ServiceError(
                ServiceErrorCode::DuplicateOrder, "user has already purchased this activity");
        }
        throw;
    }
}

} // namespace campus::service
