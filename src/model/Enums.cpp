#include "model/Enums.h"

#include <stdexcept>
#include <string>

namespace campus::model
{

namespace
{

[[noreturn]] void throwUnknown(const char* type, std::string_view value)
{
    throw std::invalid_argument("unknown " + std::string(type) + ": " + std::string(value));
}

} // namespace

std::string_view toString(UserRole value)
{
    return value == UserRole::Admin ? "ADMIN" : "USER";
}

std::string_view toString(UserStatus value)
{
    return value == UserStatus::Disabled ? "DISABLED" : "ACTIVE";
}

std::string_view toString(ProductStatus value)
{
    switch (value)
    {
    case ProductStatus::OnSale:
        return "ON_SALE";
    case ProductStatus::InFlashSale:
        return "IN_FLASH_SALE";
    case ProductStatus::Sold:
        return "SOLD";
    case ProductStatus::OffShelf:
        return "OFF_SHELF";
    }
    throw std::invalid_argument("invalid ProductStatus value");
}

std::string_view toString(ActivityStatus value)
{
    switch (value)
    {
    case ActivityStatus::Pending:
        return "PENDING";
    case ActivityStatus::Active:
        return "ACTIVE";
    case ActivityStatus::Ended:
        return "ENDED";
    case ActivityStatus::Canceled:
        return "CANCELED";
    }
    throw std::invalid_argument("invalid ActivityStatus value");
}

std::string_view toString(OrderStatus value)
{
    return value == OrderStatus::Canceled ? "CANCELED" : "CREATED";
}

std::string_view toString(InventoryReason value)
{
    switch (value)
    {
    case InventoryReason::FlashSaleDeduct:
        return "FLASH_SALE_DEDUCT";
    case InventoryReason::OrderCancelRestore:
        return "ORDER_CANCEL_RESTORE";
    case InventoryReason::AdminAdjust:
        return "ADMIN_ADJUST";
    }
    throw std::invalid_argument("invalid InventoryReason value");
}

UserRole userRoleFromString(std::string_view value)
{
    if (value == "USER")
    {
        return UserRole::User;
    }
    if (value == "ADMIN")
    {
        return UserRole::Admin;
    }
    throwUnknown("UserRole", value);
}

UserStatus userStatusFromString(std::string_view value)
{
    if (value == "ACTIVE")
    {
        return UserStatus::Active;
    }
    if (value == "DISABLED")
    {
        return UserStatus::Disabled;
    }
    throwUnknown("UserStatus", value);
}

ProductStatus productStatusFromString(std::string_view value)
{
    if (value == "ON_SALE")
    {
        return ProductStatus::OnSale;
    }
    if (value == "IN_FLASH_SALE")
    {
        return ProductStatus::InFlashSale;
    }
    if (value == "SOLD")
    {
        return ProductStatus::Sold;
    }
    if (value == "OFF_SHELF")
    {
        return ProductStatus::OffShelf;
    }
    throwUnknown("ProductStatus", value);
}

ActivityStatus activityStatusFromString(std::string_view value)
{
    if (value == "PENDING")
    {
        return ActivityStatus::Pending;
    }
    if (value == "ACTIVE")
    {
        return ActivityStatus::Active;
    }
    if (value == "ENDED")
    {
        return ActivityStatus::Ended;
    }
    if (value == "CANCELED")
    {
        return ActivityStatus::Canceled;
    }
    throwUnknown("ActivityStatus", value);
}

OrderStatus orderStatusFromString(std::string_view value)
{
    if (value == "CREATED")
    {
        return OrderStatus::Created;
    }
    if (value == "CANCELED")
    {
        return OrderStatus::Canceled;
    }
    throwUnknown("OrderStatus", value);
}

InventoryReason inventoryReasonFromString(std::string_view value)
{
    if (value == "FLASH_SALE_DEDUCT")
    {
        return InventoryReason::FlashSaleDeduct;
    }
    if (value == "ORDER_CANCEL_RESTORE")
    {
        return InventoryReason::OrderCancelRestore;
    }
    if (value == "ADMIN_ADJUST")
    {
        return InventoryReason::AdminAdjust;
    }
    throwUnknown("InventoryReason", value);
}

} // namespace campus::model
