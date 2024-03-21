#include "siyi_protocol.hpp"

namespace siyi {

std::ostream& operator<<(std::ostream& str, const std::vector<std::uint8_t>& bytes)
{
    std::ios old_state(nullptr);
    old_state.copyfmt(std::cout);

    for (const auto& byte : bytes) {
        str << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }

    std::cout.copyfmt(old_state);

    return str;
}

bool Messager::send(const std::vector<std::uint8_t>& message)
{
    // Send the UDP packet
    const ssize_t sent = sendto(_sockfd, message.data(), message.size(), 0, (struct sockaddr *)&_addr, sizeof(_addr));
    if (sent < 0) {
        std::cerr << "Error sending UDP packet: " << strerror(errno) << std::endl;
        return false;
    }
    // std::cerr << "Sent " << sent << std::endl;
    return true;
}

std::vector<std::uint8_t> Messager::receive() const
{
    std::vector<std::uint8_t> result;

    struct timeval tv{};
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(_sockfd, &read_fds);

    const int select_ret = select(_sockfd+1, &read_fds, nullptr, nullptr, &tv);
    if (select_ret > 0) {
        result.resize(2048);
        const ssize_t recv_ret = recv(_sockfd, result.data(), result.size(), 0);
        if (recv_ret == -1) {
            std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
            result.resize(0);
            return result;
        }

        result.resize(recv_ret);

        // std::cout << "Received: " << result << std::endl;

    } else if (select_ret == 0) {
        std::cout << "Timed out." << std::endl;

    } else {
        std::cerr << "Error with select: " << strerror(errno) << std::endl;
    }

    return result;
}

} // namespace siyi
