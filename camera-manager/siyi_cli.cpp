#include "siyi_camera.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Invalid argument\n";
        return 1;
    }

    // SIYI setup second
    siyi::Messager siyi_messager;
    siyi_messager.setup("192.168.144.25", 37260);

    siyi::Serializer siyi_serializer;
    siyi::Deserializer siyi_deserializer;

    siyi::Camera siyi_camera{siyi_serializer, siyi_deserializer, siyi_messager};

    if (!siyi_camera.init()) {
        std::cerr << "Error: camera could not get initialized.";
        return 1;
    }

    siyi_camera.print_version();
    siyi_camera.print_stream_settings();
    std::cout << "---\n";

    const std::string action{argv[1]};

    if (action == "take_picture") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::TakePicture{}));
        (void)siyi_messager.receive();
        std::cout << "Took picture\n";

    } else if (action == "toggle_recording") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
        (void)siyi_messager.receive();
        std::cout << "Toggled recording\n";

    } else if (action == "stream_1080") {
        std::cout << "Set resolution to 1920x1080..." << std::flush;
        if (siyi_camera.set_resolution(siyi::Camera::Resolution::Res1920x1080)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "stream_720") {
        std::cout << "Set resolution to 1280x720..." << std::flush;
        if (siyi_camera.set_resolution(siyi::Camera::Resolution::Res1280x720)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "codec_h264") {
        std::cout << "Set codec to H264..." << std::flush;
        if (siyi_camera.set_codec(siyi::Camera::Codec::H264)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "codec_h265") {
        std::cout << "Set codec to H265..." << std::flush;
        if (siyi_camera.set_codec(siyi::Camera::Codec::H265)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "bitrate_4m") {
        std::cout << "Set bitrate to 4 Mbps..." << std::flush;
        if (siyi_camera.set_bitrate(4000)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "bitrate_3m") {
        std::cout << "Set bitrate to 3 Mbps..." << std::flush;
        if (siyi_camera.set_bitrate(3000)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "bitrate_2m") {
        std::cout << "Set bitrate to 2 Mbps..." << std::flush;
        if (siyi_camera.set_bitrate(2000)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }

    } else if (action == "bitrate_1m") {
        std::cout << "Set bitrate to 1 Mbps..." << std::flush;
        if (siyi_camera.set_bitrate(1000)) {
            std::cout << "ok" << std::endl;
        } else {
            std::cout << "failed" << std::endl;
            return 1;
        }


    } else if (action == "stream_settings") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamSettings{}));
        (void)siyi_messager.receive();
        std::cout << "Requested stream settings\n";

    } else if (action == "gimbal_forward") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::GimbalCenter{}));
        (void)siyi_messager.receive();
        std::cout << "Set gimbal forward\n";

    } else {
        std::cerr << "Unknown command\n";
        return 2;
    }

    siyi_camera.print_stream_settings();

    return 0;
}
