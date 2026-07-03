#include <fmt/printf.h>

#include "gstreamer_raii.hpp"

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::raii::pipeline_new("webcam-viewer");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::raii::element_factory_make("v4l2src", "source");
  if(!source) {
    fmt::print(stderr, "{}\n", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "device", "/dev/video0", nullptr);

  auto convert = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink    = gst::raii::element_factory_make("autovideosink", "sink");

  if(!convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  auto raw_source  = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink    = gst::raii::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link source to convert: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link convert to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Webcam viewer running. Press Ctrl+C to stop.\n");

  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error);
  if(msg_result) {
    auto error_result = gst::message_parse_error(msg_result.value().get());
    if(error_result) {
      fmt::print(stderr, "Error: {}\n", error_result.value().first);
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline destructor calls gst_object_unref

  return EXIT_SUCCESS;
}
