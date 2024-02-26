#include <iostream>
#include <chrono>
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/camera_server/camera_server.h>
#include <mavsdk/plugins/ftp_server/ftp_server.h>
#include <mavsdk/plugins/param_server/param_server.h>
#include "siyi_protocol.hpp"

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

    // SIYI setup first
    siyi::Messager siyi_messager;
    siyi_messager.setup("192.168.144.25", 37260);

    siyi::Serializer siyi_serializer;

    // MAVSDK setup second
    mavsdk::Mavsdk mavsdk{mavsdk::Mavsdk::Configuration{mavsdk::Mavsdk::ComponentType::Camera}};

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

    auto ftp_server = mavsdk::FtpServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

    auto ftp_result = ftp_server.set_root_dir("./mavlink_ftp_root");
    if (ftp_result != mavsdk::FtpServer::Result::Success) {
        std::cerr << "Could not set FTP server root dir: " << ftp_result << std::endl;
        return 2;
    }

    auto param_server = mavsdk::ParamServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

    param_server.provide_param_int("CAM_MODE", 0);
    param_server.provide_param_int("STREAM_RES", 0);

    param_server.subscribe_changed_param_int([&](auto param_int) {
        if (param_int.name == "STREAM_RES") {
            if (param_int.value == 0) {
                std::cout << "Set stream resolution to 1280x720" << std::endl;
                siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res720}));
            } else if (param_int.value == 1) {
                std::cout << "Set stream resolution to 1920x1080" << std::endl;
                siyi_messager.send(siyi_serializer.assemble_message(siyi::StreamResolution{siyi::StreamResolution::Resolution::Res1080}));
            } else {
                std::cout << "Unknown stream resolution" << std::endl;
            }
        }
    });

    auto camera_server = mavsdk::CameraServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

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
        .definition_file_version = 0,
        .definition_file_uri = "mftp://siyi_a8_mini.xml",
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

    int32_t images_captured = 0;

    camera_server.subscribe_take_photo([&](int32_t index) {

        // TODO: not sure what to do about this index.
        (void)index;
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
                .index = images_captured++,
                .file_url = {},
            });
        });

    bool recording = false;
    std::chrono::time_point<std::chrono::steady_clock> recording_start_time{};

    camera_server.subscribe_start_video([&](int32_t) {

        if (recording) {
            std::cout << "Video already started\n";
            camera_server.respond_start_video(
                mavsdk::CameraServer::CameraFeedback::Failed);

        } else {
            std::cout << "Start video\n";
            siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
            std::cout << "Video started\n";
            siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
            recording = true;
            recording_start_time = std::chrono::steady_clock::now();
            camera_server.respond_start_video(
                mavsdk::CameraServer::CameraFeedback::Ok);
        }
    });

    camera_server.subscribe_stop_video([&](int32_t) {

        if (!recording) {
            std::cout << "Video not started\n";
            camera_server.respond_stop_video(
                mavsdk::CameraServer::CameraFeedback::Failed);

        } else {
            std::cout << "Stop video\n";
            siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
            recording = false;
            camera_server.respond_stop_video(
                mavsdk::CameraServer::CameraFeedback::Ok);
        }
    });

    camera_server.subscribe_set_mode(
        [&](mavsdk::CameraServer::Mode mode) {
            switch (mode) {
                case mavsdk::CameraServer::Mode::Photo:
                    param_server.provide_param_int("CAM_MODE", 0);
                    camera_server.respond_set_mode(mavsdk::CameraServer::CameraFeedback::Ok);
                    break;
                case mavsdk::CameraServer::Mode::Video:
                    param_server.provide_param_int("CAM_MODE", 1);
                    camera_server.respond_set_mode(mavsdk::CameraServer::CameraFeedback::Ok);
                    break;
                case mavsdk::CameraServer::Mode::Unknown:
                    camera_server.respond_set_mode(mavsdk::CameraServer::CameraFeedback::Failed);
                    break;
            }
    });


    camera_server.subscribe_capture_status([&](int32_t) {

        auto camera_feedback = mavsdk::CameraServer::CameraFeedback::Ok;
        auto capture_status = mavsdk::CameraServer::CaptureStatus{};
        capture_status.image_interval_s = NAN;
        capture_status.recording_time_s = recording ?
           (static_cast<float>((std::chrono::steady_clock::now() - recording_start_time).count()) * std::chrono::steady_clock::period::num)
                / static_cast<double>(std::chrono::steady_clock::period::den) :
            NAN;
        capture_status.available_capacity_mib = NAN; // TODO: figure out remaining storage
        capture_status.image_status = mavsdk::CameraServer::CaptureStatus::ImageStatus::Idle;
        capture_status.video_status = recording ?
            mavsdk::CameraServer::CaptureStatus::VideoStatus::CaptureInProgress :
            mavsdk::CameraServer::CaptureStatus::VideoStatus::Idle;
        capture_status.image_count = images_captured;

        camera_server.respond_capture_status(camera_feedback, capture_status);
    });

    camera_server.subscribe_storage_information([&](int32_t) {

        auto storage_information_feedback = mavsdk::CameraServer::CameraFeedback::Ok;
        auto storage_information = mavsdk::CameraServer::StorageInformation{};
        storage_information.used_storage_mib = NAN; // TODO
        storage_information.available_storage_mib = NAN; // TODO
        storage_information.total_storage_mib = NAN; // TODO
        storage_information.storage_status = mavsdk::CameraServer::StorageInformation::StorageStatus::Formatted;
        storage_information.storage_id = 1;
        storage_information.storage_type = mavsdk::CameraServer::StorageInformation::StorageType::Microsd;

        camera_server.respond_storage_information(
            storage_information_feedback,
            storage_information);
    });

    // Run as a server and never quit
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
