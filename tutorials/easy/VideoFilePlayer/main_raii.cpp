#include <fmt/printf.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr auto MaxNumberOfBuffers = 90;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  // gst::raii::pipeline_new — returns an owning Pipeline (freed on scope exit)
  auto pipeline = gst::raii::pipeline_new("video-player");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::raii::element_factory_make("videotestsrc", "source");
  if(!source) {
    fmt::print(stderr, "{}\n", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "pattern", 0, "num-buffers", MaxNumberOfBuffers, nullptr);

  auto sink = gst::raii::element_factory_make("autovideosink", "sink");
  if(!sink) {
    fmt::print(stderr, "{}\n", sink.error());
    return EXIT_FAILURE;
  }

  // raii::bin_add transfers ownership of the element into the pipeline bin.
  // Returns a non-owning gst::Element handle for linking (bin now owns it).
  auto raw_source = gst::raii::bin_add(*pipeline, std::move(*source));
  if(!raw_source) {
    fmt::print(stderr, "Failed to add source to pipeline: {}\n", raw_source.error());
    return EXIT_FAILURE;
  }

  auto raw_sink = gst::raii::bin_add(*pipeline, std::move(*sink));
  if(!raw_sink) {
    fmt::print(stderr, "Failed to add sink to pipeline: {}\n", raw_sink.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link source to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  // raii::element_get_bus returns an owning Bus (freed on scope exit)
  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(
      *bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
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
  // *pipeline goes out of scope here — gst::raii::Pipeline destructor
  // calls gst_object_unref, which in turn unrefs all contained elements.

  return EXIT_SUCCESS;
}
