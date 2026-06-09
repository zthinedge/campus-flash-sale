#pragma once

#include "db/MySqlConnection.h"
#include "model/Order.h"

#include <cstdint>

namespace campus::service
{

struct PurchaseResult
{
    model::Order order;
    std::uint32_t remainingStock {};
};

class FlashSaleService
{
public:
    explicit FlashSaleService(db::MySqlConnection& connection);

    PurchaseResult purchase(std::uint64_t userId, std::uint64_t activityId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
