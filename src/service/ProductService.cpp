#include "service/ProductService.h"

#include "dao/ProductDao.h"
#include "dao/UserDao.h"
#include "service/ServiceError.h"

namespace campus::service
{

ProductService::ProductService(db::MySqlConnection& connection)
    : connection_(connection)
{
}

model::Product ProductService::publish(
    std::uint64_t sellerId, const PublishProductRequest& request)
{
    if (request.productNo.empty() || request.title.empty() || request.originalPriceCents == 0)
    {
        throw ServiceError(ServiceErrorCode::InvalidArgument, "invalid product input");
    }

    dao::UserDao userDao(connection_);
    const auto seller = userDao.findById(sellerId);
    if (!seller.has_value())
    {
        throw ServiceError(ServiceErrorCode::UserNotFound, "seller does not exist");
    }
    if (seller->status != model::UserStatus::Active)
    {
        throw ServiceError(ServiceErrorCode::UserDisabled, "seller account is disabled");
    }

    dao::ProductDao productDao(connection_);
    const std::uint64_t productId = productDao.create(model::CreateProduct {
        request.productNo,
        sellerId,
        request.title,
        request.description,
        request.originalPriceCents,
        model::ProductStatus::OnSale
    });
    return *productDao.findById(productId);
}

model::Product ProductService::getById(std::uint64_t productId)
{
    dao::ProductDao productDao(connection_);
    const auto product = productDao.findById(productId);
    if (!product.has_value())
    {
        throw ServiceError(ServiceErrorCode::ProductNotFound, "product does not exist");
    }
    return *product;
}

std::vector<model::Product> ProductService::listVisible()
{
    dao::ProductDao productDao(connection_);
    return productDao.listVisible();
}

} // namespace campus::service
