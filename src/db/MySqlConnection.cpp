#include "db/MySqlConnection.h"

#include "db/MySqlError.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace campus::db
{

namespace
{

MySqlError mysqlError(MYSQL* handle, const std::string& operation)
{
    if (handle == nullptr)
    {
        return MySqlError(0, "", operation + " failed: MySQL handle is null");
    }
    return MySqlError(
        mysql_errno(handle), mysql_sqlstate(handle), operation + " failed: " + mysql_error(handle));
}

} // namespace

MySqlConnection::MySqlConnection()
    : handle_(mysql_init(nullptr))
{
    if (handle_ == nullptr)
    {
        throw std::runtime_error("mysql_init failed");
    }
}

MySqlConnection::~MySqlConnection()
{
    close();
}

MySqlConnection::MySqlConnection(MySqlConnection&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)),
      connected_(std::exchange(other.connected_, false))
{
}

MySqlConnection& MySqlConnection::operator=(MySqlConnection&& other) noexcept
{
    if (this != &other)
    {
        close();
        handle_ = std::exchange(other.handle_, nullptr);
        connected_ = std::exchange(other.connected_, false);
    }
    return *this;
}

void MySqlConnection::connect(const utils::Config& config)
{
    if (connected_)
    {
        throw std::logic_error("MySQL connection is already connected");
    }
    if (handle_ == nullptr)
    {
        throw std::logic_error("MySQL connection has no valid handle");
    }

    constexpr unsigned int connectTimeoutSeconds = 5;
    if (mysql_options(handle_, MYSQL_OPT_CONNECT_TIMEOUT, &connectTimeoutSeconds) != 0)
    {
        throw mysqlError(handle_, "setting MySQL connect timeout");
    }
    if (mysql_options(handle_, MYSQL_SET_CHARSET_NAME, "utf8mb4") != 0)
    {
        throw mysqlError(handle_, "setting MySQL character set");
    }

    if (mysql_real_connect(handle_,
                           config.mysqlHost.c_str(),
                           config.mysqlUser.c_str(),
                           config.mysqlPassword.c_str(),
                           config.mysqlDatabase.c_str(),
                           config.mysqlPort,
                           nullptr,
                           0)
        == nullptr)
    {
        throw mysqlError(handle_, "connecting to MySQL");
    }

    connected_ = true;
}

void MySqlConnection::ping()
{
    ensureConnected();
    if (mysql_ping(handle_) != 0)
    {
        throw mysqlError(handle_, "pinging MySQL");
    }
}

void MySqlConnection::execute(const std::string& sql)
{
    ensureConnected();
    if (mysql_query(handle_, sql.c_str()) != 0)
    {
        throw mysqlError(handle_, "executing statement");
    }

    while (mysql_next_result(handle_) == 0)
    {
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> extraResult(
            mysql_store_result(handle_), &mysql_free_result);
    }
}

std::string MySqlConnection::queryScalar(const std::string& sql)
{
    ensureConnected();
    if (mysql_query(handle_, sql.c_str()) != 0)
    {
        throw mysqlError(handle_, "executing query");
    }

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
        mysql_store_result(handle_), &mysql_free_result);
    if (result == nullptr)
    {
        if (mysql_field_count(handle_) == 0)
        {
            throw std::runtime_error("query did not return a result set");
        }
        throw mysqlError(handle_, "reading query result");
    }

    MYSQL_ROW row = mysql_fetch_row(result.get());
    if (row == nullptr || mysql_num_fields(result.get()) == 0 || row[0] == nullptr)
    {
        throw std::runtime_error("query returned no scalar value");
    }

    unsigned long* lengths = mysql_fetch_lengths(result.get());
    if (lengths == nullptr)
    {
        throw mysqlError(handle_, "reading scalar length");
    }

    return std::string(row[0], lengths[0]);
}

unsigned long long MySqlConnection::affectedRows() const
{
    ensureConnected();
    return mysql_affected_rows(handle_);
}

void MySqlConnection::close() noexcept
{
    if (handle_ != nullptr)
    {
        mysql_close(handle_);
        handle_ = nullptr;
    }
    connected_ = false;
}

void MySqlConnection::ensureConnected() const
{
    if (handle_ == nullptr || !connected_)
    {
        throw std::logic_error("MySQL connection is not connected");
    }
}

} // namespace campus::db
