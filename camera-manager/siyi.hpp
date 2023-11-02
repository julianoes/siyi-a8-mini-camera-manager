#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


#include "siyi_crc.hpp"

namespace siyi {

std::ostream& operator<<(std::ostream& str, const std::vector<uint8_t>& bytes);

template<typename PayloadType>
class Payload {
public:
    std::vector<std::uint8_t> bytes() const {
        return static_cast<const PayloadType*>(this)->bytes_impl();
    }

    uint8_t cmd_id() const {
        return static_cast<const PayloadType*>(this)->cmd_id_impl();
    }

    friend std::ostream& operator<<(std::ostream& str, const Payload<PayloadType>& payload) {
        str << payload.bytes();
        return str;
    }
};

class FirmwareVersion : public Payload<FirmwareVersion> {
public:
    FirmwareVersion() = default;

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x01;
    }
};

class GimbalCenter : public Payload<GimbalCenter> {
public:
    GimbalCenter() = default;

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_center_pos);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x08;
    }

private:
    const uint8_t _center_pos{1};
};

class GimbalRotate : public Payload<GimbalRotate> {
public:
    GimbalRotate(int8_t turn_yaw, int8_t turn_pitch)
    : _turn_yaw(turn_yaw)
    , _turn_pitch(turn_pitch)
    {}

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_turn_yaw);
        result.push_back(_turn_pitch);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x07;
    }

private:
    const int8_t _turn_yaw;
    const int8_t _turn_pitch;
};

class TakePicture : public Payload<TakePicture> {
public:
    TakePicture() = default;

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_func_type);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x0C;
    }

private:
    const uint8_t _func_type{0};
};

class ToggleRecording : public Payload<ToggleRecording> {
public:
    ToggleRecording() = default;

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_func_type);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x0C;
    }

private:
    const uint8_t _func_type{2};
};

class StreamSettings : public Payload<StreamSettings> {
public:
    StreamSettings() = default;

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_stream_type);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x20;
    }

private:
    const uint8_t _stream_type{1}; // Set video stream
};

class StreamResolution : public Payload<StreamResolution> {
public:
    enum class Resolution {
        Res720,
        Res1080,
    };

    StreamResolution(Resolution resolution) {
        switch (resolution) {
            case Resolution::Res720:
                _resolution_l = 1280;
                _resolution_h = 720;
                break;
            case Resolution::Res1080:
                _resolution_l = 1920;
                _resolution_h = 1080;
                break;
        }
    }

    std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_stream_type);
        result.push_back(_video_enc_type);
        result.push_back(_resolution_l & 0xff);
        result.push_back((_resolution_l >> 8) & 0xff);
        result.push_back(_resolution_h & 0xff);
        result.push_back((_resolution_h >> 8) & 0xff);
        result.push_back(_video_bitrate & 0xff);
        result.push_back((_video_bitrate >> 8) & 0xff);
        result.push_back(_reserved);
        return result;
    }

    uint8_t cmd_id_impl() const {
        return 0x21;
    }

private:
    const uint8_t _stream_type{1}; // Set video stream
    const uint8_t _video_enc_type{2}; // H265
    uint16_t _resolution_l{0};
    uint16_t _resolution_h{0};
    const uint16_t _video_bitrate{4000};
    const uint8_t _reserved{0};
};

class Messager
{
public:
    Messager() = default;
    ~Messager()
    {
        close(_sockfd);
        _sockfd = -1;
    }

    bool setup(const std::string& ip, unsigned port)
    {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
            return false;
        }

        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);

        unsigned char buf[4];
        if (inet_pton(AF_INET, ip.c_str(), buf) != 1) {
            std::cerr << "Error converting IP address string to binary buffer: " << strerror(errno) << std::endl;
            return false;
        }

        _addr.sin_addr.s_addr = *reinterpret_cast<uint32_t *>(buf);

        return true;
    }

    bool send(const std::vector<uint8_t> message)
    {
        // Send the UDP packet
        const int sent = sendto(_sockfd, message.data(), message.size(), 0, (struct sockaddr *)&_addr, sizeof(_addr));
        if (sent < 0) {
            std::cerr << "Error sending UDP packet: " << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

    void receive()
    {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(_sockfd, &readfds);

        int ret = select(_sockfd+1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            std::cout << "Something should be available: " << std::endl;

            std::vector<uint8_t> buffer;
            buffer.resize(2048);
            ssize_t ret = recv(_sockfd, buffer.data(), buffer.size(), 0);
            if (ret == -1) {
                std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
                return;
            }

            buffer.resize(ret);

            std::cout << "Received: " << buffer << std::endl;

        } else if (ret == 0) {
            std::cout << "Timed out." << std::endl;

        } else {
                std::cerr << "Error with select: " << strerror(errno) << std::endl;
        }
    }

private:
    int _sockfd{-1};
    struct sockaddr_in _addr{};
};


class Serializer {
public:
    template<typename PayloadType>
    std::vector<uint8_t> assemble_message(const Payload<PayloadType>& payload)
    {
        const auto bytes = payload.bytes();
        std::vector<uint8_t> message;

        message.push_back(magic1);
        message.push_back(magic2);
        message.push_back(1); // need ack.

        message.push_back(bytes.size() & 0xff);
        message.push_back((bytes.size() >> 8) & 0xff);
        message.push_back(_next_seq & 0xff);
        message.push_back((_next_seq >> 8) & 0xff);
        ++_next_seq;
        message.push_back(payload.cmd_id());

        message.insert(std::end(message), std::begin(bytes), std::end(bytes));

        auto crc16 = crc16_cal(message.data(), message.size(), 0);
        message.push_back(crc16 & 0xff);
        message.push_back((crc16 >> 8) & 0xff);

        std::cout << "message is: " << message << std::endl;

        return message;
    }

private:
    static constexpr uint8_t magic1 = 0x55;
    static constexpr uint8_t magic2 = 0x66;
    std::uint16_t _next_seq{0};
};

} // namespace siyi
