#include <cstdlib>
#include <memory>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>
#include <gst/gstbus.h>
#include <gst/gstelement.h>

constexpr auto MaxNumberOfBuffers = 90;
using ptr = std::unique_ptr<GstElement, decltype(&gst_object_unref)>;
using bus_ptr = std::unique_ptr<GstBus, decltype(&gst_object_unref)>;

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  
  auto pipeline = ptr(gst_pipeline_new("video-player"), gst_object_unref);
  if(!pipeline) {
    fmt::println(stderr, "Failed to create pipeline.");
    return EXIT_FAILURE;
  }

  auto source = ptr(gst_element_factory_make("videotestsrc", "source"), gst_object_unref);
  if(!source) {
    fmt::println(stderr, "Failed to create videotestsrc.");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source.get()), "pattern", 0, "num-buffers", MaxNumberOfBuffers, nullptr);

  auto sink = ptr(gst_element_factory_make("autovideosink", "sink"), gst_object_unref);
  if(!sink) {
    fmt::println(stderr, "Failed to create videotestsrc.");
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
    fmt::println(stderr, "Failed to add sink to pipeline.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline.get(), GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to change pipeline state to PLAYING.");
    return EXIT_FAILURE;
  }

  auto bus = bus_ptr(gst_element_get_bus(pipeline.get()), gst_object_unref);
  if(!bus) {
    fmt::println(stderr, "Failed to get bus from pipeline.");
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(
      bus.get(), GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* error = nullptr;
      gst_message_parse_error(msg, &error, nullptr);
      fmt::println(stderr, "Error: {}", error->message);
      g_error_free(error);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::println(stdout, "End of stream reached.");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline.get(), GST_STATE_NULL);
  bus.reset();
  pipeline.reset();

  return EXIT_SUCCESS;
}
