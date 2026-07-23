// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomPlugin/main_view.cpp
#include <cstdlib>

#include <fmt/core.h>

#include "gstreamer.hpp"
#include "myedgedetector.h"

namespace {
constexpr auto NumBuffers = 100;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(TRUE != my_edge_detector_register()) {
    fmt::print(stderr, "Failed to register '{}' element.\n", MY_EDGE_DETECTOR_NAME);
    return EXIT_FAILURE;
  }

  auto pipeline = gst::pipeline_new("custom-plugin-view");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source   = gst::element_factory_make("videotestsrc", "source");
  auto convert  = gst::element_factory_make("videoconvert", "convert");
  auto detector = gst::element_factory_make(MY_EDGE_DETECTOR_NAME, "detector");
  auto sink     = gst::element_factory_make("fakesink", "sink");

  if(!source || !convert || !detector || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto raw_source   = gst::bin_add(*pipeline, std::move(*source));
  auto raw_convert  = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_detector = gst::bin_add(*pipeline, std::move(*detector));
  auto raw_sink     = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_convert || !raw_detector || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link source to convert: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_detector); !link) {
    fmt::print(stderr, "Failed to link convert to detector: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_detector, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link detector to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running with custom '{}' element...\n", MY_EDGE_DETECTOR_NAME);

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::print(stderr, "Error: {}\n", parsed->first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
