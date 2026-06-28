#include <fmt/printf.h>

#include "gstreamer.hpp"

namespace {
constexpr auto MaxNumberOfBuffers = 90;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto pipeline = gst::ElementPtr(gst_pipeline_new("video-player"));
  if(!pipeline) {
    fmt::println(stderr, "Failed to create pipeline.");
    return EXIT_FAILURE;
  }

  auto source = gst::ElementPtr(gst_element_factory_make("videotestsrc", "source"));
  if(!source) {
    fmt::println(stderr, "Failed to create videotestsrc.");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source.get()), "pattern", 0, "num-buffers", MaxNumberOfBuffers, nullptr);

  auto sink = gst::ElementPtr(gst_element_factory_make("autovideosink", "sink"));
  if(!sink) {
    fmt::println(stderr, "Failed to create autovideosink.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline.get()), source.get())) {
    fmt::println(stderr, "Failed to add source to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline.get()), sink.get())) {
    fmt::println(stderr, "Failed to add sink to pipeline.");
    return EXIT_FAILURE;
  }

  // After adding elements to pipeline the pipeline will handle free the memory.
  if(TRUE != gst_element_link(source.release(), sink.release())) {
    fmt::println(stderr, "Failed to link source to sink.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline.get(), GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to change pipeline state to PLAYING.");
    return EXIT_FAILURE;
  }

  auto bus = gst::BusPtr(gst_element_get_bus(pipeline.get()));
  if(!bus) {
    fmt::println(stderr, "Failed to get bus from pipeline.");
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
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

  gst_element_set_state(pipeline.get(), GST_STATE_NULL);

  return EXIT_SUCCESS;
}
