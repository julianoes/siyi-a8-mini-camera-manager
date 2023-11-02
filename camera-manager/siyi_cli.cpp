#include "siyi.hpp"
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
    siyi_messager.receive();

    const std::string action{argv[1]};

    if (action == "firmware_version") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::FirmwareVersion{}));
        std::cout << "Requested firmware version\n";

    } else if (action == "take_picture") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::TakePicture{}));
        std::cout << "Took picture\n";

    } else if (action == "toggle_recording") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
        std::cout << "Toggled recording\n";

    } else if (action == "stream_1080") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res1080}));
        std::cout << "Set stream resolution to 1080\n";

    } else if (action == "stream_720") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res720}));
        std::cout << "Set stream resolution to 720\n";

    } else if (action == "stream_settings") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamSettings{}));
        std::cout << "Requested stream settings\n";

    } else if (action == "gimbal_forward") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::GimbalCenter{}));
        std::cout << "Set gimbal forward\n";

    } else {
        std::cerr << "Unknown command\n";
        return 2;
    }

    siyi_messager.receive();

    return 0;
}
