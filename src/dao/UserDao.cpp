#include "dao/UserDao.h"

#include "db/PreparedStatement.h"

#include <string>

namespace campus::dao
{

namespace
{

constexpr const char* userColumns =
    "id, username, password_hash, role, status, created_at, updated_at";

model::User mapUser(const db::ResultRow& row)
{
    return model::User {
        row.getUInt64("id"),
        row.getString("username"),
        row.getString("password_hash"),
        model::userRoleFromString(row.getString("role")),
        model::userStatusFromString(row.getString("status")),
        row.getString("created_at"),
        row.getString("updated_at")
    };
}

} // namespace

UserDao::UserDao(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::optional<model::User> UserDao::findById(std::uint64_t id)
{
    db::PreparedStatement statement(
        connection_, std::string("SELECT ") + userColumns + " FROM users WHERE id = ?");
    const auto rows = statement.query(db::SqlParams { id });
    return rows.empty() ? std::nullopt : std::optional<model::User>(mapUser(rows.front()));
}

std::optional<model::User> UserDao::findByUsername(std::string_view username)
{
    db::PreparedStatement statement(
        connection_, std::string("SELECT ") + userColumns + " FROM users WHERE username = ?");
    const auto rows = statement.query(db::SqlParams { std::string(username) });
    return rows.empty() ? std::nullopt : std::optional<model::User>(mapUser(rows.front()));
}

std::uint64_t UserDao::create(const model::CreateUser& input)
{
    db::PreparedStatement statement(
        connection_,
        "INSERT INTO users (username, password_hash, role, status) VALUES (?, ?, ?, ?)");

    const auto result = statement.execute(db::SqlParams {
        input.username,
        input.passwordHash,
        std::string(model::toString(input.role)),
        std::string(model::toString(input.status))
    });
    return result.insertId;
}

} // namespace campus::dao
