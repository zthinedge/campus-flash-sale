#include "service/OrderService.h"

#include "dao/OrderDao.h"
#include "dao/UserDao.h"
#include "service/ServiceError.h"

namespace campus::service
{

OrderService::OrderService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::vector<model::Order> OrderService::listMyOrders(std::uint64_t userId)
{
    dao::UserDao userDao(connection_);
    const auto user = userDao.findById(userId);
    if (!user.has_value())
    {
        throw ServiceError(ServiceErrorCode::UserNotFound, "user does not exist");
    }
    if (user->status != model::UserStatus::Active)
    {
        throw ServiceError(ServiceErrorCode::UserDisabled, "user account is disabled");
    }

    dao::OrderDao orderDao(connection_);
    return orderDao.findByUserId(userId);
}

} // namespace campus::service
