#include "app/CliApp.h"
#include "service/ProductService.h"
#include "utils/Config.h"

#include <cassert>
#include <filesystem>
#include <sstream>
#include <string>

int main(int argc, char* argv[])
{
    assert(argc == 2);

    const auto config = campus::utils::Config::loadFromFile(std::filesystem::path(argv[1]));
    campus::db::MySqlConnection connection;
    connection.connect(config);

    campus::service::ProductService productService(connection);
    const auto products = productService.listVisible();
    assert(!products.empty());

    std::istringstream input(
        "3\n4\n" + std::to_string(products.front().id) + "\n5\n0\n");
    std::ostringstream output;
    campus::app::CliApp app(connection, input, output);
    assert(app.run() == 0);

    const std::string text = output.str();
    assert(text.find("Campus Flash Sale CLI") != std::string::npos);
    assert(text.find("Products (") != std::string::npos);
    assert(text.find("Product details") != std::string::npos);
    assert(text.find("Active activities (") != std::string::npos);
    assert(text.find("Bye.") != std::string::npos);
    return 0;
}
