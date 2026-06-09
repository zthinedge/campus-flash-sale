#include "dao/InventoryLogDao.h"

#include "db/PreparedStatement.h"

#include <string>

namespace campus::dao
{

namespace
{

model::InventoryLog mapInventoryLog(const db::ResultRow& row)
{
    std::optional<std::uint64_t> orderId;
    if (!row.isNull("order_id"))
    {
        orderId = row.getUInt64("order_id");
    }

    return model::InventoryLog {
        row.getUInt64("id"),
        row.getUInt64("activity_id"),
        orderId,
        row.getInt32("change_amount"),
        row.getUInt32("stock_after"),
        model::inventoryReasonFromString(row.getString("reason")),
        row.getString("created_at")
    };
}

} // namespace

InventoryLogDao::InventoryLogDao(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::vector<model::InventoryLog> InventoryLogDao::findByActivityId(std::uint64_t activityId)
{
    db::PreparedStatement statement(
        connection_,
        "SELECT id, activity_id, order_id, change_amount, stock_after, reason, created_at "
        "FROM inventory_logs WHERE activity_id = ? ORDER BY created_at, id");
    const auto rows = statement.query(db::SqlParams { activityId });

    std::vector<model::InventoryLog> logs;
    logs.reserve(rows.size());
    for (const auto& row : rows)
    {
        logs.push_back(mapInventoryLog(row));
    }
    return logs;
}

std::uint64_t InventoryLogDao::create(const model::CreateInventoryLog& input)
{
    db::PreparedStatement statement(
        connection_,
        "INSERT INTO inventory_logs "
        "(activity_id, order_id, change_amount, stock_after, reason) "
        "VALUES (?, ?, ?, ?, ?)");

    const db::SqlParam orderId =
        input.orderId.has_value() ? db::SqlParam(*input.orderId) : db::SqlParam(nullptr);
    const auto result = statement.execute(db::SqlParams {
        input.activityId,
        orderId,
        static_cast<std::int64_t>(input.changeAmount),
        static_cast<std::uint64_t>(input.stockAfter),
        std::string(model::toString(input.reason))
    });
    return result.insertId;
}

std::uint64_t InventoryLogDao::countByActivityId(std::uint64_t activityId)
{
    db::PreparedStatement statement(
        connection_,
        "SELECT COUNT(*) AS log_count FROM inventory_logs WHERE activity_id = ?");
    const auto rows = statement.query(db::SqlParams { activityId });
    return rows.front().getUInt64("log_count");
}

} // namespace campus::dao
