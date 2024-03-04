#pragma once

#include "siyi_protocol.hpp"

#include <cassert>

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

        _messager.send(_serializer.assemble_message(siyi::GetStreamSettings{}));
        const auto maybe_stream_settings = _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());
        if (maybe_stream_settings) {
            _stream_settings = maybe_stream_settings.value();
        } else {
            return false;
        }

        return true;
    }

    void print_stream_settings()
    {
        std::cout << "Stream settings: \n"
                  << _stream_settings;
    }

    void print_version()
    {
        std::cout << _version;
    }

    enum class Codec {
        H264,
        H265,
    };

    enum class Resolution {
        Res1280x720,
        Res1920x1080,
    };

    enum class Zoom {
        Stop,
        In,
        Out,
    };


    [[nodiscard]] Resolution resolution() const {
        if (_stream_settings.resolution_h == 1080 && _stream_settings.resolution_l == 1920) {
            return Resolution::Res1920x1080;
        } else if (_stream_settings.resolution_h == 720 && _stream_settings.resolution_l == 1280) {
            return Resolution::Res1280x720;
        } else {
            std::cerr << "resolution invalid\n";
            assert(false);
            return Resolution::Res1280x720;
        }
    }

    bool set_resolution(Resolution resolution) {

        auto set_stream_settings = siyi::StreamSettings{};

        if (resolution == Resolution::Res1280x720) {
            set_stream_settings.resolution_l = 1280;
            set_stream_settings.resolution_h = 720;
        } else if (resolution == Resolution::Res1920x1080) {
            set_stream_settings.resolution_l = 1920;
            set_stream_settings.resolution_h = 1080;
        } else {
            std::cerr << "resolution invalid\n";
            return false;
        }

        set_stream_settings.video_enc_type = _stream_settings.video_enc_type;
        set_stream_settings.video_bitrate_kbps= _stream_settings.video_bitrate_kbps;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings=
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed\n";
            return false;
        }

        _messager.send(_serializer.assemble_message(siyi::GetStreamSettings{}));
        const auto maybe_stream_settings =
            _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            _stream_settings = maybe_stream_settings.value();
            return true;
        } else {
            return false;
        }
    }

    [[nodiscard]] Codec codec() const {
        if (_stream_settings.video_enc_type == 1) {
            return Codec::H264;
        } else if (_stream_settings.video_enc_type == 2) {
            return Codec::H265;
        } else {
            std::cerr << "codec invalid\n";
            assert(false);
            return Codec::H264;
        }
    }

    bool set_codec(Codec codec)
    {
        auto set_stream_settings = siyi::StreamSettings{};

        if (codec == Codec::H264) {
            set_stream_settings.video_enc_type = 1;
        } else if (codec == Codec::H265) {
            set_stream_settings.video_enc_type = 2;
        } else {
            std::cerr << "codec invalid\n";
            return false;
        }
        set_stream_settings.video_bitrate_kbps= _stream_settings.video_bitrate_kbps;
        set_stream_settings.resolution_l = _stream_settings.resolution_l;
        set_stream_settings.resolution_h = _stream_settings.resolution_h;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings=
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed\n";
            return false;
        }

        _messager.send(_serializer.assemble_message(siyi::GetStreamSettings{}));
        const auto maybe_stream_settings =
                _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            _stream_settings = maybe_stream_settings.value();
            return true;
        } else {
            return false;
        }
    }

    bool set_bitrate(unsigned bitrate)
    {
        auto set_stream_settings = siyi::StreamSettings{};

        set_stream_settings.video_bitrate_kbps = static_cast<std::uint16_t>(bitrate);
        set_stream_settings.video_enc_type = _stream_settings.video_enc_type;
        set_stream_settings.resolution_l = _stream_settings.resolution_l;
        set_stream_settings.resolution_h = _stream_settings.resolution_h;

        _messager.send(_serializer.assemble_message(set_stream_settings));
        const auto maybe_ack_set_stream_settings=
                _deserializer.disassemble_message<siyi::AckSetStreamSettings>(_messager.receive());

        if (!maybe_ack_set_stream_settings || maybe_ack_set_stream_settings.value().result != 1) {
            std::cerr << "setting stream settings failed\n";
            return false;
        }

        _messager.send(_serializer.assemble_message(siyi::GetStreamSettings{}));
        const auto maybe_stream_settings =
                _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());

        if (maybe_stream_settings) {
            _stream_settings = maybe_stream_settings.value();
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
                break;
            case Zoom::Out:
                manual_zoom.zoom = -1;
                break;
            case Zoom::Stop:
                manual_zoom.zoom = 0;
                break;
        }

        _messager.send(_serializer.assemble_message(manual_zoom));

        // We don't seem to be getting anything back.
        //const auto maybe_ack_manual_zoom =
        //        _deserializer.disassemble_message<siyi::AckManualZoom>(_messager.receive());

        //if (maybe_ack_manual_zoom) {
        //    std::cerr << "current zoom: " << maybe_ack_manual_zoom.value().zoom_multiple << '\n';
        //    _ack_manual_zoom = maybe_ack_manual_zoom.value();
        //    return false;
        //}
        return true;
    }

    [[nodiscard]] unsigned zoom() const {
        return _ack_manual_zoom.zoom_multiple;
    }

private:
    Serializer& _serializer;
    Deserializer& _deserializer;
    Messager& _messager;

    AckFirmwareVersion _version{};
    AckGetStreamResolution _stream_settings{};
    AckManualZoom _ack_manual_zoom{};
};

} // siyi
