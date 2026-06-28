#include <cstdlib>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

constexpr auto MaxNumberOfBuffers = 90;

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("video-player");
  if(nullptr == pipeline) {
    fmt::println(stderr, "Failed to create pipeline.");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("videotestsrc", "source");
  if(nullptr == source) {
    fmt::println(stderr, "Failed to create videotestsrc.");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "pattern", 0, "num-buffers", MaxNumberOfBuffers, nullptr);

  auto* sink = gst_element_factory_make("autovideosink", "sink");
  if(nullptr == sink) {
    fmt::println(stderr, "Failed to create videotestsrc.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), source)) {
    fmt::println(stderr, "Failed to add source to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), sink)) {
    fmt::println(stderr, "Failed to add sink to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(source, sink)) {
    fmt::println(stderr, "Failed to add sink to pipeline.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to change pipeline state to PLAYING.");
    return EXIT_FAILURE;
  }

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::println(stderr, "Failed to get bus from pipeline.");
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
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

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
