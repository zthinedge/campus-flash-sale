#pragma once

#include "db/MySqlConnection.h"
#include "model/Order.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace campus::dao
{

class OrderDao
{
public:
    explicit OrderDao(db::MySqlConnection& connection);

    std::optional<model::Order> findByOrderNo(std::string_view orderNo);
    std::vector<model::Order> findByUserId(std::uint64_t userId);
    std::uint64_t create(const model::CreateOrder& input);
    std::uint64_t countByActivityId(std::uint64_t activityId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::dao
