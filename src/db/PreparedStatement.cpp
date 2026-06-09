#include "db/PreparedStatement.h"

#include "db/MySqlError.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace campus::db
{

namespace
{

MySqlError statementError(MYSQL_STMT* statement, const std::string& operation)
{
    if (statement == nullptr)
    {
        return MySqlError(0, "", operation + " failed: MySQL statement handle is null");
    }
    return MySqlError(
        mysql_stmt_errno(statement),
        mysql_stmt_sqlstate(statement),
        operation + " failed: " + mysql_stmt_error(statement));
}

template <typename Integer>
Integer parseInteger(const std::string& value, const std::string& column)
{
    Integer result {};
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto [position, error] = std::from_chars(begin, end, result);
    if (error != std::errc {} || position != end)
    {
        throw std::runtime_error("column '" + column + "' is not a valid integer");
    }
    return result;
}

struct ParameterBindings
{
    explicit ParameterBindings(const SqlParams& params)
        : bindings(params.size()),
          lengths(params.size()),
          nullFlags(params.size())
    {
        for (std::size_t index = 0; index < params.size(); ++index)
        {
            MYSQL_BIND& binding = bindings[index];
            std::memset(&binding, 0, sizeof(binding));

            std::visit(
                [&](const auto& value)
                {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Value, std::nullptr_t>)
                    {
                        nullFlags[index] = 1;
                        binding.buffer_type = MYSQL_TYPE_NULL;
                        binding.is_null = &nullFlags[index];
                    }
                    else if constexpr (std::is_same_v<Value, std::string>)
                    {
                        lengths[index] = static_cast<unsigned long>(value.size());
                        binding.buffer_type = MYSQL_TYPE_STRING;
                        binding.buffer = const_cast<char*>(value.data());
                        binding.buffer_length = lengths[index];
                        binding.length = &lengths[index];
                    }
                    else if constexpr (std::is_same_v<Value, std::int64_t>)
                    {
                        binding.buffer_type = MYSQL_TYPE_LONGLONG;
                        binding.buffer = const_cast<std::int64_t*>(&value);
                        binding.is_unsigned = 0;
                    }
                    else if constexpr (std::is_same_v<Value, std::uint64_t>)
                    {
                        binding.buffer_type = MYSQL_TYPE_LONGLONG;
                        binding.buffer = const_cast<std::uint64_t*>(&value);
                        binding.is_unsigned = 1;
                    }
                },
                params[index]);
        }
    }

    std::vector<MYSQL_BIND> bindings;
    std::vector<unsigned long> lengths;
    std::vector<my_bool> nullFlags;
};

} // namespace

bool ResultRow::hasColumn(const std::string& name) const
{
    return values_.find(name) != values_.end();
}

bool ResultRow::isNull(const std::string& name) const
{
    const auto iterator = values_.find(name);
    if (iterator == values_.end())
    {
        throw std::out_of_range("result column not found: " + name);
    }
    return !iterator->second.has_value();
}

std::optional<std::string> ResultRow::getOptionalString(const std::string& name) const
{
    const auto iterator = values_.find(name);
    if (iterator == values_.end())
    {
        throw std::out_of_range("result column not found: " + name);
    }
    return iterator->second;
}

std::string ResultRow::getString(const std::string& name) const
{
    const auto value = getOptionalString(name);
    if (!value.has_value())
    {
        throw std::runtime_error("result column is NULL: " + name);
    }
    return *value;
}

std::uint64_t ResultRow::getUInt64(const std::string& name) const
{
    return parseInteger<std::uint64_t>(getString(name), name);
}

std::uint32_t ResultRow::getUInt32(const std::string& name) const
{
    const std::uint64_t value = getUInt64(name);
    if (value > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::out_of_range("result column exceeds uint32 range: " + name);
    }
    return static_cast<std::uint32_t>(value);
}

std::int32_t ResultRow::getInt32(const std::string& name) const
{
    return parseInteger<std::int32_t>(getString(name), name);
}

PreparedStatement::PreparedStatement(MySqlConnection& connection, const std::string& sql)
{
    connection.ensureConnected();
    statement_ = mysql_stmt_init(connection.handle_);
    if (statement_ == nullptr)
    {
        throw std::runtime_error("mysql_stmt_init failed");
    }

    my_bool updateMaxLength = 1;
    if (mysql_stmt_attr_set(statement_, STMT_ATTR_UPDATE_MAX_LENGTH, &updateMaxLength) != 0)
    {
        const auto error = statementError(statement_, "setting statement attributes");
        mysql_stmt_close(statement_);
        statement_ = nullptr;
        throw error;
    }

    if (mysql_stmt_prepare(statement_, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
    {
        const auto error = statementError(statement_, "preparing statement");
        mysql_stmt_close(statement_);
        statement_ = nullptr;
        throw error;
    }
}

PreparedStatement::~PreparedStatement()
{
    if (statement_ != nullptr)
    {
        mysql_stmt_close(statement_);
    }
}

ExecuteResult PreparedStatement::execute(const SqlParams& params)
{
    resetAndValidateParameterCount(params);
    ParameterBindings bindings(params);
    if (!bindings.bindings.empty()
        && mysql_stmt_bind_param(statement_, bindings.bindings.data()) != 0)
    {
        throw statementError(statement_, "binding prepared statement parameters");
    }

    if (mysql_stmt_execute(statement_) != 0)
    {
        throw statementError(statement_, "executing prepared statement");
    }

    return ExecuteResult {
        static_cast<std::uint64_t>(mysql_stmt_insert_id(statement_)),
        static_cast<std::uint64_t>(mysql_stmt_affected_rows(statement_))
    };
}

ResultRows PreparedStatement::query(const SqlParams& params)
{
    resetAndValidateParameterCount(params);
    ParameterBindings parameterBindings(params);
    if (!parameterBindings.bindings.empty()
        && mysql_stmt_bind_param(statement_, parameterBindings.bindings.data()) != 0)
    {
        throw statementError(statement_, "binding prepared statement parameters");
    }

    if (mysql_stmt_execute(statement_) != 0)
    {
        throw statementError(statement_, "executing prepared query");
    }

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(
        mysql_stmt_result_metadata(statement_), &mysql_free_result);
    if (metadata == nullptr)
    {
        throw statementError(statement_, "reading prepared query metadata");
    }

    if (mysql_stmt_store_result(statement_) != 0)
    {
        throw statementError(statement_, "buffering prepared query result");
    }

    try
    {
        const unsigned int fieldCount = mysql_num_fields(metadata.get());
        MYSQL_FIELD* fields = mysql_fetch_fields(metadata.get());

        std::vector<std::vector<char>> buffers(fieldCount);
        std::vector<unsigned long> lengths(fieldCount);
        std::vector<my_bool> nullFlags(fieldCount);
        std::vector<my_bool> errorFlags(fieldCount);
        std::vector<MYSQL_BIND> resultBindings(fieldCount);

        for (unsigned int index = 0; index < fieldCount; ++index)
        {
            const std::size_t bufferSize =
                std::max<std::size_t>(1, static_cast<std::size_t>(fields[index].max_length) + 1);
            buffers[index].resize(bufferSize);

            MYSQL_BIND& binding = resultBindings[index];
            std::memset(&binding, 0, sizeof(binding));
            binding.buffer_type = MYSQL_TYPE_STRING;
            binding.buffer = buffers[index].data();
            binding.buffer_length = static_cast<unsigned long>(buffers[index].size());
            binding.length = &lengths[index];
            binding.is_null = &nullFlags[index];
            binding.error = &errorFlags[index];
        }

        if (fieldCount > 0 && mysql_stmt_bind_result(statement_, resultBindings.data()) != 0)
        {
            throw statementError(statement_, "binding prepared query result");
        }

        ResultRows rows;
        while (true)
        {
            const int fetchResult = mysql_stmt_fetch(statement_);
            if (fetchResult == MYSQL_NO_DATA)
            {
                break;
            }
            if (fetchResult == MYSQL_DATA_TRUNCATED)
            {
                throw std::runtime_error("prepared query result was unexpectedly truncated");
            }
            if (fetchResult != 0)
            {
                throw statementError(statement_, "fetching prepared query result");
            }

            ResultRow row;
            for (unsigned int index = 0; index < fieldCount; ++index)
            {
                if (nullFlags[index] != 0)
                {
                    row.values_.emplace(fields[index].name, std::nullopt);
                }
                else
                {
                    row.values_.emplace(
                        fields[index].name,
                        std::string(buffers[index].data(), lengths[index]));
                }
            }
            rows.push_back(std::move(row));
        }

        mysql_stmt_free_result(statement_);
        return rows;
    }
    catch (...)
    {
        mysql_stmt_free_result(statement_);
        throw;
    }
}

void PreparedStatement::resetAndValidateParameterCount(const SqlParams& params)
{
    if (mysql_stmt_reset(statement_) != 0)
    {
        throw statementError(statement_, "resetting prepared statement");
    }

    const unsigned long expectedCount = mysql_stmt_param_count(statement_);
    if (expectedCount != params.size())
    {
        throw std::invalid_argument(
            "prepared statement expected " + std::to_string(expectedCount) + " parameters, got "
            + std::to_string(params.size()));
    }
}

} // namespace campus::db
