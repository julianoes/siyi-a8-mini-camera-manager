#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>
#include <mavsdk/mavsdk.h>
#include <mavsdk/log_callback.h>
#include <mavsdk/plugins/camera_server/camera_server.h>
#include <mavsdk/plugins/ftp_server/ftp_server.h>
#include <mavsdk/plugins/param_server/param_server.h>
#include "siyi_protocol.hpp"
#include "siyi_camera.hpp"

class CommandLineParser {
public:
    static void print_usage(const std::string& programName) {
        std::cout << "Usage: " << programName << " [options]\n"
                  << "Options:\n"
                  << "  --connection <connection string>   Specify a connection string (can be used multiple times)\n"
                  << "  --forwarding <on|off>              Enable or disable forwarding (default off)\n"
                  << "  --stream-url <stream string>       Specify the stream URL\n"
                  << "  --help                             Show this help message\n";
    }

    enum Result {
        Ok,
        Invalid,
        Help,
    };

    Result parse(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            std::string current_arg = argv[i];

            if (current_arg == "--help") {
                return Result::Help;

            } else if (current_arg == "--connection") {
                if (i + 1 < argc) {
                    connections.emplace_back(argv[++i]);
                } else {
                    std::cerr << "Error: --connection requires a value" << std::endl;
                    return Result::Invalid;
                }
            } else if (current_arg == "--forwarding") {
                if (i + 1 < argc) {
                    auto option = std::string(argv[++i]);
                    if (option == "on") {
                        forwarding = true;
                    } else if (option == "off") {
                        forwarding = false;
                    } else {
                        std::cerr << "Error: --forwarding requires 'on' or 'off'" << std::endl;
                        return Result::Invalid;
                    }
                } else {
                    std::cerr << "Error: --forwarding requires a value" << std::endl;
                    return Result::Invalid;
                }
            } else if (current_arg == "--stream-url") {
                if (i + 1 < argc) {
                    stream_url = argv[++i];
                } else {
                    std::cerr << "Error: --stream-url requires a value" << std::endl;
                    return Result::Invalid;
                }
            } else {
                std::cerr << "Unknown argument: " << current_arg << std::endl;
                return Result::Invalid;
            }
        }

        if (connections.empty()) {
            std::cerr << "At least one connection is required:" << std::endl;
            return Result::Invalid;
        }

        if (stream_url.empty()) {
            std::cerr << "Stream URL is required:" << std::endl;
            return Result::Invalid;
        }

        return Result::Ok;
    }

    std::vector<std::string> connections;
    std::string stream_url;
    bool forwarding {false};
};

int main(int argc, char* argv[])
{
    CommandLineParser parser;
    switch (parser.parse(argc, argv)) {
        case CommandLineParser::Result::Help:
            CommandLineParser::print_usage(argv[0]);
            return 0;

        case CommandLineParser::Result::Invalid:
            CommandLineParser::print_usage(argv[0]);
            return 1;

        case CommandLineParser::Result::Ok:
            break;
    }

    // SIYI setup first
    siyi::Messager siyi_messager;
    siyi_messager.setup("192.168.144.25", 37260);

    siyi::Serializer siyi_serializer;
    siyi::Deserializer siyi_deserializer;
    siyi::Camera siyi_camera{siyi_serializer, siyi_deserializer, siyi_messager};
    if (!siyi_camera.init()) {
        std::cerr << "Could not initialize camera" << std::endl;
        return 3;
    }

    // MAVSDK setup second
    mavsdk::Mavsdk mavsdk{mavsdk::Mavsdk::Configuration{mavsdk::Mavsdk::ComponentType::Camera}};

    // We overwrite the mavsdk logs to prepend "Mavsdk:" and to make sure we flush it after every
    // line using std::endl. Otherwise, it gets buffered by systemctl and logs appear delayed.

    mavsdk::log::subscribe([](mavsdk::log::Level level,   // message severity level
                              const std::string& message, // message text
                              const std::string& file,    // source file from which the message was sent
                              int line) {                 // line number in the source file

        std::cout << "Mavsdk ";
        // process the log message in a way you like
        switch (level) {
            case mavsdk::log::Level::Debug:
                std::cout << "debug: ";
                break;
            case mavsdk::log::Level::Info:
                std::cout << "info: ";
                break;
            case mavsdk::log::Level::Warn:
                std::cout << "warning: ";
                break;
            case mavsdk::log::Level::Err:
                std::cout << "error: ";
        }
        std::cout << message;
        std::cout << " (" << file << ":" << line << ")" << std::endl;

        // returning true from the callback disables default printing
        return true;
    });

    for (auto& connection : parser.connections) {
        auto result = mavsdk.add_any_connection(
            connection,
            parser.forwarding ? mavsdk::ForwardingOption::ForwardingOn : mavsdk::ForwardingOption::ForwardingOff);

        if (result != mavsdk::ConnectionResult::Success) {
            std::cerr << "Could not establish connection '" << connection << """': " << result << std::endl;
            return 1;
        }
        std::cout << "Created connection '" << connection << "' forwarding '"
                  << (parser.forwarding ? "on" : "off") << "'"<<std::endl;
    }

    auto ftp_server = mavsdk::FtpServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

    // If running locally when built first, otherwise use system-wise:
    std::string path = "./camera-manager/mavlink_ftp_root";
    if (!std::filesystem::exists(path)) {
        path = "/usr/share/mavlink_ftp_root";
    }

    std::cout << "Using FTP root: " << path << " to serve camera xml file." << std::endl;

    auto ftp_result = ftp_server.set_root_dir(path);
    if (ftp_result != mavsdk::FtpServer::Result::Success) {
        std::cerr << "Could not set FTP server root dir: " << ftp_result << std::endl;
        return 2;
    }

    auto param_server = mavsdk::ParamServer{
        mavsdk.server_component_by_type(mavsdk::Mavsdk::ComponentType::Camera)};

    int32_t stream_res = 0;
    switch (siyi_camera.resolution()) {
        case siyi::Camera::Resolution::Res1280x720:
            stream_res = 0;
            break;
        case siyi::Camera::Resolution::Res1920x1080:
            stream_res = 1;
            break;
        default:
            std::cerr << "Unexpected stream resolution" << std::endl;
            break;
    }

    int32_t stream_codec = 0;
    switch (siyi_camera.codec(siyi::Camera::Type::Stream)) {
        case siyi::Camera::Codec::H264:
            stream_codec = 1;
            break;
        case siyi::Camera::Codec::H265:
            stream_codec = 2;
            break;
    }

    param_server.provide_param_int("CAM_MODE", 0);
    param_server.provide_param_int("STREAM_RES", stream_res);
    param_server.provide_param_int("STREAM_BITRATE", static_cast<int32_t>(siyi_camera.bitrate()));
    param_server.provide_param_int("STREAM_CODEC", stream_codec);

    param_server.subscribe_changed_param_int([&](auto param_int) {
        if (param_int.name == "STREAM_RES") {
            if (param_int.value == 0) {
                std::cout << "Set stream resolution to 1280x720" << std::endl;
                (void)siyi_camera.set_resolution(siyi::Camera::Type::Stream, siyi::Camera::Resolution::Res1280x720);
                // TODO: should we ack/nack?
            } else if (param_int.value == 1) {
                std::cout << "Set stream resolution to 1920x1080" << std::endl;
                (void)siyi_camera.set_resolution(siyi::Camera::Type::Stream, siyi::Camera::Resolution::Res1920x1080);
                // TODO: should we ack/nack?
            } else {
                std::cout << "Unknown stream resolution" << std::endl;
            }
        } else if (param_int.name == "STREAM_BITRATE") {
            std::cout << "Set bitrate to " << param_int.value << std::endl;
            (void)siyi_camera.set_bitrate(siyi::Camera::Type::Stream, param_int.value);

        } else if (param_int.name == "STREAM_CODEC") {
            if (param_int.value == 1) {
                std::cout << "Set codec to H264" << std::endl;
                (void)siyi_camera.set_codec(siyi::Camera::Type::Stream, siyi::Camera::Codec::H264);
            } else if (param_int.value == 2) {
                std::cout << "Set codec to H265" << std::endl;
                (void)siyi_camera.set_codec(siyi::Camera::Type::Stream, siyi::Camera::Codec::H265);
            } else {
                std::cout << "Unknown codec" << std::endl;
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
        .definition_file_version = 30,
        .definition_file_uri = "mftp://siyi_a8_mini.xml",
    });

    if (ret != mavsdk::CameraServer::Result::Success) {
        std::cerr << "Failed to set camera info, exiting" << std::endl;
        return 2;
    }

    ret = camera_server.set_video_streaming(mavsdk::CameraServer::VideoStreaming{
        .has_rtsp_server = true,
        .rtsp_uri = parser.stream_url,
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
            std::cout << "Video already started" << std::endl;
            camera_server.respond_start_video(
                mavsdk::CameraServer::CameraFeedback::Failed);

        } else {
            std::cout << "Start video" << std::endl;
            siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
            std::cout << "Video started" << std::endl;
            siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
            recording = true;
            recording_start_time = std::chrono::steady_clock::now();
            camera_server.respond_start_video(
                mavsdk::CameraServer::CameraFeedback::Ok);
        }
    });

    camera_server.subscribe_stop_video([&](int32_t) {

        if (!recording) {
            std::cout << "Video not started" << std::endl;
            camera_server.respond_stop_video(
                mavsdk::CameraServer::CameraFeedback::Failed);

        } else {
            std::cout << "Stop video" << std::endl;
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
                / static_cast<float>(std::chrono::steady_clock::period::den) :
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

    camera_server.subscribe_zoom_range([&](float zoom_factor) {
        if (zoom_factor < 1.f) {
            std::cout << "Zoom below 1x not possible" << std::endl;
            camera_server.respond_zoom_range(mavsdk::CameraServer::CameraFeedback::Failed);
            return;
        }
        if (zoom_factor > 6.f) {
            std::cout << "Zoom above 6x not possible" << std::endl;
            camera_server.respond_zoom_range(mavsdk::CameraServer::CameraFeedback::Failed);
            return;
        }

        siyi_camera.absolute_zoom(zoom_factor);
        camera_server.respond_zoom_range(mavsdk::CameraServer::CameraFeedback::Ok);
    });

    camera_server.subscribe_zoom_in_start([&](int) {
        siyi_camera.zoom(siyi::Camera::Zoom::In);
        camera_server.respond_zoom_in_start(mavsdk::CameraServer::CameraFeedback::Ok);
    });

    camera_server.subscribe_zoom_out_start([&](int) {
        siyi_camera.zoom(siyi::Camera::Zoom::Out);
        camera_server.respond_zoom_in_start(mavsdk::CameraServer::CameraFeedback::Ok);
    });

    camera_server.subscribe_zoom_stop([&](int) {
        siyi_camera.zoom(siyi::Camera::Zoom::Stop);
        camera_server.respond_zoom_stop(mavsdk::CameraServer::CameraFeedback::Ok);
    });

    // Run as a server and never quit
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
