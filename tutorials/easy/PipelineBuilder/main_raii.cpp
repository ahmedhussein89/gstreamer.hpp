#include <string_view>

#include <fmt/printf.h>

#include "gstreamer.hpp"

namespace {
constexpr std::string_view DefaultPipeline = "videotestsrc ! autovideosink";
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view pipeline_str = (argc > 1) ? argv[1] : DefaultPipeline;

  auto pipeline = gst::parse_launch(pipeline_str);
  if(!pipeline) {
    fmt::println(stderr, "Failed to parse pipeline: {}", pipeline.error()->message);
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::println(stderr, "Failed to start pipeline: {}", state.error());
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Pipeline running. Press Ctrl+C to stop.");

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::println(stderr, "Failed to get bus: {}", bus.error());
    std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto error_result = gst::message_parse_error(msg.get());
      if(error_result) {
        fmt::println(stderr, "Error: {}", error_result.value().first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::println(stdout, "End of stream reached.");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);

  return EXIT_SUCCESS;
}
