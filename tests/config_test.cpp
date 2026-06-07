#include "utils/Config.h"

#include <cassert>
#include <filesystem>

int main()
{
    const std::filesystem::path configPath = CAMPUS_CONFIG_EXAMPLE_PATH;
    const auto config = campus::utils::Config::loadFromFile(configPath);

    assert(config.serverHost == "0.0.0.0");
    assert(config.serverPort == 8080);
    assert(config.mysqlHost == "127.0.0.1");
    assert(config.mysqlPort == 3306);
    assert(config.mysqlUser == "campus_app");
    assert(config.mysqlDatabase == "campus_flash_sale");
    assert(config.threadPoolSize == 8);
    assert(config.mysqlPoolSize == 8);
    return 0;
}
