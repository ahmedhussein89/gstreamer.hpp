#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

#include <pipeline.hpp>

#include "gstreamer.hpp"

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  // clang-format off
  auto result = gst::build(gst::PipelineDesc{
      gst::Node{"videotestsrc"}.prop("pattern", 0).prop("num-buffers", 90),
      gst::Node{"videoconvert"},
      gst::Node{"autovideosink"}});
  // clang-format on

  if(!result) {
    fmt::print(stderr, "Error building pipeline: {}\n", result.error());
    return 1;
  }

  const gst::Pipeline& pipeline = *result;
  std::ignore = gst::element_set_state(pipeline, GST_STATE_PLAYING);

  auto bus = gst::element_get_bus(pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);

  if(msg && gst::message_type(*msg) == gst::MessageType::Error) {
    auto parsed = gst::message_parse_error(msg->get());
    if(parsed) {
      fmt::print(stderr, "Error: {}\nDebug: {}\n", parsed->first, parsed->second);
    }
  }

  std::ignore = gst::element_set_state(pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
