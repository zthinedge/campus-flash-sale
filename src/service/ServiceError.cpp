#include "service/ServiceError.h"

#include <utility>

namespace campus::service
{

ServiceError::ServiceError(ServiceErrorCode code, std::string message)
    : std::runtime_error(std::move(message)),
      code_(code)
{
}

ServiceErrorCode ServiceError::code() const noexcept
{
    return code_;
}

} // namespace campus::service
