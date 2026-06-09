#pragma once

#include "db/MySqlConnection.h"
#include "model/Order.h"

#include <cstdint>
#include <vector>

namespace campus::service
{

class OrderService
{
public:
    explicit OrderService(db::MySqlConnection& connection);

    std::vector<model::Order> listMyOrders(std::uint64_t userId);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
