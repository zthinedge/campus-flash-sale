#include "utils/Config.h"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
    const std::filesystem::path configPath =
        argc > 1 ? std::filesystem::path(argv[1]) : "config/config.json";

    try
    {
        const auto config = campus::utils::Config::loadFromFile(configPath);

        std::cout << "[INFO] campus-flash-sale starting..." << '\n'
                  << "[INFO] config loaded from " << configPath << '\n'
                  << "[INFO] server=" << config.serverHost << ':' << config.serverPort << '\n'
                  << "[INFO] mysql=" << config.mysqlUser << '@' << config.mysqlHost << ':'
                  << config.mysqlPort << '/' << config.mysqlDatabase << '\n'
                  << "[INFO] thread_pool_size=" << config.threadPoolSize
                  << ", mysql_pool_size=" << config.mysqlPoolSize << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[ERROR] startup failed: " << error.what() << '\n';
        return 1;
    }
}
