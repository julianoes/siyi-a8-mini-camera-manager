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

    const std::string action{argv[1]};

    if (action == "version") {
        std::cout << siyi_camera.version;

    } else if (action == "take_picture") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::TakePicture{}));
        (void)siyi_messager.receive();
        std::cout << "Took picture\n";

    } else if (action == "toggle_recording") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
        (void)siyi_messager.receive();
        std::cout << "Toggled recording\n";

    } else if (action == "stream_1080") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res1080}));
        (void)siyi_messager.receive();
        std::cout << "Set stream resolution to 1080\n";

    } else if (action == "stream_720") {
        siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res720}));
        (void)siyi_messager.receive();
        std::cout << "Set stream resolution to 720\n";

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

    return 0;
}
