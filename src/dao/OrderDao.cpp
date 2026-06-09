#include "dao/OrderDao.h"

#include "db/PreparedStatement.h"

#include <string>

namespace campus::dao
{

namespace
{

constexpr const char* orderColumns =
    "id, order_no, user_id, activity_id, product_id, product_title_snapshot, "
    "purchase_price_cents, quantity, total_amount_cents, status, created_at, updated_at";

model::Order mapOrder(const db::ResultRow& row)
{
    return model::Order {
        row.getUInt64("id"),
        row.getString("order_no"),
        row.getUInt64("user_id"),
        row.getUInt64("activity_id"),
        row.getUInt64("product_id"),
        row.getString("product_title_snapshot"),
        row.getUInt32("purchase_price_cents"),
        row.getUInt32("quantity"),
        row.getUInt32("total_amount_cents"),
        model::orderStatusFromString(row.getString("status")),
        row.getString("created_at"),
        row.getString("updated_at")
    };
}

} // namespace

OrderDao::OrderDao(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::optional<model::Order> OrderDao::findByOrderNo(std::string_view orderNo)
{
    db::PreparedStatement statement(
        connection_, std::string("SELECT ") + orderColumns + " FROM orders WHERE order_no = ?");
    const auto rows = statement.query(db::SqlParams { std::string(orderNo) });
    return rows.empty() ? std::nullopt : std::optional<model::Order>(mapOrder(rows.front()));
}

std::vector<model::Order> OrderDao::findByUserId(std::uint64_t userId)
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + orderColumns
            + " FROM orders WHERE user_id = ? ORDER BY created_at DESC, id DESC");
    const auto rows = statement.query(db::SqlParams { userId });

    std::vector<model::Order> orders;
    orders.reserve(rows.size());
    for (const auto& row : rows)
    {
        orders.push_back(mapOrder(row));
    }
    return orders;
}

std::uint64_t OrderDao::create(const model::CreateOrder& input)
{
    db::PreparedStatement statement(
        connection_,
        "INSERT INTO orders "
        "(order_no, user_id, activity_id, product_id, product_title_snapshot, "
        "purchase_price_cents, quantity, total_amount_cents, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    const auto result = statement.execute(db::SqlParams {
        input.orderNo,
        input.userId,
        input.activityId,
        input.productId,
        input.productTitleSnapshot,
        static_cast<std::uint64_t>(input.purchasePriceCents),
        static_cast<std::uint64_t>(input.quantity),
        static_cast<std::uint64_t>(input.totalAmountCents),
        std::string(model::toString(input.status))
    });
    return result.insertId;
}

std::uint64_t OrderDao::countByActivityId(std::uint64_t activityId)
{
    db::PreparedStatement statement(
        connection_, "SELECT COUNT(*) AS order_count FROM orders WHERE activity_id = ?");
    const auto rows = statement.query(db::SqlParams { activityId });
    return rows.front().getUInt64("order_count");
}

} // namespace campus::dao
