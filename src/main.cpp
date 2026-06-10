#include "app/CliApp.h"
#include "app/WebApp.h"
#include "db/MySqlConnection.h"
#include "db/Transaction.h"
#include "utils/Config.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[])
{
    enum class RunMode
    {
        Web,
        Cli,
        CheckConfig
    };

    RunMode mode = RunMode::Web;
    bool checkConfigOnly = false;
    std::filesystem::path configPath = "config/config.json";

    if (argc == 2)
    {
        if (std::string_view(argv[1]) == "--cli")
        {
            mode = RunMode::Cli;
        }
        else
        {
            configPath = argv[1];
        }
    }
    else if (argc == 3 && std::string_view(argv[1]) == "--cli")
    {
        mode = RunMode::Cli;
        configPath = argv[2];
    }
    else if (argc == 3 && std::string_view(argv[1]) == "--check-config")
    {
        mode = RunMode::CheckConfig;
        checkConfigOnly = true;
        configPath = argv[2];
    }
    else if (argc > 1)
    {
        std::cerr << "Usage: " << argv[0] << " [config_path]\n"
                  << "       " << argv[0] << " --cli [config_path]\n"
                  << "       " << argv[0] << " --check-config <config_path>\n";
        return 2;
    }

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

        if (checkConfigOnly)
        {
            std::cout << "[INFO] config validation successful" << '\n';
            return 0;
        }

        campus::db::MySqlConnection connection;
        connection.connect(config);
        connection.ping();

        const std::string selectOne = connection.queryScalar("SELECT 1");
        if (selectOne != "1")
        {
            throw std::runtime_error("unexpected SELECT 1 result: " + selectOne);
        }

        std::cout << "[INFO] MySQL connection verified with SELECT 1" << '\n';

        {
            campus::db::Transaction transaction(connection);
            const std::string txCheck = connection.queryScalar("SELECT 1");
            if (txCheck != "1")
            {
                throw std::runtime_error("unexpected transaction check result: " + txCheck);
            }
            transaction.commit();
        }

        std::cout << "[INFO] MySQL transaction wrapper verified" << '\n';
        if (mode == RunMode::Cli)
        {
            campus::app::CliApp app(connection, std::cin, std::cout);
            return app.run();
        }

        campus::app::WebApp app(connection, config, CAMPUS_WEB_ROOT);
        return app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "[ERROR] startup failed: " << error.what() << '\n';
        return 1;
    }
}
