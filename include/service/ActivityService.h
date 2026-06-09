#pragma once

#include "db/MySqlConnection.h"
#include "model/FlashSaleActivity.h"

#include <cstdint>
#include <string>
#include <vector>

namespace campus::service
{

struct CreateActivityRequest
{
    std::string activityNo;
    std::uint64_t productId {};
    std::uint32_t flashPriceCents {};
    std::uint32_t totalStock {};
    std::string startTime;
    std::string endTime;
    model::ActivityStatus status { model::ActivityStatus::Pending };
};

class ActivityService
{
public:
    explicit ActivityService(db::MySqlConnection& connection);

    model::FlashSaleActivity create(
        std::uint64_t operatorUserId, const CreateActivityRequest& request);
    model::FlashSaleActivity getById(std::uint64_t activityId);
    std::vector<model::FlashSaleActivity> listActive();

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
