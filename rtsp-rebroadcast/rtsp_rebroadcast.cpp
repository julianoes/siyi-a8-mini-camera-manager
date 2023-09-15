#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>

// RTSP re-broadcast using gstreamer.
//
// This application subscribes to the RTSP stream of the SIYI A8 mini
// and directly rebroadcasts the H265 stream as an RTSP server.
//
// Source mostly taken from:
// https://github.com/JonasVautherin/px4-gazebo-headless/tree/master/sitl_rtsp_proxy

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    GMainLoop* main_loop = g_main_loop_new(NULL, false);

    if (!gst_debug_is_active()) {
        gst_debug_set_active(TRUE);
        gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    }

    GstRTSPServer* server = gst_rtsp_server_new();
    g_object_set(server, "service", "8554", NULL);

    static constexpr auto launch_string =
        "rtspsrc location=rtsp://192.168.144.25:8554/main.264 latency=0 "
        "! rtph265depay "
        "! rtph265pay name=pay0 pt=96";

    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, launch_string);
    gst_rtsp_media_factory_set_shared(factory, true);

    GstRTSPMountPoints* mount_points = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_add_factory(mount_points, "/live", factory);
    g_object_unref(mount_points);

    gst_rtsp_server_attach(server, NULL);

    g_main_loop_run(main_loop);
}
