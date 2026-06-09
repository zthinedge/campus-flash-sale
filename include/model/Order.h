#pragma once

#include "model/Enums.h"

#include <cstdint>
#include <string>

namespace campus::model
{

struct Order
{
    std::uint64_t id {};
    std::string orderNo;
    std::uint64_t userId {};
    std::uint64_t activityId {};
    std::uint64_t productId {};
    std::string productTitleSnapshot;
    std::uint32_t purchasePriceCents {};
    std::uint32_t quantity { 1 };
    std::uint32_t totalAmountCents {};
    OrderStatus status { OrderStatus::Created };
    std::string createdAt;
    std::string updatedAt;
};

struct CreateOrder
{
    std::string orderNo;
    std::uint64_t userId {};
    std::uint64_t activityId {};
    std::uint64_t productId {};
    std::string productTitleSnapshot;
    std::uint32_t purchasePriceCents {};
    std::uint32_t quantity { 1 };
    std::uint32_t totalAmountCents {};
    OrderStatus status { OrderStatus::Created };
};

} // namespace campus::model
