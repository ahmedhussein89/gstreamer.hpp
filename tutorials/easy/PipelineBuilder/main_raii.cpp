#include <string_view>

#include <fmt/printf.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr std::string_view DefaultPipeline = "videotestsrc ! autovideosink";
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view pipeline_str = (argc > 1) ? argv[1] : DefaultPipeline;

  auto pipeline = gst::raii::parse_launch(pipeline_str);
  if(!pipeline) {
    fmt::print(stderr, "Failed to parse pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running. Press Ctrl+C to stop.\n");

  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto error_result = gst::message_parse_error(msg.get());
      if(error_result) {
        fmt::print(stderr, "Error: {}\n", error_result.value().first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline destructor calls gst_object_unref

  return EXIT_SUCCESS;
}
