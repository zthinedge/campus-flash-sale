#include "service/AuthService.h"

#include "dao/UserDao.h"
#include "db/MySqlError.h"
#include "service/ServiceError.h"
#include "utils/PasswordHasher.h"

#include <string>

namespace campus::service
{

namespace
{

void validateCredentials(std::string_view username, std::string_view password)
{
    if (username.size() < 3 || username.size() > 64)
    {
        throw ServiceError(
            ServiceErrorCode::InvalidArgument, "username length must be between 3 and 64");
    }
    if (password.size() < 4 || password.size() > 128)
    {
        throw ServiceError(
            ServiceErrorCode::InvalidArgument, "password length must be between 4 and 128");
    }
}

} // namespace

AuthService::AuthService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

std::uint64_t AuthService::registerUser(std::string_view username, std::string_view password)
{
    validateCredentials(username, password);

    dao::UserDao userDao(connection_);
    if (userDao.findByUsername(username).has_value())
    {
        throw ServiceError(ServiceErrorCode::UsernameTaken, "username already exists");
    }

    try
    {
        return userDao.create(model::CreateUser {
            std::string(username),
            utils::PasswordHasher::sha256(password),
            model::UserRole::User,
            model::UserStatus::Active
        });
    }
    catch (const db::MySqlError& error)
    {
        if (error.code() == 1062)
        {
            throw ServiceError(ServiceErrorCode::UsernameTaken, "username already exists");
        }
        throw;
    }
}

model::User AuthService::login(std::string_view username, std::string_view password)
{
    validateCredentials(username, password);

    dao::UserDao userDao(connection_);
    const auto user = userDao.findByUsername(username);
    if (!user.has_value() || user->passwordHash != utils::PasswordHasher::sha256(password))
    {
        throw ServiceError(
            ServiceErrorCode::InvalidCredentials, "invalid username or password");
    }
    if (user->status != model::UserStatus::Active)
    {
        throw ServiceError(ServiceErrorCode::UserDisabled, "user account is disabled");
    }
    return *user;
}

} // namespace campus::service
