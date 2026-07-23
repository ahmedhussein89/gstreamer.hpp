#include <cstdlib>
#include <string>
#include <string_view>

#include <fmt/core.h>

#include <gst/gst.h>
#include <gst/net/gstnet.h>

namespace {
constexpr gint NetClockPort         = 9998;
constexpr auto NumBuffers           = 60;
constexpr auto ServerLifetimeSeconds = 20;

// GstNetClock/GstNetTimeProvider have no gst:: wrapper -- use raw GStreamer API
GstNetTimeProvider* start_server(GstClock* clock, gint port) {
  return gst_net_time_provider_new(clock, nullptr, port);
}

int run_client(const std::string& host, gint port) {
  GstClock* net_clock = gst_net_client_clock_new("net_clock", host.c_str(), port, 0);
  if(nullptr == net_clock) {
    fmt::print(stderr, "Failed to create net client clock for {}:{}\n", host, port);
    return EXIT_FAILURE;
  }
  if(FALSE == gst_clock_wait_for_sync(net_clock, 5 * GST_SECOND)) {
    fmt::print(stderr, "Timed out waiting for clock sync with {}:{}\n", host, port);
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("net-clock-client");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* sink     = gst_element_factory_make("fakesink", "sink");

  if(nullptr == pipeline || nullptr == source || nullptr == convert || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);
  // sync=TRUE is the whole point: without it fakesink renders as fast as buffers arrive and
  // the network clock below has no observable effect on playback.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(sink), "sync", TRUE, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, nullptr);
  if(TRUE != gst_element_link_many(source, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  // Use the network clock as the pipeline clock and pin an explicit base time,
  // matching how a real distributed player synchronizes to a shared reference.
  gst_pipeline_use_clock(GST_PIPELINE(pipeline), net_clock);
  const GstClockTime base_time = gst_clock_get_time(net_clock);
  gst_element_set_base_time(GST_ELEMENT(pipeline), base_time);

  GstClock* system_clock = gst_system_clock_obtain();
  const auto offset_ns   = static_cast<gint64>(gst_clock_get_time(net_clock)) -
                          static_cast<gint64>(gst_clock_get_time(system_clock));
  gst_object_unref(system_clock);

  fmt::print(stdout, "Net clock synced with {}:{}. base_time={:.3f}s offset-from-system-clock={}ns\n",
             host, port, static_cast<double>(base_time) / GST_SECOND, offset_ns);

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::print(stdout, "Client finished ({} frames synced to net clock).\n", NumBuffers);
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);
  gst_object_unref(net_clock);
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(argc > 1 && std::string_view(argv[1]) == "--server") {
    GstClock* clock               = gst_system_clock_obtain();
    GstNetTimeProvider* provider   = start_server(clock, NetClockPort);
    if(nullptr == provider) {
      fmt::print(stderr, "Failed to start net time provider on port {}\n", NetClockPort);
      gst_object_unref(clock);
      return EXIT_FAILURE;
    }
    fmt::print(stdout, "Serving clock on 0.0.0.0:{} for {}s ...\n", NetClockPort, ServerLifetimeSeconds);
    g_usleep(static_cast<gulong>(ServerLifetimeSeconds) * G_USEC_PER_SEC);
    fmt::print(stdout, "Server done.\n");
    gst_object_unref(provider);
    gst_object_unref(clock);
    return EXIT_SUCCESS;
  }

  if(argc > 1 && std::string_view(argv[1]) == "--client") {
    if(argc < 3) {
      fmt::print(stderr, "Usage: {} --client <host>\n", argv[0]);
      return EXIT_FAILURE;
    }
    return run_client(argv[2], NetClockPort);
  }

  // Default: run both roles in one process against 127.0.0.1 -- CI-friendly, no second machine needed.
  GstClock* server_clock       = gst_system_clock_obtain();
  GstNetTimeProvider* provider = start_server(server_clock, NetClockPort);
  if(nullptr == provider) {
    fmt::print(stderr, "Failed to start net time provider on port {}\n", NetClockPort);
    gst_object_unref(server_clock);
    return EXIT_FAILURE;
  }
  fmt::print(stdout, "Serving clock on 127.0.0.1:{} (in-process loopback).\n", NetClockPort);

  const int rc = run_client("127.0.0.1", NetClockPort);

  gst_object_unref(provider);
  gst_object_unref(server_clock);
  return rc;
}
