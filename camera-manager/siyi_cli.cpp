#include "siyi_camera.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void print_usage(const std::string_view& bin_name)
{
    std::cout << "Usage: " << bin_name << " action [options]\n"
              << "Actions:\n\n"
              << "  help                                        Show this help\n\n"
              << "  version                                     Show camera and gimbal version\n\n"
              << "  take_picture                                Take a picture to SD card\n\n"
              << "  toggle_recording                            Toggle start/stop video recording to SD card\n\n"
              << "  gimbal mode <follow|lock|fpv>               Set gimbal mode to follow, lock, or FPV\n\n"
              << "  gimbal neutral                              Set gimbal forward\n\n"
              << "  gimbal angle <pitch_value> <yaw_value>      Set gimbal angles pitch (in degees, negative down)\n"
              << "                                                and yaw (in degrees, positive is to the right)\n\n"
              << "  zoom <option>                               Use zoom\n"
              << "    Options:\n"
              << "      - in (to start zooming in)\n"
              << "      - out (to start zooming out)\n"
              << "      - stop (to stop zooming)\n"
              << "      - <factor> (1.0 to 6.0)\n"
              << "\n"
              << "  get <stream|recording> settings             Show all settings for stream or recording\n\n"
              << "\n"
              << "  set <stream|recording> resolution <option>  Set stream resolution\n\n"
              << "    Options:\n"
              << "      - 720  (for 1280x720)\n"
              << "      - 1080 (for 1920x1080)\n\n"
              << "      - 1440 (for 2560x1440, recording only)\n\n"
              << "      - 1920 (for 3840x1920, recording only)\n\n"
              << "  set <stream|recording> bitrate <option>     Set stream bitrate\n"
              << "    Options:\n"
              << "      - 1m (for 1.5 Mbps, only available at 1280x720)\n"
              << "      - 2m (for 2 Mbps)\n"
              << "      - 3m (for 3 Mbps)\n"
              << "      - 4m (for 4 Mbps)\n\n"
              << "  set <stream|recording> codec <option>       Set stream codec\n"
              << "    Options:\n"
              << "      - h264 (for H264)\n"
              << "      - h265 (for H265/HVEC)\n"
              << std::endl;
}

int main(int argc, char* argv[])
{
    siyi::Messager siyi_messager;
    siyi_messager.setup("192.168.144.25", 37260);

    siyi::Serializer siyi_serializer;
    siyi::Deserializer siyi_deserializer;

    siyi::Camera siyi_camera{siyi_serializer, siyi_deserializer, siyi_messager};

    if (!siyi_camera.init()) {
        std::cout << "Error: camera could not get initialized.";
        return 1;
    }

    if (argc == 1 ) {
        std::cout << "No argument supplied." << std::endl;
        print_usage(argv[0]);
        return 0;
    }

    const std::string_view action{argv[1]};

    if (action == "help") {
        print_usage(argv[0]);

    } else if (action == "version") {
        siyi_camera.print_version();

    } else if (action == "take_picture") {
        std::cout << "Take picture" << std::endl;
        siyi_messager.send(siyi_serializer.assemble_message(siyi::TakePicture{}));
        (void)siyi_messager.receive();

    } else if (action == "toggle_recording") {
        std::cout << "Toggle recording" << std::endl;
        siyi_messager.send(siyi_serializer.assemble_message(siyi::ToggleRecording{}));
        (void)siyi_messager.receive();

    } else if (action == "gimbal") {
        if (argc >= 3) {
            const std::string_view command {argv[2]};
            if (command == "mode") {
                if (argc == 4) {
                    const std::string_view mode {argv[3]};
                    siyi::SetGimbalMode set_gimbal_mode{};
                    if (mode == "lock") {
                        set_gimbal_mode.mode = siyi::SetGimbalMode::Mode::Lock;
                    } else if (mode == "follow") {
                        set_gimbal_mode.mode = siyi::SetGimbalMode::Mode::Follow;
                    } else if (mode == "fpv") {
                        set_gimbal_mode.mode = siyi::SetGimbalMode::Mode::Fpv;
                    } else {
                        std::cout << "Unkown mode: " << mode << std::endl;
                        print_usage(argv[0]);
                        return 1;
                    }
                    std::cout << "Set gimbal mode to " << mode << std::endl;
                    siyi_messager.send(siyi_serializer.assemble_message(set_gimbal_mode));
                    //(void)siyi_messager.receive();
                } else {
                    std::cout << "Not enough arguments" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }

            } else if (command == "neutral") {
                std::cout << "Set gimbal neutral" << std::endl;
                siyi_messager.send(siyi_serializer.assemble_message(siyi::GimbalCenter{}));
                (void)siyi_messager.receive();
            } else if (command == "angle") {
                if (argc >= 5) {
                    auto pitch = std::strtol(argv[3], nullptr, 10);
                    auto yaw = std::strtol(argv[4], nullptr, 10);
                    std::cout << "Set gimbal to " << pitch << " deg and yaw " << yaw << std::endl;
                    siyi::SetGimbalAttitude set_gimbal_attitude{};
                    set_gimbal_attitude.pitch_t10 = static_cast<std::int16_t>(pitch*10);
                    set_gimbal_attitude.yaw_t10 = static_cast<std::int16_t>(-yaw*10);
                    siyi_messager.send(siyi_serializer.assemble_message(set_gimbal_attitude));
                    (void)siyi_messager.receive();

                } else {
                    std::cout << "Not enough arguments" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }
            } else {
                std::cout << "Invalid gimbal command" << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }

    } else if (action == "get") {
        if (argc >= 3) {
            const std::string_view type_str{argv[2]};
            siyi::Camera::Type type;
            if (type_str == "stream") {
                type = siyi::Camera::Type::Stream;
            } else if (type_str == "recording") {
                type = siyi::Camera::Type::Recording;
            } else {
                std::cout << "Invalid type" << std::endl;
                print_usage(argv[0]);
                return 1;
            }

            siyi_camera.print_settings(type);
        } else {
            std::cout << "Not enough arguments" << std::endl;
            print_usage(argv[0]);
            return 1;
        }

    } else if (action == "set") {
        if (argc >= 5) {
            const std::string_view type_str{argv[2]};
            const std::string_view setting{argv[3]};
            const std::string_view option{argv[4]};

            siyi::Camera::Type type;
            if (type_str == "stream") {
                type = siyi::Camera::Type::Stream;
            } else if (type_str == "recording") {
                type = siyi::Camera::Type::Recording;
            } else {
                std::cout << "Invalid type" << std::endl;
                print_usage(argv[0]);
                return 1;
            }

            if (setting == "resolution") {
                if (option == "1920") {
                    std::cout << "Set " << type_str << " resolution to 3840x2160..." << std::flush;
                    if (siyi_camera.set_resolution(type, siyi::Camera::Resolution::Res3840x2160)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "1440") {
                    std::cout << "Set " << type_str << " resolution to 2560x1440..." << std::flush;
                    if (siyi_camera.set_resolution(type, siyi::Camera::Resolution::Res2560x1440)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "1080") {
                    std::cout << "Set " << type_str << " resolution to 1920x1080..." << std::flush;
                    if (siyi_camera.set_resolution(type, siyi::Camera::Resolution::Res1920x1080)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "720") {
                    std::cout << "Set " << type_str << " resolution to 1280x720..." << std::flush;
                    if (siyi_camera.set_resolution(type, siyi::Camera::Resolution::Res1280x720)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else {
                    std::cout << "Invalid resolution" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }
            } else if (setting == "bitrate") {
                if (option == "1m") {
                    std::cout << "Set " << type_str << " bitrate to 1.5 Mbps..." << std::flush;
                    if (siyi_camera.set_bitrate(type, 1500)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "2m") {
                    std::cout << "Set " << type_str << " bitrate to 2 Mbps..." << std::flush;
                    if (siyi_camera.set_bitrate(type, 2000)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "3m") {
                    std::cout << "Set " << type_str << " bitrate to 3 Mbps..." << std::flush;
                    if (siyi_camera.set_bitrate(type, 3000)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "4m") {
                    std::cout << "Set " << type_str << " bitrate to 4 Mbps..." << std::flush;
                    if (siyi_camera.set_bitrate(type, 4000)) {
                        std::cout << "ok" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else {
                    std::cout << "Invalid bitrate" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }

            } else if (setting == "codec") {
                if (option == "h264") {
                    std::cout << "Set " << type_str << " codec to H264..." << std::flush;
                    if (siyi_camera.set_codec(type, siyi::Camera::Codec::H264)) {
                        std::cout << "ok, power cycle camera now!" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else if (option == "h265") {
                    std::cout << "Set " << type_str << " codec to H265 Mbps..." << std::flush;
                    if (siyi_camera.set_codec(type, siyi::Camera::Codec::H265)) {
                        std::cout << "ok, power cycle camera now!" << std::endl;
                    } else {
                        std::cout << "failed" << std::endl;
                        return 1;
                    }
                } else {
                    std::cout << "Invalid codec" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                }

            } else {
                std::cout << "Unknown setting" << std::endl;
                print_usage(argv[0]);
                return 1;
            }

            std::cout << "New " << type_str << " settings:" << std::endl;
            siyi_camera.print_settings(type);

        } else {
            std::cout << "Not enough arguments" << std::endl;
            print_usage(argv[0]);
            return 1;
        }

    } else if (action == "zoom") {
        if (argc >= 3) {
            const std::string_view option{argv[2]};
            if (option == "in") {
                std::cout << "Zooming in..." << std::flush;
                if (siyi_camera.zoom(siyi::Camera::Zoom::In)) {
                    std::cout << "ok" << std::endl;
                } else {
                    std::cout << "failed" << std::endl;
                    return 1;
                }
            } else if (option == "out") {
                std::cout << "Zooming out..." << std::flush;
                if (siyi_camera.zoom(siyi::Camera::Zoom::Out)) {
                    std::cout << "ok" << std::endl;
                } else {
                    std::cout << "failed" << std::endl;
                    return 1;
                }
            } else if (option == "stop") {
                std::cout << "Stop zooming..." << std::flush;
                if (siyi_camera.zoom(siyi::Camera::Zoom::Stop)) {
                    std::cout << "ok" << std::endl;
                } else {
                    std::cout << "failed" << std::endl;
                    return 1;
                }
            } else {
                float factor;
                try {
                    factor = std::stof(option.data());
                } catch (std::invalid_argument&) {
                    std::cout << "Invalid zoom command" << std::endl;
                    print_usage(argv[0]);
                    return 1;
                };
                siyi_camera.absolute_zoom(factor);
            }
        } else {
            std::cout << "Not enough arguments" << std::endl;
            print_usage(argv[0]);
            return 1;
        }

    } else if (action == "zoom") {
        if (argc >= 3) {

        } else {
            std::cout << "Not enough arguments" << std::endl;
            print_usage(argv[0]);
            return 1;
        }

    } else {
        std::cout << "Unknown command" << std::endl;
        print_usage(argv[0]);
        return 2;
    }

    return 0;
}
