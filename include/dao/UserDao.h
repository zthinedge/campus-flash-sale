#pragma once

#include "db/MySqlConnection.h"
#include "model/User.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace campus::dao
{

class UserDao
{
public:
    explicit UserDao(db::MySqlConnection& connection);

    std::optional<model::User> findById(std::uint64_t id);
    std::optional<model::User> findByUsername(std::string_view username);
    std::uint64_t create(const model::CreateUser& input);

private:
    db::MySqlConnection& connection_;
};

} // namespace campus::dao
