#include "siyi.hpp"

namespace siyi {

std::ostream& operator<<(std::ostream& str, const std::vector<uint8_t>& bytes)
{
    for (const auto& byte : bytes) {
        str << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    return str;
}

bool Messager::send(const std::vector<uint8_t>& message) {
    // Send the UDP packet
    const ssize_t sent = sendto(_sockfd, message.data(), message.size(), 0, (struct sockaddr *)&_addr, sizeof(_addr));
    if (sent < 0) {
        std::cerr << "Error sending UDP packet: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void Messager::receive() const {
    struct timeval tv{};
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(_sockfd, &read_fds);

    const int select_ret = select(_sockfd+1, &read_fds, nullptr, nullptr, &tv);
    if (select_ret > 0) {
        std::cout << "Something should be available: " << std::endl;

        std::vector<uint8_t> buffer;
        buffer.resize(2048);
        const ssize_t recv_ret = recv(_sockfd, buffer.data(), buffer.size(), 0);
        if (recv_ret == -1) {
            std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
            return;
        }

        buffer.resize(recv_ret);

        std::cout << "Received: " << buffer << std::endl;

    } else if (select_ret == 0) {
        std::cout << "Timed out." << std::endl;

    } else {
        std::cerr << "Error with select: " << strerror(errno) << std::endl;
    }
}

} // namespace siyi
