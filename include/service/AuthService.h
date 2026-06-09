#pragma once

#include "db/MySqlConnection.h"
#include "model/User.h"

#include <cstdint>
#include <string_view>

namespace campus::service
{

class AuthService
{
public:
    explicit AuthService(db::MySqlConnection& connection);

    std::uint64_t registerUser(std::string_view username, std::string_view password);
    model::User login(std::string_view username, std::string_view password);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::service
