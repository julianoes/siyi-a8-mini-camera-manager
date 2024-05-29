#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>


#include "siyi_crc.hpp"

namespace siyi {

std::ostream& operator<<(std::ostream& str, const std::vector<std::uint8_t>& bytes);

template<typename PayloadType>
class Payload {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes() const {
        return derived().bytes_impl();
    }

    [[nodiscard]] std::uint8_t cmd_id() const {
        return derived().cmd_id_impl();
    }

    friend std::ostream& operator<<(std::ostream& str, const Payload<PayloadType>& payload) {
        str << payload.bytes();
        return str;
    }

private:
    [[nodiscard]] PayloadType const& derived() const { return static_cast<PayloadType const&>(*this); }
};

class FirmwareVersion : public Payload<FirmwareVersion> {
public:
    static std::vector<std::uint8_t> bytes_impl() {
        std::vector<std::uint8_t> result;
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x01;
    }
};

class GimbalCenter : public Payload<GimbalCenter> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_center_pos);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x08;
    }

private:
    const std::uint8_t _center_pos{1};
};

class GimbalRotate : public Payload<GimbalRotate> {
public:
    GimbalRotate(int8_t turn_yaw, int8_t turn_pitch)
    : _turn_yaw(turn_yaw)
    , _turn_pitch(turn_pitch)
    {}

    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_turn_yaw);
        result.push_back(_turn_pitch);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x07;
    }

private:
    const int8_t _turn_yaw;
    const int8_t _turn_pitch;
};

class SetGimbalAttitude : public Payload<SetGimbalAttitude> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(yaw_t10 & 0xff);
        result.push_back((yaw_t10 >> 8) & 0xff);
        result.push_back(pitch_t10 & 0xff);
        result.push_back((pitch_t10 >> 8) & 0xff);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x0E;
    }

    std::int16_t yaw_t10;
    std::int16_t pitch_t10;
};

class TakePicture : public Payload<TakePicture> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_func_type);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x0C;
    }

private:
    const std::uint8_t _func_type{0};
};

class ToggleRecording : public Payload<ToggleRecording> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(_func_type);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x0C;
    }

private:
    const std::uint8_t _func_type{2};
};

class SetGimbalMode : public Payload<SetGimbalMode> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(static_cast<std::uint8_t>(mode));
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x0C;
    }

    enum class Mode : std::uint8_t {
        Lock = 3,
        Follow = 4,
        Fpv = 5,
    } mode{Mode::Follow};
};

class GetStreamSettings : public Payload<GetStreamSettings> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(stream_type);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x20;
    }

    std::uint8_t stream_type{0};
};

class StreamSettings : public Payload<StreamSettings> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(stream_type);
        result.push_back(video_enc_type);
        result.push_back(resolution_l & 0xff);
        result.push_back((resolution_l >> 8) & 0xff);
        result.push_back(resolution_h & 0xff);
        result.push_back((resolution_h >> 8) & 0xff);
        result.push_back(video_bitrate_kbps & 0xff);
        result.push_back((video_bitrate_kbps >> 8) & 0xff);
        result.push_back(_reserved);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x21;
    }

    std::uint8_t stream_type{0};
    std::uint8_t video_enc_type{};
    std::uint16_t resolution_l{0};
    std::uint16_t resolution_h{0};
    // Values from 2000 to 4000 seem to be accepted for 1080.
    // Value from 1570 to 4000 seem to be accepted for 720
    std::uint16_t video_bitrate_kbps{4000};

private:
    const std::uint8_t _reserved{0};
};

class ManualZoom : public Payload<ManualZoom> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(zoom);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x05;
    }

    std::int8_t zoom{};
};

class AbsoluteZoom : public Payload<AbsoluteZoom> {
public:
    [[nodiscard]] std::vector<std::uint8_t> bytes_impl() const {
        std::vector<std::uint8_t> result;
        result.push_back(absolute_movement_integer);
        result.push_back(absolute_movement_fractional);
        return result;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x0F;
    }

    std::uint8_t absolute_movement_integer{};
    std::uint8_t absolute_movement_fractional{};
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

    bool send(const std::vector<std::uint8_t>& message);

    [[nodiscard]] std::vector<std::uint8_t> receive() const;

private:
    int _sockfd{-1};
    struct sockaddr_in _addr{};
};

template<typename AckPayloadType>
class AckPayload {
public:
    [[nodiscard]] bool fill(const std::vector<std::uint8_t>& bytes) {
        return derived().fill_impl(bytes);
    }

    [[nodiscard]] std::uint8_t cmd_id() {
        return derived().cmd_id_impl();
    }

    friend std::ostream& operator<<(std::ostream& str, AckPayload<AckPayloadType>& ack_payload) {
        str << ack_payload.derived();
        return str;
    }

private:
    [[nodiscard]] AckPayloadType& derived() { return static_cast<AckPayloadType&>(*this); }
};

class AckFirmwareVersion : public AckPayload<AckFirmwareVersion> {
    // Note: zoom functionality is listed in the manual but not populated on the A8 mini
    public:
        bool fill_impl(const std::vector<std::uint8_t>& bytes) {

            if (bytes.size() != len) {
                std::cerr << "Length wrong: " << bytes.size() << " instead of " << len << std::endl;
                return false;
            }

            code_board_ver_major = bytes[2];
            code_board_ver_minor = bytes[1];
            code_board_ver_patch = bytes[0];
            gimbal_firmware_ver_major = bytes[6];
            gimbal_firmware_ver_minor = bytes[5];
            gimbal_firmware_ver_patch = bytes[4];

            static_assert(8 == len, "length is wrong");
            return true;
        }

        static std::uint8_t cmd_id_impl() {
            return 0x01;
        }

        friend std::ostream& operator<<(std::ostream& str, const AckFirmwareVersion& self) {
            str << "Camera version: "
                << int(self.code_board_ver_major) << '.'
                << int(self.code_board_ver_minor) << '.'
                << int(self.code_board_ver_patch) << '\n'
                << "Gimbal version: "
                << int(self.gimbal_firmware_ver_major) << '.'
                << int(self.gimbal_firmware_ver_minor) << '.'
                << int(self.gimbal_firmware_ver_patch) << std::endl;
            return str;
        }

        std::uint8_t code_board_ver_major{0};
        std::uint8_t code_board_ver_minor{0};
        std::uint8_t code_board_ver_patch{0};
        std::uint8_t gimbal_firmware_ver_major{0};
        std::uint8_t gimbal_firmware_ver_minor{0};
        std::uint8_t gimbal_firmware_ver_patch{0};
    private:

        static constexpr std::size_t len =
                sizeof(uint32_t) + sizeof(uint32_t);
    };

class AckGetStreamResolution : public AckPayload<AckGetStreamResolution> {
public:
    bool fill_impl(const std::vector<std::uint8_t>& bytes) {

        if (bytes.size() != len) {
            std::cerr << "Length wrong: " << bytes.size() << " instead of " << len << std::endl;
            return false;
        }

        _stream_type = bytes[0];
        video_enc_type = bytes[1];
        resolution_l = bytes[2] | (bytes[3] << 8);
        resolution_h = bytes[4] | (bytes[5] << 8);
        video_bitrate_kbps = bytes[6] | (bytes[7] << 8);
        _reserved = bytes[8];

        static_assert(9 == len, "length is wrong");
        return true;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x20;
    }

    friend std::ostream& operator<<(std::ostream& str, const AckGetStreamResolution& self) {
        str << "Resolution: " << self.resolution_l << "x" << self.resolution_h << '\n'
            << "Bitrate: " << self.video_bitrate_kbps << " kbps\n"
            << "Codec: " << (self.video_enc_type == 1 ? "H264" : "H265") << std::endl;
        return str;
    }

    std::uint8_t video_enc_type{0};
    std::uint16_t resolution_l{0};
    std::uint16_t resolution_h{0};
    std::uint16_t video_bitrate_kbps{0};

private:
    std::uint8_t _stream_type{0};
    std::uint8_t _reserved{0};

    static constexpr std::size_t len =
        sizeof(_stream_type) +
        sizeof(video_enc_type) +
        sizeof(resolution_l) +
        sizeof(resolution_h) +
        sizeof(video_bitrate_kbps) +
        sizeof(_reserved);
};

class AckSetStreamSettings : public AckPayload<AckSetStreamSettings> {
public:
    bool fill_impl(const std::vector<std::uint8_t>& bytes) {

        if (bytes.size() != len) {
            std::cerr << "Length wrong: " << bytes.size() << " instead of " << len << std::endl;
            return false;
        }

        _stream_type = bytes[0];
        result = bytes[1];

        static_assert(2 == len, "length is wrong");
        return true;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x21;
    }

    friend std::ostream& operator<<(std::ostream& str, const AckSetStreamSettings& self) {
        str << "Result: " << int(self.result) << '\n';
        return str;
    }

    std::uint8_t result{0};

private:
    std::uint8_t _stream_type{0};

    static constexpr std::size_t len =
            sizeof(_stream_type) +
            sizeof(result);
};

class AckManualZoom : public AckPayload<AckManualZoom> {
public:
    bool fill_impl(const std::vector<std::uint8_t>& bytes) {

        if (bytes.size() != len) {
            std::cerr << "Length wrong: " << bytes.size() << " instead of " << len << '\n';
            return false;
        }

        zoom_multiple = bytes[0] | (bytes[1] << 8);

        static_assert(2 == len, "length is wrong");
        return true;
    }

    static std::uint8_t cmd_id_impl() {
        return 0x05;
    }

    friend std::ostream& operator<<(std::ostream& str, const AckManualZoom& self) {
        str << "Zoom: " << int(self.zoom_multiple) << "x\n";
        return str;
    }

    std::uint16_t zoom_multiple{0};

private:
    static constexpr std::size_t len = sizeof(zoom_multiple);
};

class Serializer {
public:
    template<typename PayloadType>
    std::vector<std::uint8_t> assemble_message(const Payload<PayloadType>& payload)
    {
        const auto bytes = payload.bytes();
        std::vector<std::uint8_t> message;

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

        const auto crc16 = crc16_cal(message.data(), message.size());
        message.push_back(crc16 & 0xff);
        message.push_back((crc16 >> 8) & 0xff);

        return message;
    }

private:
    static constexpr std::uint8_t magic1 = 0x55;
    static constexpr std::uint8_t magic2 = 0x66;
    std::uint16_t _next_seq{0};
};

class Deserializer {
public:
    template<typename AckPayloadType>
    std::optional<AckPayloadType> disassemble_message(const std::vector<std::uint8_t>& message)
    {
        auto ack_payload = AckPayloadType{};

        if (message.size() < header_len + crc_len) {
            std::cerr << "message too short" << std::endl;
            return {};
        }

        if (message[0] != magic1) {
            std::cerr << "magic1 wrong" << std::endl;
            return {};
        }

        if (message[1] != magic2) {
            std::cerr << "magic2 wrong" << std::endl;
            return {};
        }

        if ((message[2] & 0x2) == 0) {
            std::cerr << "is not an ack package: " << std::endl;
            return {};
        }

        const std::uint8_t data_len = message[3] | (message[4] << 8);

        if (message.size() != static_cast<std::size_t>(data_len + header_len + crc_len)) {
            std::cerr << "wrong data len";
            return {};
        }

        // Ignore the sequence (5,6). We don't need it.

        const std::uint8_t cmd_id = message[7];

        if (ack_payload.cmd_id() != cmd_id) {
            std::cerr << "wrong cmd id: " << std::to_string(cmd_id) << " instead of " << std::to_string(ack_payload.cmd_id()) << std::endl;
            return {};
        }

        const auto crc16 = crc16_cal(message.data(), message.size() - crc_len);
        if ((crc16 & 0xff) != message[message.size()-2] || ((crc16 & 0xff00) >> 8) != message[message.size()-1]) {
            std::cerr << "crc failed";
            return {};
        }


        std::vector<std::uint8_t> payload_bytes{message.begin() + header_len, message.end() - crc_len};

        if (!ack_payload.fill(payload_bytes)) {
            return {};
        }

        return std::optional{ack_payload};
    }

private:
    static constexpr std::uint8_t magic1 = 0x55;
    static constexpr std::uint8_t magic2 = 0x66;
    static constexpr std::uint8_t header_len = 8;
    static constexpr std::uint8_t crc_len = 2;
};

} // namespace siyi
