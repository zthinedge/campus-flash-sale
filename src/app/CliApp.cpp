#include "app/CliApp.h"

#include "model/Enums.h"
#include "service/ActivityService.h"
#include "service/AdminService.h"
#include "service/AuthService.h"
#include "service/FlashSaleService.h"
#include "service/OrderService.h"
#include "service/ProductService.h"
#include "service/ServiceError.h"

#include <charconv>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace campus::app
{

namespace
{

std::string serviceErrorName(service::ServiceErrorCode code)
{
    switch (code)
    {
    case service::ServiceErrorCode::InvalidArgument:
        return "INVALID_ARGUMENT";
    case service::ServiceErrorCode::UsernameTaken:
        return "USERNAME_TAKEN";
    case service::ServiceErrorCode::InvalidCredentials:
        return "INVALID_CREDENTIALS";
    case service::ServiceErrorCode::UserNotFound:
        return "USER_NOT_FOUND";
    case service::ServiceErrorCode::UserDisabled:
        return "USER_DISABLED";
    case service::ServiceErrorCode::Forbidden:
        return "FORBIDDEN";
    case service::ServiceErrorCode::ProductNotFound:
        return "PRODUCT_NOT_FOUND";
    case service::ServiceErrorCode::ActivityNotFound:
        return "ACTIVITY_NOT_FOUND";
    case service::ServiceErrorCode::ActivityUnavailable:
        return "ACTIVITY_UNAVAILABLE";
    case service::ServiceErrorCode::SoldOut:
        return "SOLD_OUT";
    case service::ServiceErrorCode::DuplicateOrder:
        return "DUPLICATE_ORDER";
    }
    return "UNKNOWN";
}

template <typename Integer>
Integer parseUnsigned(const std::string& input)
{
    Integer value {};
    const auto [position, error] =
        std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc {} || position != input.data() + input.size())
    {
        throw std::invalid_argument("expected an unsigned integer");
    }
    return value;
}

} // namespace

CliApp::CliApp(db::MySqlConnection& connection, std::istream& input, std::ostream& output)
    : connection_(connection),
      input_(input),
      output_(output)
{
}

int CliApp::run()
{
    output_ << "\n=== Campus Flash Sale CLI ===\n";
    while (running_ && input_)
    {
        try
        {
            if (currentUser_.has_value())
            {
                showUserMenu();
            }
            else
            {
                showGuestMenu();
            }
        }
        catch (const service::ServiceError& error)
        {
            output_ << "[BUSINESS ERROR] " << serviceErrorName(error.code()) << ": "
                    << error.what() << "\n";
        }
        catch (const std::exception& error)
        {
            output_ << "[ERROR] " << error.what() << "\n";
        }
    }
    return 0;
}

void CliApp::showGuestMenu()
{
    output_ << "\nGuest menu\n"
            << "  1. Register\n"
            << "  2. Login\n"
            << "  3. List products\n"
            << "  4. Product details\n"
            << "  5. List active activities\n"
            << "  0. Exit\n";

    const std::string choice = readLine("Select: ");
    if (choice == "1")
    {
        registerUser();
    }
    else if (choice == "2")
    {
        login();
    }
    else if (choice == "3")
    {
        listProducts();
    }
    else if (choice == "4")
    {
        showProductDetails();
    }
    else if (choice == "5")
    {
        listActivities();
    }
    else if (choice == "0")
    {
        output_ << "Bye.\n";
        running_ = false;
    }
    else
    {
        output_ << "Unknown menu option.\n";
    }
}

void CliApp::showUserMenu()
{
    output_ << "\nSigned in as " << currentUser_->username << " ["
            << model::toString(currentUser_->role) << "]\n"
            << "  1. List products\n"
            << "  2. Product details\n"
            << "  3. List active activities\n"
            << "  4. Publish product\n"
            << "  5. Purchase activity\n"
            << "  6. My orders\n";

    if (isAdmin())
    {
        output_ << "  7. Create flash-sale activity\n"
                << "  8. Activity summary\n";
    }

    output_ << "  9. Logout\n"
            << "  0. Exit\n";

    const std::string choice = readLine("Select: ");
    if (choice == "1")
    {
        listProducts();
    }
    else if (choice == "2")
    {
        showProductDetails();
    }
    else if (choice == "3")
    {
        listActivities();
    }
    else if (choice == "4")
    {
        publishProduct();
    }
    else if (choice == "5")
    {
        purchase();
    }
    else if (choice == "6")
    {
        listMyOrders();
    }
    else if (choice == "7" && isAdmin())
    {
        createActivity();
    }
    else if (choice == "8" && isAdmin())
    {
        showAdminSummary();
    }
    else if (choice == "9")
    {
        logout();
    }
    else if (choice == "0")
    {
        output_ << "Bye.\n";
        running_ = false;
    }
    else
    {
        output_ << "Unknown or unauthorized menu option.\n";
    }
}

void CliApp::registerUser()
{
    const std::string username = readLine("Username: ");
    const std::string password = readLine("Password: ");
    service::AuthService authService(connection_);
    const std::uint64_t userId = authService.registerUser(username, password);
    output_ << "Registered user id=" << userId << ". You can now log in.\n";
}

void CliApp::login()
{
    const std::string username = readLine("Username: ");
    const std::string password = readLine("Password: ");
    service::AuthService authService(connection_);
    currentUser_ = authService.login(username, password);
    output_ << "Login successful.\n";
}

void CliApp::logout()
{
    currentUser_.reset();
    output_ << "Logged out.\n";
}

void CliApp::listProducts()
{
    service::ProductService productService(connection_);
    const auto products = productService.listVisible();

    output_ << "\nProducts (" << products.size() << ")\n";
    for (const auto& product : products)
    {
        output_ << "  id=" << product.id << " no=" << product.productNo << " seller="
                << product.sellerId << " price=" << product.originalPriceCents
                << " status=" << model::toString(product.status) << " title=" << product.title
                << "\n";
    }
}

void CliApp::showProductDetails()
{
    const std::uint64_t productId = readUInt64("Product id: ");
    service::ProductService productService(connection_);
    const auto product = productService.getById(productId);

    output_ << "\nProduct details\n"
            << "  id=" << product.id << "\n"
            << "  number=" << product.productNo << "\n"
            << "  seller=" << product.sellerId << "\n"
            << "  title=" << product.title << "\n"
            << "  description=" << product.description << "\n"
            << "  original_price_cents=" << product.originalPriceCents << "\n"
            << "  status=" << model::toString(product.status) << "\n"
            << "  created_at=" << product.createdAt << "\n";
}

void CliApp::listActivities()
{
    service::ActivityService activityService(connection_);
    const auto activities = activityService.listActive();

    output_ << "\nActive activities (" << activities.size() << ")\n";
    for (const auto& activity : activities)
    {
        output_ << "  id=" << activity.id << " no=" << activity.activityNo
                << " product=" << activity.productId << " price=" << activity.flashPriceCents
                << " stock=" << activity.availableStock << "/" << activity.totalStock
                << " end=" << activity.endTime << "\n";
    }
}

void CliApp::publishProduct()
{
    const std::string productNo = readLine("Product number: ");
    const std::string title = readLine("Title: ");
    const std::string description = readLine("Description: ");
    const std::uint32_t price = readUInt32("Original price in cents: ");

    service::ProductService productService(connection_);
    const auto product = productService.publish(
        currentUser_->id,
        service::PublishProductRequest { productNo, title, description, price });
    output_ << "Published product id=" << product.id << ".\n";
}

void CliApp::purchase()
{
    const std::uint64_t activityId = readUInt64("Activity id: ");
    service::FlashSaleService flashSaleService(connection_);
    const auto result = flashSaleService.purchase(currentUser_->id, activityId);
    output_ << "Purchase succeeded. order=" << result.order.orderNo
            << " remaining_stock=" << result.remainingStock << "\n";
}

void CliApp::listMyOrders()
{
    service::OrderService orderService(connection_);
    const auto orders = orderService.listMyOrders(currentUser_->id);

    output_ << "\nMy orders (" << orders.size() << ")\n";
    for (const auto& order : orders)
    {
        output_ << "  id=" << order.id << " no=" << order.orderNo
                << " activity=" << order.activityId << " amount=" << order.totalAmountCents
                << " status=" << model::toString(order.status)
                << " product=" << order.productTitleSnapshot << "\n";
    }
}

void CliApp::createActivity()
{
    const std::string activityNo = readLine("Activity number: ");
    const std::uint64_t productId = readUInt64("Product id: ");
    const std::uint32_t flashPrice = readUInt32("Flash price in cents: ");
    const std::uint32_t stock = readUInt32("Stock: ");
    const std::string startTime = readLine("Start time (YYYY-MM-DD HH:MM:SS.mmm): ");
    const std::string endTime = readLine("End time (YYYY-MM-DD HH:MM:SS.mmm): ");

    service::ActivityService activityService(connection_);
    const auto activity = activityService.create(
        currentUser_->id,
        service::CreateActivityRequest {
            activityNo,
            productId,
            flashPrice,
            stock,
            startTime,
            endTime,
            model::ActivityStatus::Active
        });
    output_ << "Created activity id=" << activity.id << ".\n";
}

void CliApp::showAdminSummary()
{
    service::AdminService adminService(connection_);
    const auto summaries = adminService.listActivitySummaries(currentUser_->id);

    output_ << "\nActivity summary (" << summaries.size() << ")\n";
    for (const auto& summary : summaries)
    {
        output_ << "  id=" << summary.activity.id << " no=" << summary.activity.activityNo
                << " stock=" << summary.activity.availableStock << "/"
                << summary.activity.totalStock << " orders=" << summary.orderCount
                << " logs=" << summary.inventoryLogCount
                << " status=" << model::toString(summary.activity.status) << "\n";
    }
}

std::string CliApp::readLine(const std::string& prompt)
{
    output_ << prompt;
    output_.flush();

    std::string value;
    if (!std::getline(input_, value))
    {
        running_ = false;
        return {};
    }
    return value;
}

std::uint64_t CliApp::readUInt64(const std::string& prompt)
{
    const std::string value = readLine(prompt);
    return parseUnsigned<std::uint64_t>(value);
}

std::uint32_t CliApp::readUInt32(const std::string& prompt)
{
    const std::string value = readLine(prompt);
    const std::uint64_t parsed = parseUnsigned<std::uint64_t>(value);
    if (parsed > std::numeric_limits<std::uint32_t>::max())
    {
        throw std::out_of_range("number exceeds uint32 range");
    }
    return static_cast<std::uint32_t>(parsed);
}

bool CliApp::isAdmin() const
{
    return currentUser_.has_value() && currentUser_->role == model::UserRole::Admin;
}

} // namespace campus::app
