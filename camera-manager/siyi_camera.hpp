#pragma once

#include "siyi_protocol.hpp"

#include <cassert>
#include <cmath>

namespace siyi {

class Camera {
public:
    Camera(Serializer& serializer, Deserializer& deserializer, Messager& messager) :
        _serializer(serializer),
        _deserializer(deserializer),
        _messager(messager) {}

    [[nodiscard]] bool init()
    {
        _messager.send(_serializer.assemble_message(siyi::FirmwareVersion{}));

        const auto maybe_version = _deserializer.disassemble_message<siyi::AckFirmwareVersion>(_messager.receive());
        if (maybe_version) {
            _version = maybe_version.value();
        } else {
            return false;
        }

        {
            auto get_stream_settings = siyi::GetStreamSettings{};
            get_stream_settings.stream_type = 1;
            _messager.send(_serializer.assemble_message(get_stream_settings));
            const auto maybe_stream_settings = _deserializer.disassemble_message<siyi::AckGetStreamResolution>(
                    _messager.receive());
            if (maybe_stream_settings) {
                _stream_settings = maybe_stream_settings.value();
            } else {
                return false;
            }
        }

        {
            auto get_recording_settings = siyi::GetStreamSettings{};
            get_recording_settings.stream_type = 0;
            _messager.send(_serializer.assemble_message(get_recording_settings));
            const auto maybe_recording_settings = _deserializer.disassemble_message<siyi::AckGetStreamResolution>(
                    _messager.receive());
            if (maybe_recording_settings) {
                _recording_settings = maybe_recording_settings.value();
            } else {
                return false;
            }
        }

        return true;
    }

    void print_version()
    {
        std::cout << _version;
    }

    enum class Type {
        Recording,
        Stream,
    };

    enum class Codec {
        H264,
        H265,
    };

    enum class Resolution {
        Res1280x720,
        Res1920x1080,
        Res2560x1440,
        Res3840x2160,
    };

    enum class Zoom {
        Stop,
        In,
        Out,
    };

    void print_settings(Type type)
    {
        switch (type) {
            case Type::Recording:
                std::cout << "Recording settings: \n"
                          << _recording_settings;
                break;
            case Type::Stream:
                std::cout << "Stream settings: \n"
                          << _stream_settings;
                break;
        }
    }

    [[nodiscard]] Resolution resolution() const {
        if (_stream_settings.resolution_h == 1920 && _stream_settings.resolution_l == 3840) {
            return Resolution::Res3840x2160;
        } else if (_stream_settings.resolution_h == 1440 && _stream_settings.resolution_l == 2560) {
            return Resolution::Res2560x1440;
        } else if (_stream_settings.resolution_h == 1080 && _stream_settings.resolution_l == 1920) {
            return Resolution::Res1920x1080;
        } else if (_stream_settings.resolution_h == 720 && _stream_settings.resolution_l == 1280) {
            return Resolution::Res1280x720;
        } else {
            std::cerr << "resolution invalid" << std::endl;
            assert(false);
            return Resolution::Res1280x720;
        }
    }

    bool set_resolution(Type type, Resolution resolution) {

        auto set_stream_settings = siyi::StreamSettings{};

        switch (type) {
            case Type::Recording:
                set_stream_settings.stream_type = 0;
                break;
            case Type::Stream:
                set_stream_settings.stream_type = 1;
                break;
        }

        if (resolution == Resolution::Res1280x720) {
            set_stream_settings.resolution_l = 1280;
            set_stream_settings.resolution_h = 720;
        } else if (resolution == Resolution::Res1920x1080) {
            set_stream_settings.resolution_l = 1920;
            set_stream_settings.resolution_h = 1080;
        } else if (resolution == Resolution::Res2560x1440) {
            set_stream_settings.resolution_l = 2560;
            set_stream_settings.resolution_h = 1440;
        } else if (resolution == Resolution::Res3840x2160) {
            set_stream_settings.resolution_l = 3840;
            set_stream_settings.resolution_h = 2160;
        } else {
            std::cerr << "resolution invalid" << std::endl;
            return false;
        }

        set_stream_settings.video_enc_type = _stream_settings.video_enc_type;
        set_stream_settings.video_bitrate_kbps= _stream_settings.video_bitrate_kbps;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings=
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed" << std::endl;
            return false;
        }

        auto get_stream_settings = siyi::GetStreamSettings{};
        get_stream_settings.stream_type = set_stream_settings.stream_type;

        _messager.send(_serializer.assemble_message(get_stream_settings));
        const auto maybe_stream_settings =
            _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            switch (type) {
                case Type::Recording:
                    _recording_settings = maybe_stream_settings.value();
                    break;
                case Type::Stream:
                    _stream_settings = maybe_stream_settings.value();
                    break;
            }
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] Codec codec(Type type) const {
        auto& settings = (type == Type::Recording ? _recording_settings : _stream_settings);

        if (settings.video_enc_type == 1) {
            return Codec::H264;
        } else if (settings.video_enc_type == 2) {
            return Codec::H265;
        } else {
            std::cerr << "codec invalid" << std::endl;
            assert(false);
            return Codec::H264;
        }
    }

    bool set_codec(Type type, Codec codec) {
        auto set_stream_settings = siyi::StreamSettings{};
        switch (type) {
            case Type::Recording:
                set_stream_settings.stream_type = 0;
                break;
            case Type::Stream:
                set_stream_settings.stream_type = 1;
                break;
        }

        if (codec == Codec::H264) {
            set_stream_settings.video_enc_type = 1;
        } else if (codec == Codec::H265) {
            set_stream_settings.video_enc_type = 2;
        } else {
            std::cerr << "codec invalid" << std::endl;
            return false;
        }
        set_stream_settings.video_bitrate_kbps = _stream_settings.video_bitrate_kbps;
        set_stream_settings.resolution_l = _stream_settings.resolution_l;
        set_stream_settings.resolution_h = _stream_settings.resolution_h;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings =
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed" << std::endl;
            return false;
        }
        auto get_stream_settings = siyi::GetStreamSettings{};
        get_stream_settings.stream_type = set_stream_settings.stream_type;
        _messager.send(_serializer.assemble_message(get_stream_settings));
        const auto maybe_stream_settings =
                _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            switch (type) {
                case Type::Recording:
                    _recording_settings = maybe_stream_settings.value();
                    break;
                case Type::Stream:
                    _stream_settings = maybe_stream_settings.value();
                    break;
            }
            return true;
        } else {
            return false;
        }
    }

    bool set_bitrate(Type type, unsigned bitrate)
    {
        auto set_stream_settings = siyi::StreamSettings{};

        switch (type) {
            case Type::Recording:
                set_stream_settings.stream_type = 0;
                break;
            case Type::Stream:
                set_stream_settings.stream_type = 1;
                break;
        }

        set_stream_settings.video_bitrate_kbps = static_cast<std::uint16_t>(bitrate);
        set_stream_settings.video_enc_type = _stream_settings.video_enc_type;
        set_stream_settings.resolution_l = _stream_settings.resolution_l;
        set_stream_settings.resolution_h = _stream_settings.resolution_h;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings=
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed" << std::endl;
            return false;
        }

        auto get_stream_settings = siyi::GetStreamSettings{};
        get_stream_settings.stream_type = set_stream_settings.stream_type;
        _messager.send(_serializer.assemble_message(get_stream_settings));
        const auto maybe_stream_settings =
                _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            switch (type) {
                case Type::Recording:
                    _recording_settings = maybe_stream_settings.value();
                    break;
                case Type::Stream:
                    _stream_settings = maybe_stream_settings.value();
                    break;
            }
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] unsigned bitrate() const {
        return _stream_settings.video_bitrate_kbps;
    }

    bool zoom(Zoom option)
    {
        auto manual_zoom = siyi::ManualZoom{};

        switch (option) {
            case Zoom::In:
                manual_zoom.zoom = 1;
                std::cerr << "Starting to zoom in" << std::endl;
                break;
            case Zoom::Out:
                manual_zoom.zoom = -1;
                std::cerr << "Starting to zoom out" << std::endl;
                break;
            case Zoom::Stop:
                std::cerr << "Stopping to zoom" << std::endl;
                manual_zoom.zoom = 0;
                break;
        }

        _messager.send(_serializer.assemble_message(manual_zoom));

        // We don't seem to be getting anything back, it just times out.
        //const auto maybe_ack_manual_zoom =
        //        _deserializer.disassemble_message<siyi::AckManualZoom>(_messager.receive());

        //if (maybe_ack_manual_zoom) {
        //    std::cerr << "current zoom: " << maybe_ack_manual_zoom.value().zoom_multiple << std::endl;
        //    _ack_manual_zoom = maybe_ack_manual_zoom.value();
        //    return false;
        //}
        return true;
    }

    // Unavailable
    //[[nodiscard]] unsigned zoom() const {
    //    return _ack_manual_zoom.zoom_multiple;
    //}

    bool absolute_zoom(float factor)
    {
        auto message = siyi::AbsoluteZoom{};

        if (factor > static_cast<float>(0x1E)) {
            std::cerr << "zoom factor too high" << std::endl;
            return false;
        }
        if (factor < 1.f) {
            std::cerr << "zoom factor too small" << std::endl;
            return false;
        }

        message.absolute_movement_integer = static_cast<uint8_t>(factor);
        message.absolute_movement_fractional = static_cast<uint8_t>(std::roundf((factor-static_cast<float>(message.absolute_movement_integer)) * 10.f));

        std::cerr << "Sending abs zoom: " << (int)message.absolute_movement_integer << "." << (int)message.absolute_movement_fractional << std::endl;

        _messager.send(_serializer.assemble_message(message));

        // We don't seem to be getting anything back, it just times out.
        //const auto maybe_ack_manual_zoom =
        //        _deserializer.disassemble_message<siyi::AckManualZoom>(_messager.receive());

        //if (maybe_ack_manual_zoom) {
        //    std::cerr << "current zoom: " << maybe_ack_manual_zoom.value().zoom_multiple << std::endl;
        //    _ack_manual_zoom = maybe_ack_manual_zoom.value();
        //    return false;
        //}
        return true;
    }

private:
    Serializer& _serializer;
    Deserializer& _deserializer;
    Messager& _messager;

    AckFirmwareVersion _version{};
    AckGetStreamResolution _recording_settings{};
    AckGetStreamResolution _stream_settings{};
    // AckManualZoom _ack_manual_zoom{};
};

} // siyi
