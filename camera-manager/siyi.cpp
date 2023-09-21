#include "siyi.hpp"

namespace siyi {

std::ostream& operator<<(std::ostream& str, const std::vector<uint8_t>& bytes)
{
    for (const auto& byte : bytes) {
        str << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    return str;
}


} // namespace siyi
