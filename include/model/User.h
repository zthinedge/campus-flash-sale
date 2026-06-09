#pragma once

#include "model/Enums.h"

#include <cstdint>
#include <string>

namespace campus::model
{

struct User
{
    std::uint64_t id {};
    std::string username;
    std::string passwordHash;
    UserRole role { UserRole::User };
    UserStatus status { UserStatus::Active };
    std::string createdAt;
    std::string updatedAt;
};

struct CreateUser
{
    std::string username;
    std::string passwordHash;
    UserRole role { UserRole::User };
    UserStatus status { UserStatus::Active };
};

} // namespace campus::model
