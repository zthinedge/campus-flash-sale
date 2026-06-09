#include "service/ActivityService.h"

#include "dao/ActivityDao.h"
#include "dao/ProductDao.h"
#include "dao/UserDao.h"
#include "db/Transaction.h"
#include "service/ServiceError.h"

namespace campus::service
{

ActivityService::ActivityService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

model::FlashSaleActivity ActivityService::create(
    std::uint64_t operatorUserId, const CreateActivityRequest& request)
{
    if (request.activityNo.empty() || request.flashPriceCents == 0 || request.totalStock == 0
        || request.startTime.empty() || request.endTime.empty())
    {
        throw ServiceError(ServiceErrorCode::InvalidArgument, "invalid activity input");
    }

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

    dao::ProductDao productDao(connection_);
    const auto product = productDao.findById(request.productId);
    if (!product.has_value())
    {
        throw ServiceError(ServiceErrorCode::ProductNotFound, "product does not exist");
    }

    db::Transaction transaction(connection_);
    dao::ActivityDao activityDao(connection_);
    const std::uint64_t activityId =
        activityDao.create(model::CreateFlashSaleActivity {
            request.activityNo,
            request.productId,
            request.flashPriceCents,
            request.totalStock,
            request.totalStock,
            request.startTime,
            request.endTime,
            request.status
        });
    if (!productDao.updateStatus(request.productId, model::ProductStatus::InFlashSale))
    {
        throw ServiceError(ServiceErrorCode::ProductNotFound, "product disappeared");
    }

    const auto activity = activityDao.findById(activityId);
    if (!activity.has_value())
    {
        throw ServiceError(ServiceErrorCode::ActivityNotFound, "created activity disappeared");
    }

    transaction.commit();
    return *activity;
}

model::FlashSaleActivity ActivityService::getById(std::uint64_t activityId)
{
    dao::ActivityDao activityDao(connection_);
    const auto activity = activityDao.findById(activityId);
    if (!activity.has_value())
    {
        throw ServiceError(ServiceErrorCode::ActivityNotFound, "activity does not exist");
    }
    return *activity;
}

std::vector<model::FlashSaleActivity> ActivityService::listActive()
{
    dao::ActivityDao activityDao(connection_);
    return activityDao.listActive();
}

} // namespace campus::service
