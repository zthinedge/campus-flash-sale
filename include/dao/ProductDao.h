#pragma once

#include "db/MySqlConnection.h"
#include "model/Product.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace campus::dao
{

class ProductDao
{
public:
    explicit ProductDao(db::MySqlConnection& connection);

    std::optional<model::Product> findById(std::uint64_t id);
    std::optional<model::Product> findByProductNo(std::string_view productNo);
    std::vector<model::Product> listVisible();
    std::uint64_t create(const model::CreateProduct& input);
    bool updateStatus(std::uint64_t productId, model::ProductStatus status);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::dao
