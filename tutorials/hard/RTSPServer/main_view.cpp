#include <cstdlib>
#include <span>
#include <string>
#include <utility>

#include <fmt/core.h>

#include "gstreamer.hpp"
#include "gstreamer_rtsp.hpp"

// GstRTSPServer / GstRTSPMediaFactory / GstRTSPMountPoints go through the thin gst::rtsp_*
// wrappers in gstreamer_rtsp.hpp (owning smart pointers, no gst::raii::* class of their own).
// GMainLoop and the "shared" property set below have no gst:: wrapper — raw GLib API.
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
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  // A media factory's launch string is not parsed until a client connects, so a missing
  // encoder would otherwise let this program run its full timeout and exit 0 while being
  // completely non-functional. Probe up front instead.
  auto encoder = gst::element_factory_find("x264enc");
  if(!encoder) {
    fmt::print(stderr, "'x264enc' not found — install gst-plugins-ugly.\n");
    return EXIT_FAILURE;
  }

  auto* loop = g_main_loop_new(nullptr, FALSE);

  auto server = gst::rtsp_server_new();

  auto mounts = gst::rtsp_server_get_mount_points(server.get());
  if(!mounts) {
    fmt::print(stderr, "{}\n", mounts.error());
    g_main_loop_unref(loop);
    return EXIT_FAILURE;
  }

  auto factory = gst::rtsp_media_factory_new();
  gst::rtsp_media_factory_set_launch(factory.get(), "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(factory.get()), "shared", TRUE, nullptr);

  // Transfers factory ownership into mounts.
  gst::rtsp_mount_points_add_factory(mounts->get(), "/test", std::move(factory));

  auto attached = gst::rtsp_server_attach(server.get(), nullptr);
  if(!attached) {
    fmt::print(stderr, "{}\n", attached.error());
    g_main_loop_unref(loop);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Stream ready at rtsp://127.0.0.1:8554/test\n");
  fmt::print(stdout, "Running for {} seconds, then exiting on its own.\n", RunSeconds);

  g_timeout_add_seconds(RunSeconds, on_timeout, loop);
  g_main_loop_run(loop);

  g_main_loop_unref(loop);

  return EXIT_SUCCESS;
}
