#include <chrono>
#include <cstdlib>
#include <span>
#include <string_view>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr auto NumBuffers = 120;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const bool no_sync = (argc > 1 && std::string_view(argv[1]) == "nosync");

  auto pipeline = gst::raii::pipeline_new("clocks-sync");
  auto source   = gst::raii::element_factory_make("videotestsrc", "source");
  auto convert  = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink     = gst::raii::element_factory_make("autovideosink","sink");

  if(!pipeline || !source || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);
  if(no_sync) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(sink->get()), "sync", FALSE, nullptr);
    fmt::print(stdout, "sync=false: sink renders as fast as possible.\n");
  }

  auto raw_source  = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink    = gst::raii::bin_add(*pipeline, std::move(*sink));
  if(!raw_source || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n"); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_sink); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    return EXIT_FAILURE;
  }
  gst_element_get_state(pipeline->get(), nullptr, nullptr, GST_CLOCK_TIME_NONE);

  GstClock* clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline->get()));
  if(nullptr != clock) {
    fmt::print(stdout, "Clock type: {}\n", G_OBJECT_TYPE_NAME(clock));
    fmt::print(stdout, "Clock time at start: {:.3f}s\n",
               static_cast<double>(gst_clock_get_time(clock)) / GST_SECOND);
    gst_object_unref(clock);
  }

  const auto wall_start = std::chrono::steady_clock::now();

  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) { fmt::print(stderr, "Failed to get bus.\n"); return EXIT_FAILURE; }

  auto msg = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);

  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall_start).count();

  if(msg) {
    if(gst::MessageType::Error == gst::message_type(*msg)) {
      auto parsed = gst::message_parse_error(msg->get());
      if(parsed) { fmt::print(stderr, "Error: {}\n", parsed->first); }
    } else if(gst::MessageType::EOS == gst::message_type(*msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  fmt::print(stdout, "Elapsed wall time: {:.3f}s ({} frames)\n", elapsed, NumBuffers);
  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
