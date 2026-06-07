#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace campus::utils
{

struct Config
{
    std::string serverHost;
    std::uint16_t serverPort {};
    std::string mysqlHost;
    std::uint16_t mysqlPort {};
    std::string mysqlUser;
    std::string mysqlPassword;
    std::string mysqlDatabase;
    std::size_t threadPoolSize {};
    std::size_t mysqlPoolSize {};

    static Config loadFromFile(const std::filesystem::path& path);
};

} // namespace campus::utils
