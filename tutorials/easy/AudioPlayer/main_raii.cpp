#include <fmt/printf.h>

#include "gstreamer.hpp"

namespace {
constexpr auto MaxNumberOfBuffers = 200;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("audio-player");
  if(!pipeline) {
    fmt::println(stderr, "Failed to create pipeline: {}", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::element_factory_make("audiotestsrc", "source");
  if(!source) {
    fmt::println(stderr, "{}", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", MaxNumberOfBuffers, nullptr);

  auto convert = gst::element_factory_make("audioconvert", "converter");
  if(!convert) {
    fmt::println(stderr, "{}", convert.error());
    return EXIT_FAILURE;
  }

  auto resample = gst::element_factory_make("audioresample", "resampler");
  if(!resample) {
    fmt::println(stderr, "{}", resample.error());
    return EXIT_FAILURE;
  }

  auto sink = gst::element_factory_make("autoaudiosink", "sink");
  if(!sink) {
    fmt::println(stderr, "{}", sink.error());
    return EXIT_FAILURE;
  }

  auto raw_source   = gst::bin_add(*pipeline, std::move(*source));
  auto raw_convert  = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_resample = gst::bin_add(*pipeline, std::move(*resample));
  auto raw_sink     = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_convert || !raw_resample || !raw_sink) {
    fmt::println(stderr, "Failed to add elements to pipeline.");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::println(stderr, "Failed to link source to converter: {}", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_convert, *raw_resample); !link) {
    fmt::println(stderr, "Failed to link converter to resampler: {}", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_resample, *raw_sink); !link) {
    fmt::println(stderr, "Failed to link resampler to sink: {}", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::println(stderr, "Failed to start pipeline: {}", state.error());
    return EXIT_FAILURE;
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::println(stderr, "Failed to get bus: {}", bus.error());
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
