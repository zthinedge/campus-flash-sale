#pragma once

#include "db/MySqlConnection.h"
#include "model/InventoryLog.h"

#include <cstdint>
#include <vector>

namespace campus::dao
{

class InventoryLogDao
{
public:
    explicit InventoryLogDao(db::MySqlConnection& connection);

    std::vector<model::InventoryLog> findByActivityId(std::uint64_t activityId);
    std::uint64_t create(const model::CreateInventoryLog& input);
    std::uint64_t countByActivityId(std::uint64_t activityId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::dao
