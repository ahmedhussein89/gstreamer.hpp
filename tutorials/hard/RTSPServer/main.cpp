#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

namespace {

constexpr auto RunSeconds = 5;

gboolean on_timeout(gpointer user_data) {
  auto* loop = static_cast<GMainLoop*>(user_data);
  fmt::print(stdout, "Timeout reached, stopping server.\n");
  g_main_loop_quit(loop);
  return G_SOURCE_REMOVE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  // A media factory's launch string is not parsed until a client connects, so a missing
  // encoder would otherwise let this program run its full timeout and exit 0 while being
  // completely non-functional. Probe up front instead.
  GstElementFactory* encoder = gst_element_factory_find("x264enc");
  if(nullptr == encoder) {
    fmt::print(stderr, "'x264enc' not found — install gst-plugins-ugly.\n");
    return EXIT_FAILURE;
  }
  gst_object_unref(encoder);

  auto* loop = g_main_loop_new(nullptr, FALSE);

  auto* server  = gst_rtsp_server_new();
  auto* mounts  = gst_rtsp_server_get_mount_points(server);
  auto* factory = gst_rtsp_media_factory_new();

  gst_rtsp_media_factory_set_launch(factory, "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(factory), "shared", TRUE, nullptr);

  gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
  g_object_unref(mounts);

  if(0 == gst_rtsp_server_attach(server, nullptr)) {
    fmt::print(stderr, "Failed to attach RTSP server to the main context.\n");
    g_object_unref(server);
    g_main_loop_unref(loop);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Stream ready at rtsp://127.0.0.1:8554/test\n");
  fmt::print(stdout, "Running for {} seconds, then exiting on its own.\n", RunSeconds);

  g_timeout_add_seconds(RunSeconds, on_timeout, loop);
  g_main_loop_run(loop);

  g_object_unref(server);
  g_main_loop_unref(loop);

  return EXIT_SUCCESS;
}
