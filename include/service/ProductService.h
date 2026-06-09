#pragma once

#include "db/MySqlConnection.h"
#include "model/Product.h"

#include <cstdint>
#include <string>
#include <vector>

namespace campus::service
{

struct PublishProductRequest
{
    std::string productNo;
    std::string title;
    std::string description;
    std::uint32_t originalPriceCents {};
};

class ProductService
{
public:
    explicit ProductService(db::MySqlConnection& connection);

    model::Product publish(std::uint64_t sellerId, const PublishProductRequest& request);
    model::Product getById(std::uint64_t productId);
    std::vector<model::Product> listVisible();

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
