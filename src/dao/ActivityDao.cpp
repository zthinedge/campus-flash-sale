#include "dao/ActivityDao.h"

#include "db/PreparedStatement.h"

#include <string>

namespace campus::dao
{

namespace
{

constexpr const char* activityColumns =
    "id, activity_no, product_id, flash_price_cents, total_stock, available_stock, "
    "start_time, end_time, status, created_at, updated_at";

model::FlashSaleActivity mapActivity(const db::ResultRow& row)
{
    return model::FlashSaleActivity {
        row.getUInt64("id"),
        row.getString("activity_no"),
        row.getUInt64("product_id"),
        row.getUInt32("flash_price_cents"),
        row.getUInt32("total_stock"),
        row.getUInt32("available_stock"),
        row.getString("start_time"),
        row.getString("end_time"),
        model::activityStatusFromString(row.getString("status")),
        row.getString("created_at"),
        row.getString("updated_at")
    };
}

std::vector<model::FlashSaleActivity> mapActivities(const db::ResultRows& rows)
{
    std::vector<model::FlashSaleActivity> activities;
    activities.reserve(rows.size());
    for (const auto& row : rows)
    {
        activities.push_back(mapActivity(row));
    }
    return activities;
}

} // namespace

ActivityDao::ActivityDao(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::optional<model::FlashSaleActivity> ActivityDao::findById(std::uint64_t id)
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + activityColumns + " FROM flash_sale_activities WHERE id = ?");
    const auto rows = statement.query(db::SqlParams { id });
    return rows.empty()
        ? std::nullopt
        : std::optional<model::FlashSaleActivity>(mapActivity(rows.front()));
}

std::optional<model::FlashSaleActivity> ActivityDao::findByActivityNo(
    std::string_view activityNo)
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + activityColumns
            + " FROM flash_sale_activities WHERE activity_no = ?");
    const auto rows = statement.query(db::SqlParams { std::string(activityNo) });
    return rows.empty()
        ? std::nullopt
        : std::optional<model::FlashSaleActivity>(mapActivity(rows.front()));
}

std::vector<model::FlashSaleActivity> ActivityDao::listAll()
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + activityColumns
            + " FROM flash_sale_activities ORDER BY created_at DESC, id DESC");
    return mapActivities(statement.query());
}

std::vector<model::FlashSaleActivity> ActivityDao::listActive()
{
    db::PreparedStatement statement(
        connection_,
        std::string("SELECT ") + activityColumns
            + " FROM flash_sale_activities "
              "WHERE status = 'ACTIVE' AND start_time <= NOW(3) AND end_time > NOW(3) "
              "ORDER BY start_time, id");
    return mapActivities(statement.query());
}

std::uint64_t ActivityDao::create(const model::CreateFlashSaleActivity& input)
{
    db::PreparedStatement statement(
        connection_,
        "INSERT INTO flash_sale_activities "
        "(activity_no, product_id, flash_price_cents, total_stock, available_stock, "
        "start_time, end_time, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

    const auto result = statement.execute(db::SqlParams {
        input.activityNo,
        input.productId,
        static_cast<std::uint64_t>(input.flashPriceCents),
        static_cast<std::uint64_t>(input.totalStock),
        static_cast<std::uint64_t>(input.availableStock),
        input.startTime,
        input.endTime,
        std::string(model::toString(input.status))
    });
    return result.insertId;
}

bool ActivityDao::deductOne(std::uint64_t activityId)
{
    db::PreparedStatement statement(
        connection_,
        "UPDATE flash_sale_activities "
        "SET available_stock = available_stock - 1 "
        "WHERE id = ? AND status = 'ACTIVE' "
        "AND start_time <= NOW(3) AND end_time > NOW(3) AND available_stock > 0");

    const auto result = statement.execute(db::SqlParams { activityId });
    return result.affectedRows == 1;
}

} // namespace campus::dao
