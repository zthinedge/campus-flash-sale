#include "db/Transaction.h"
#include "utils/Config.h"

#include <cassert>
#include <filesystem>
#include <string>

int main(int argc, char* argv[])
{
    assert(argc == 2);

    const auto config = campus::utils::Config::loadFromFile(std::filesystem::path(argv[1]));

    campus::db::MySqlConnection connection;
    connection.connect(config);

    const std::string suffix = connection.queryScalar("SELECT REPLACE(UUID(), '-', '')");
    const std::string rollbackUser = "tx_rollback_" + suffix.substr(0, 16);
    const std::string commitUser = "tx_commit_" + suffix.substr(0, 18);

    {
        campus::db::Transaction transaction(connection);
        connection.execute("INSERT INTO users (username, password_hash, role, status) VALUES ('"
                           + rollbackUser + "', 'test', 'USER', 'ACTIVE')");
    }

    assert(connection.queryScalar("SELECT COUNT(*) FROM users WHERE username = '" + rollbackUser + "'")
           == "0");

    {
        campus::db::Transaction transaction(connection);
        connection.execute("INSERT INTO users (username, password_hash, role, status) VALUES ('"
                           + commitUser + "', 'test', 'USER', 'ACTIVE')");
        transaction.commit();
    }

    assert(connection.queryScalar("SELECT COUNT(*) FROM users WHERE username = '" + commitUser + "'")
           == "1");

    connection.execute("DELETE FROM users WHERE username = '" + commitUser + "'");
    return 0;
}
