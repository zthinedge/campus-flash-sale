#pragma once

#include "utils/Config.h"

#include <mysql/mysql.h>

#include <string>

namespace campus::db
{

class MySqlConnection
{
public:
    MySqlConnection();
    ~MySqlConnection();

    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;

    MySqlConnection(MySqlConnection&& other) noexcept;
    MySqlConnection& operator=(MySqlConnection&& other) noexcept;

    void connect(const utils::Config& config);
    void ping();
    std::string queryScalar(const std::string& sql);

private:
    void close() noexcept;
    void ensureConnected() const;

    MYSQL* handle_ { nullptr };
    bool connected_ { false };
};

} // namespace campus::db
