#pragma once

#include <stdexcept>
#include <string>

namespace campus::service
{

enum class ServiceErrorCode
{
    InvalidArgument,
    UsernameTaken,
    InvalidCredentials,
    UserNotFound,
    UserDisabled,
    Forbidden,
    ProductNotFound,
    ActivityNotFound,
    ActivityUnavailable,
    SoldOut,
    DuplicateOrder
};

class ServiceError : public std::runtime_error
{
public:
    ServiceError(ServiceErrorCode code, std::string message);

    ServiceErrorCode code() const noexcept;

private:
    ServiceErrorCode code_;
};

} // namespace campus::service
