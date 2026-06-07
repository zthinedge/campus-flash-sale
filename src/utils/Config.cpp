#include "utils/Config.h"

#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace campus::utils
{

namespace
{

using Json = nlohmann::json;

std::string readRequiredString(const Json& json, const char* key)
{
    if (!json.contains(key) || !json.at(key).is_string())
    {
        throw std::runtime_error(std::string("config field '") + key + "' must be a string");
    }

    std::string value = json.at(key).get<std::string>();
    if (value.empty())
    {
        throw std::runtime_error(std::string("config field '") + key + "' must not be empty");
    }
    return value;
}

std::uint16_t readPort(const Json& json, const char* key)
{
    if (!json.contains(key) || !json.at(key).is_number_unsigned())
    {
        throw std::runtime_error(std::string("config field '") + key
                                 + "' must be an unsigned integer");
    }

    const auto value = json.at(key).get<std::uint64_t>();
    if (value == 0 || value > std::numeric_limits<std::uint16_t>::max())
    {
        throw std::runtime_error(std::string("config field '") + key
                                 + "' must be between 1 and 65535");
    }
    return static_cast<std::uint16_t>(value);
}

std::size_t readPoolSize(const Json& json, const char* key)
{
    if (!json.contains(key) || !json.at(key).is_number_unsigned())
    {
        throw std::runtime_error(std::string("config field '") + key
                                 + "' must be an unsigned integer");
    }

    const auto value = json.at(key).get<std::uint64_t>();
    if (value == 0 || value > 1024)
    {
        throw std::runtime_error(std::string("config field '") + key
                                 + "' must be between 1 and 1024");
    }
    return static_cast<std::size_t>(value);
}

} // namespace

Config Config::loadFromFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("cannot open config file: " + path.string());
    }

    Json json;
    try
    {
        input >> json;
    }
    catch (const Json::parse_error& error)
    {
        throw std::runtime_error("invalid JSON in config file '" + path.string()
                                 + "': " + error.what());
    }

    if (!json.is_object())
    {
        throw std::runtime_error("config root must be a JSON object");
    }

    Config config;
    config.serverHost = readRequiredString(json, "server_host");
    config.serverPort = readPort(json, "server_port");
    config.mysqlHost = readRequiredString(json, "mysql_host");
    config.mysqlPort = readPort(json, "mysql_port");
    config.mysqlUser = readRequiredString(json, "mysql_user");
    config.mysqlPassword = readRequiredString(json, "mysql_password");
    config.mysqlDatabase = readRequiredString(json, "mysql_database");
    config.threadPoolSize = readPoolSize(json, "thread_pool_size");
    config.mysqlPoolSize = readPoolSize(json, "mysql_pool_size");
    return config;
}

} // namespace campus::utils
