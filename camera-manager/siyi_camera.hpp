#pragma once

#include "siyi_protocol.hpp"

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
            version = maybe_version.value();
        } else {
            return false;
        }

        _messager.send(_serializer.assemble_message(siyi::GetStreamResolution{}));
        const auto maybe_stream_settings = _deserializer.disassemble_message<siyi::AckGetStreamResolution>(_messager.receive());
        if (maybe_stream_settings) {
            stream_settings = maybe_stream_settings.value();
        } else {
            return false;
        }

        return true;
    }

    AckFirmwareVersion version{};
    AckGetStreamResolution stream_settings{};

private:
    Serializer& _serializer;
    Deserializer& _deserializer;
    Messager& _messager;

};

} // siyi