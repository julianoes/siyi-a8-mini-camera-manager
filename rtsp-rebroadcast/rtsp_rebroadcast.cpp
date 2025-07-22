#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>
#include <iostream>
#include <cstring>

// RTSP re-broadcast using gstreamer.
//
// This application subscribes to the RTSP stream of the SIYI A8 mini
// and directly rebroadcasts the H265 stream as an RTSP server.
//
// Source mostly taken from:
// https://github.com/JonasVautherin/px4-gazebo-headless/tree/master/sitl_rtsp_proxy

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " --codec <h264|h265> [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --codec <h264|h265>  Video codec to use (required)\n";
    std::cout << "  --help               Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string codec;
    bool codec_specified = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--codec") == 0) {
            if (i + 1 < argc) {
                codec = argv[i + 1];
                if (codec != "h264" && codec != "h265") {
                    std::cerr << "Error: Invalid codec '" << codec << "'. Use 'h264' or 'h265'.\n";
                    return 1;
                }
                codec_specified = true;
                i++;
            } else {
                std::cerr << "Error: --codec requires an argument\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!codec_specified) {
        std::cerr << "Error: --codec argument is required\n";
        print_usage(argv[0]);
        return 1;
    }

    gst_init(&argc, &argv);

    GMainLoop* main_loop = g_main_loop_new(NULL, false);

    if (!gst_debug_is_active()) {
        gst_debug_set_active(TRUE);
        gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    }

    GstRTSPServer* server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", NULL);

    std::string launch_string;
    if (codec == "h264") {
        launch_string =
            "v4l2src device=/dev/video0 ! video/x-raw, format=I420, width=1024, height=576, framerate=30/1 "
            "! x264enc"
            "! rtph264pay name=pay0 pt=96";
    } else if (codec == "h265") {
        launch_string =
            "v4l2src device=/dev/video0 ! video/x-raw, format=I420, width=1024, height=576, framerate=30/1 "
            "! x265enc"
            "! rtph265pay name=pay0 pt=97";
    } else {
        std::cerr << "Error: Unexpected codec value '" << codec << "'\n";
        return 1;
    }

    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, launch_string.c_str());
    gst_rtsp_media_factory_set_shared(factory, true);

    GstRTSPMountPoints* mount_points = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_add_factory(mount_points, "/live", factory);
    g_object_unref(mount_points);

    gst_rtsp_server_attach(server, NULL);

    g_main_loop_run(main_loop);
}
