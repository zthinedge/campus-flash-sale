#pragma once

#include "model/Enums.h"

#include <cstdint>
#include <string>

namespace campus::model
{

struct Product
{
    std::uint64_t id {};
    std::string productNo;
    std::uint64_t sellerId {};
    std::string title;
    std::string description;
    std::uint32_t originalPriceCents {};
    ProductStatus status { ProductStatus::OnSale };
    std::string createdAt;
    std::string updatedAt;
};

struct CreateProduct
{
    std::string productNo;
    std::uint64_t sellerId {};
    std::string title;
    std::string description;
    std::uint32_t originalPriceCents {};
    ProductStatus status { ProductStatus::OnSale };
};

} // namespace campus::model
