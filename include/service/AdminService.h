#pragma once

#include "db/MySqlConnection.h"
#include "model/FlashSaleActivity.h"

#include <cstdint>
#include <vector>

namespace campus::service
{

struct ActivitySummary
{
    model::FlashSaleActivity activity;
    std::uint64_t orderCount {};
    std::uint64_t inventoryLogCount {};
};

class AdminService
{
public:
    explicit AdminService(db::MySqlConnection& connection);

    std::vector<ActivitySummary> listActivitySummaries(std::uint64_t operatorUserId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
