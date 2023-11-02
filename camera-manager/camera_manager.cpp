#include <iostream>
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/camera_server/camera_server.h>
#include "siyi.hpp"

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Error: Invalid argument.\n"
                  << "\n"
                  << "Usage: " << argv[0] << " <mavsdk connection url> <ground station connection url> <our ip>\n"
                  << "\n"
                  << "E.g. " << argv[0] << " serial:///dev/ttyUSB0:57600 udp://192.168.1.51:14550 192.168.1.45\n";
        return 1;
    }

    const std::string mavsdk_connection_url{argv[1]};
    const std::string groundstation_connection_url{argv[2]};
    const std::string our_ip{argv[3]};

    // MAVSDK setup first
    mavsdk::Mavsdk mavsdk;
    mavsdk::Mavsdk::Configuration configuration(mavsdk::Mavsdk::ComponentType::Camera);
    mavsdk.set_configuration(configuration);

    auto result = mavsdk.add_any_connection(mavsdk_connection_url, mavsdk::ForwardingOption::ForwardingOn);
    if (result != mavsdk::ConnectionResult::Success) {
        std::cerr << "Could not establish autopilot connection: " << result << std::endl;
        return 1;
    }

    result = mavsdk.add_any_connection(groundstation_connection_url, mavsdk::ForwardingOption::ForwardingOn);
    if (result != mavsdk::ConnectionResult::Success) {
        std::cerr << "Could not establish ground station connection: " << result << std::endl;
        return 1;
    }

    std::cout << "Created camera server connection" << std::endl;

    auto camera_server = mavsdk::CameraServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

    // SIYI setup second
    siyi::Messager siyi_messager;
    siyi_messager.setup("192.168.144.25", 37260);

    siyi::Serializer siyi_serializer;

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
        .rtsp_uri = std::string{"rtsp://"} + our_ip + std::string{":8554/live"},
    });

    if (ret != mavsdk::CameraServer::Result::Success) {
        std::cerr << "Failed to set video stream info, exiting" << std::endl;
        return 2;
    }

    camera_server.subscribe_take_photo([&camera_server, &siyi_messager, &siyi_serializer](int32_t index) {
            camera_server.set_in_progress(true);

            std::cout << "Taking a picture (" << +index << ")..." << std::endl;
            siyi_messager.send(siyi_serializer.assemble_message(siyi::TakePicture{}));

            // TODO: populate with telemetry data
            auto position = mavsdk::CameraServer::Position{};
            auto attitude = mavsdk::CameraServer::Quaternion{};

            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
            auto success = true;

            camera_server.set_in_progress(false);

            camera_server.respond_take_photo(
                mavsdk::CameraServer::CameraFeedback::Ok,
                mavsdk::CameraServer::CaptureInfo{
                    .position = position,
                    .attitude_quaternion = attitude,
                    .time_utc_us = static_cast<uint64_t>(timestamp),
                    .is_success = success,
                    .index = index,
                    .file_url = {},
                });
        });

    // Run as a server and never quit
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
