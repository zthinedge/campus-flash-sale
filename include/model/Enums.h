#pragma once

#include <string_view>

namespace campus::model
{

enum class UserRole
{
    User,
    Admin
};

enum class UserStatus
{
    Active,
    Disabled
};

enum class ProductStatus
{
    OnSale,
    InFlashSale,
    Sold,
    OffShelf
};

enum class ActivityStatus
{
    Pending,
    Active,
    Ended,
    Canceled
};

enum class OrderStatus
{
    Created,
    Canceled
};

enum class InventoryReason
{
    FlashSaleDeduct,
    OrderCancelRestore,
    AdminAdjust
};

std::string_view toString(UserRole value);
std::string_view toString(UserStatus value);
std::string_view toString(ProductStatus value);
std::string_view toString(ActivityStatus value);
std::string_view toString(OrderStatus value);
std::string_view toString(InventoryReason value);

UserRole userRoleFromString(std::string_view value);
UserStatus userStatusFromString(std::string_view value);
ProductStatus productStatusFromString(std::string_view value);
ActivityStatus activityStatusFromString(std::string_view value);
OrderStatus orderStatusFromString(std::string_view value);
InventoryReason inventoryReasonFromString(std::string_view value);

} // namespace campus::model
