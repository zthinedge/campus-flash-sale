#pragma once

#include "model/Enums.h"

#include <cstdint>
#include <string>

namespace campus::model
{

struct FlashSaleActivity
{
    std::uint64_t id {};
    std::string activityNo;
    std::uint64_t productId {};
    std::uint32_t flashPriceCents {};
    std::uint32_t totalStock {};
    std::uint32_t availableStock {};
    std::string startTime;
    std::string endTime;
    ActivityStatus status { ActivityStatus::Pending };
    std::string createdAt;
    std::string updatedAt;
};

struct CreateFlashSaleActivity
{
    std::string activityNo;
    std::uint64_t productId {};
    std::uint32_t flashPriceCents {};
    std::uint32_t totalStock {};
    std::uint32_t availableStock {};
    std::string startTime;
    std::string endTime;
    ActivityStatus status { ActivityStatus::Pending };
};

} // namespace campus::model
