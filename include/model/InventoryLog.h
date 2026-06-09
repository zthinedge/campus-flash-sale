#pragma once

#include "model/Enums.h"

#include <cstdint>
#include <optional>
#include <string>

namespace campus::model
{

struct InventoryLog
{
    std::uint64_t id {};
    std::uint64_t activityId {};
    std::optional<std::uint64_t> orderId;
    std::int32_t changeAmount {};
    std::uint32_t stockAfter {};
    InventoryReason reason { InventoryReason::FlashSaleDeduct };
    std::string createdAt;
};

struct CreateInventoryLog
{
    std::uint64_t activityId {};
    std::optional<std::uint64_t> orderId;
    std::int32_t changeAmount {};
    std::uint32_t stockAfter {};
    InventoryReason reason { InventoryReason::FlashSaleDeduct };
};

} // namespace campus::model
