#include <iostream>
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/camera_server/camera_server.h>

int main(int argc, char** argv)
{
    mavsdk::Mavsdk mavsdk;
    mavsdk::Mavsdk::Configuration configuration(mavsdk::Mavsdk::Configuration::UsageType::Camera);
    mavsdk.set_configuration(configuration);

    auto result = mavsdk.add_any_connection("udp://:14030");
    if (result != mavsdk::ConnectionResult::Success) {
        std::cerr << "Could not establish connection: " << result << std::endl;
        return 1;
    }
    std::cout << "Created camera server connection" << std::endl;

    auto camera_server = mavsdk::CameraServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ServerComponentType::Camera)};

    auto ret = camera_server.set_information({
        .vendor_name = "SIYI",
        .model_name = "A8 mini",
        .firmware_version = "0.0.0",
        .focal_length_mm = 21,
        .horizontal_sensor_size_mm = 9.5,
        .vertical_sensor_size_mm = 7.6,
        .horizontal_resolution_px = 4000,
        .vertical_resolution_px = 3000,
        .lens_id = 0,
        .definition_file_version = 0, // TODO: add this
        .definition_file_uri = "", // TODO: implement this
    });

    if (ret != mavsdk::CameraServer::Result::Success) {
        std::cerr << "Failed to set camera info, exiting" << std::endl;
        return 2;
    }

    ret = camera_server.set_video_streaming(mavsdk::CameraServer::VideoStreaming{
        .has_rtsp_server = true,
        .rtsp_uri = "rtsp://192.168.1.39:8554/live",
    });

    if (ret != mavsdk::CameraServer::Result::Success) {
        std::cerr << "Failed to set video stream info, exiting" << std::endl;
        return 2;
    }

    // Run as a server and never quit
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
