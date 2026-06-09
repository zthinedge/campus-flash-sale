#include "service/AdminService.h"

#include "dao/ActivityDao.h"
#include "dao/InventoryLogDao.h"
#include "dao/OrderDao.h"
#include "dao/UserDao.h"
#include "service/ServiceError.h"

namespace campus::service
{

AdminService::AdminService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::vector<ActivitySummary> AdminService::listActivitySummaries(std::uint64_t operatorUserId)
{
    dao::UserDao userDao(connection_);
    const auto operatorUser = userDao.findById(operatorUserId);
    if (!operatorUser.has_value())
    {
        throw ServiceError(ServiceErrorCode::UserNotFound, "operator does not exist");
    }
    if (operatorUser->status != model::UserStatus::Active)
    {
        throw ServiceError(ServiceErrorCode::UserDisabled, "operator account is disabled");
    }
    if (operatorUser->role != model::UserRole::Admin)
    {
        throw ServiceError(ServiceErrorCode::Forbidden, "administrator role is required");
    }

    dao::ActivityDao activityDao(connection_);
    dao::OrderDao orderDao(connection_);
    dao::InventoryLogDao inventoryLogDao(connection_);

    const auto activities = activityDao.listAll();
    std::vector<ActivitySummary> summaries;
    summaries.reserve(activities.size());
    for (const auto& activity : activities)
    {
        summaries.push_back(ActivitySummary {
            activity,
            orderDao.countByActivityId(activity.id),
            inventoryLogDao.countByActivityId(activity.id)
        });
    }
    return summaries;
}

} // namespace campus::service
