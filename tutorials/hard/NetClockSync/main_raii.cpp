#include <cstdlib>
#include <span>
#include <string>
#include <string_view>

#include <fmt/core.h>

#include <gst/gst.h>
#include <gst/net/gstnet.h>

#include "gstreamer_raii.hpp"

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

  auto pipeline = gst::raii::pipeline_new("net-clock-client");
  auto source   = gst::raii::element_factory_make("videotestsrc", "source");
  auto convert  = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink     = gst::raii::element_factory_make("fakesink", "sink");

  if(!pipeline || !source || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);
  // sync=TRUE is the whole point: without it fakesink renders as fast as buffers arrive and
  // the network clock below has no observable effect on playback.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(sink->get()), "sync", TRUE, nullptr);

  auto raw_source  = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink    = gst::raii::bin_add(*pipeline, std::move(*sink));
  if(!raw_source || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n");
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
    fmt::print(stderr, "{}\n", l.error());
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_sink); !l) {
    fmt::print(stderr, "{}\n", l.error());
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  // Use the network clock as the pipeline clock and pin an explicit base time,
  // matching how a real distributed player synchronizes to a shared reference.
  gst_pipeline_use_clock(GST_PIPELINE(pipeline->get()), net_clock);
  const GstClockTime base_time = gst_clock_get_time(net_clock);
  gst_element_set_base_time(GST_ELEMENT(pipeline->get()), base_time);

  GstClock* system_clock = gst_system_clock_obtain();
  const auto offset_ns   = static_cast<gint64>(gst_clock_get_time(net_clock)) -
                          static_cast<gint64>(gst_clock_get_time(system_clock));
  gst_object_unref(system_clock);

  fmt::print(stdout, "Net clock synced with {}:{}. base_time={:.3f}s offset-from-system-clock={}ns\n",
             host, port, static_cast<double>(base_time) / GST_SECOND, offset_ns);

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus.\n");
    gst_object_unref(net_clock);
    return EXIT_FAILURE;
  }

  auto msg = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg) {
    if(gst::MessageType::Error == gst::message_type(*msg)) {
      auto parsed = gst::message_parse_error(msg->get());
      if(parsed) { fmt::print(stderr, "Error: {}\n", parsed->first); }
    } else if(gst::MessageType::EOS == gst::message_type(*msg)) {
      fmt::print(stdout, "Client finished ({} frames synced to net clock).\n", NumBuffers);
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  gst_object_unref(net_clock);
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(argc > 1 && std::string_view(argv[1]) == "--server") {
    GstClock* clock             = gst_system_clock_obtain();
    GstNetTimeProvider* provider = start_server(clock, NetClockPort);
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
