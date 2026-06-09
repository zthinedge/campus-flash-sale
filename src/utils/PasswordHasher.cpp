#include "utils/PasswordHasher.h"

#include <openssl/evp.h>

#include <array>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace campus::utils
{

std::string PasswordHasher::sha256(std::string_view value)
{
    using Context = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    Context context(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!context)
    {
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1
        || EVP_DigestUpdate(context.get(), value.data(), value.size()) != 1)
    {
        throw std::runtime_error("SHA-256 initialization failed");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest {};
    unsigned int digestLength = 0;
    if (EVP_DigestFinal_ex(context.get(), digest.data(), &digestLength) != 1)
    {
        throw std::runtime_error("SHA-256 finalization failed");
    }

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < digestLength; ++index)
    {
        output << std::setw(2) << static_cast<unsigned int>(digest[index]);
    }
    return output.str();
}

} // namespace campus::utils
