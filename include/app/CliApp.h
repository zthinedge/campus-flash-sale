#pragma once

#include "db/MySqlConnection.h"
#include "model/User.h"

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

namespace campus::app
{

class CliApp
{
public:
    CliApp(db::MySqlConnection& connection, std::istream& input, std::ostream& output);

    int run();

private:
    void showGuestMenu();
    void showUserMenu();
    void registerUser();
    void login();
    void logout();
    void listProducts();
    void showProductDetails();
    void listActivities();
    void publishProduct();
    void purchase();
    void listMyOrders();
    void createActivity();
    void showAdminSummary();

    std::string readLine(const std::string& prompt);
    std::uint64_t readUInt64(const std::string& prompt);
    std::uint32_t readUInt32(const std::string& prompt);
    bool isAdmin() const;

    db::MySqlConnection& connection_;
    std::istream& input_;
    std::ostream& output_;
    std::optional<model::User> currentUser_;
    bool running_ { true };
};

} // namespace campus::app
