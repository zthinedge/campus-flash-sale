#pragma once

#include <string>
#include <string_view>

namespace campus::utils
{

class PasswordHasher
{
public:
    static std::string sha256(std::string_view value);
};

} // namespace campus::utils
