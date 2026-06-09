#pragma once

#include "db/MySqlConnection.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace campus::db
{

using SqlParam = std::variant<std::nullptr_t, std::int64_t, std::uint64_t, std::string>;
using SqlParams = std::vector<SqlParam>;

struct ExecuteResult
{
    std::uint64_t insertId {};
    std::uint64_t affectedRows {};
};

class ResultRow
{
public:
    bool hasColumn(const std::string& name) const;
    bool isNull(const std::string& name) const;
    std::optional<std::string> getOptionalString(const std::string& name) const;
    std::string getString(const std::string& name) const;
    std::uint64_t getUInt64(const std::string& name) const;
    std::uint32_t getUInt32(const std::string& name) const;
    std::int32_t getInt32(const std::string& name) const;

private:
    friend class PreparedStatement;

    std::unordered_map<std::string, std::optional<std::string>> values_;
};

using ResultRows = std::vector<ResultRow>;

class PreparedStatement
{
public:
    PreparedStatement(MySqlConnection& connection, const std::string& sql);
    ~PreparedStatement();

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;
    PreparedStatement(PreparedStatement&&) = delete;
    PreparedStatement& operator=(PreparedStatement&&) = delete;

    ExecuteResult execute(const SqlParams& params = {});
    ResultRows query(const SqlParams& params = {});

private:
    void resetAndValidateParameterCount(const SqlParams& params);

    MYSQL_STMT* statement_ { nullptr };
};

} // namespace campus::db
