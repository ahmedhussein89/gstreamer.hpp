#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {
constexpr auto NumBuffers = 150;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("element-by-hand");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::element_factory_make("videotestsrc", "source");
  if(!source) {
    fmt::print(stderr, "{}\n", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto convert = gst::element_factory_make("videoconvert", "convert");
  if(!convert) {
    fmt::print(stderr, "{}\n", convert.error());
    return EXIT_FAILURE;
  }

  auto sink = gst::element_factory_make("autovideosink", "sink");
  if(!sink) {
    fmt::print(stderr, "{}\n", sink.error());
    return EXIT_FAILURE;
  }

  auto raw_source = gst::bin_add(*pipeline, *source);
  if(!raw_source) {
    fmt::print(stderr, "Failed to add source: {}\n", raw_source.error());
    return EXIT_FAILURE;
  }
  auto raw_convert = gst::bin_add(*pipeline, *convert);
  if(!raw_convert) {
    fmt::print(stderr, "Failed to add convert: {}\n", raw_convert.error());
    return EXIT_FAILURE;
  }
  auto raw_sink = gst::bin_add(*pipeline, *sink);
  if(!raw_sink) {
    fmt::print(stderr, "Failed to add sink: {}\n", raw_sink.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link source→convert: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link convert→sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg) {
    if(gst::MessageType::Error == gst::message_type(*msg)) {
      auto parsed = gst::message_parse_error(msg->get());
      if(parsed) { fmt::print(stderr, "Error: {}\n", parsed->first); }
    } else if(gst::MessageType::EOS == gst::message_type(*msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
