#include "dao/ProductDao.h"

#include "db/PreparedStatement.h"

#include <string>

namespace campus::dao
{

namespace
{

constexpr const char* productColumns =
    "id, product_no, seller_id, title, description, original_price_cents, status, "
    "created_at, updated_at";

model::Product mapProduct(const db::ResultRow& row)
{
    return model::Product {
        row.getUInt64("id"),
        row.getString("product_no"),
        row.getUInt64("seller_id"),
        row.getString("title"),
        row.getString("description"),
        row.getUInt32("original_price_cents"),
        model::productStatusFromString(row.getString("status")),
        row.getString("created_at"),
        row.getString("updated_at")
    };
}

std::vector<model::Product> mapProducts(const db::ResultRows& rows)
{
    std::vector<model::Product> products;
    products.reserve(rows.size());
    for (const auto& row : rows)
    {
        products.push_back(mapProduct(row));
    }
    return products;
}

} // namespace

ProductDao::ProductDao(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::optional<model::Product> ProductDao::findById(std::uint64_t id)
{
    db::PreparedStatement statement(
        connection_, std::string("SELECT ") + productColumns + " FROM products WHERE id = ?");
    const auto rows = statement.query(db::SqlParams { id });
    return rows.empty() ? std::nullopt : std::optional<model::Product>(mapProduct(rows.front()));
}

std::optional<model::Product> ProductDao::findByProductNo(std::string_view productNo)
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + productColumns + " FROM products WHERE product_no = ?");
    const auto rows = statement.query(db::SqlParams { std::string(productNo) });
    return rows.empty() ? std::nullopt : std::optional<model::Product>(mapProduct(rows.front()));
}

std::vector<model::Product> ProductDao::listVisible()
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + productColumns
            + " FROM products WHERE status <> 'OFF_SHELF' ORDER BY created_at DESC, id DESC");
    return mapProducts(statement.query());
}

std::uint64_t ProductDao::create(const model::CreateProduct& input)
{
    db::PreparedStatement statement(
        connection_,
        "INSERT INTO products "
        "(product_no, seller_id, title, description, original_price_cents, status) "
        "VALUES (?, ?, ?, ?, ?, ?)");

    const auto result = statement.execute(db::SqlParams {
        input.productNo,
        input.sellerId,
        input.title,
        input.description,
        static_cast<std::uint64_t>(input.originalPriceCents),
        std::string(model::toString(input.status))
    });
    return result.insertId;
}

bool ProductDao::updateStatus(std::uint64_t productId, model::ProductStatus status)
{
    db::PreparedStatement statement(
        connection_, "UPDATE products SET status = ? WHERE id = ?");
    const auto result = statement.execute(db::SqlParams {
        std::string(model::toString(status)),
        productId
    });
    return result.affectedRows == 1;
}

} // namespace campus::dao
