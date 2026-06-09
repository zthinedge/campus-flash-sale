#pragma once

#include <stdexcept>
#include <string>

namespace campus::db
{

class MySqlError : public std::runtime_error
{
public:
    MySqlError(unsigned int code, std::string sqlState, std::string message);

    unsigned int code() const noexcept;
    const std::string& sqlState() const noexcept;

private:
    unsigned int code_;
    std::string sqlState_;
};

} // namespace campus::db
