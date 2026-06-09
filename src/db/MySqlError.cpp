#include "db/MySqlError.h"

#include <utility>

namespace campus::db
{

MySqlError::MySqlError(unsigned int code, std::string sqlState, std::string message)
    : std::runtime_error(std::move(message)),
      code_(code),
      sqlState_(std::move(sqlState))
{
}

unsigned int MySqlError::code() const noexcept
{
    return code_;
}

const std::string& MySqlError::sqlState() const noexcept
{
    return sqlState_;
}

} // namespace campus::db
