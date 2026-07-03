#include <cstdlib>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

constexpr auto MaxNumberOfBuffers = 200;

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("audio-player");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("audiotestsrc", "source");
  if(nullptr == source) {
    fmt::print(stderr, "Failed to create audiotestsrc.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", MaxNumberOfBuffers, nullptr);

  auto* convert = gst_element_factory_make("audioconvert", "converter");
  if(nullptr == convert) {
    fmt::print(stderr, "Failed to create audioconvert.\n");
    return EXIT_FAILURE;
  }

  auto* resample = gst_element_factory_make("audioresample", "resampler");
  if(nullptr == resample) {
    fmt::print(stderr, "Failed to create audioresample.\n");
    return EXIT_FAILURE;
  }

  auto* sink = gst_element_factory_make("autoaudiosink", "sink");
  if(nullptr == sink) {
    fmt::print(stderr, "Failed to create autoaudiosink.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), source) || TRUE != gst_bin_add(GST_BIN(pipeline), convert) ||
     TRUE != gst_bin_add(GST_BIN(pipeline), resample) || TRUE != gst_bin_add(GST_BIN(pipeline), sink)) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link_many(source, convert, resample, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    return EXIT_FAILURE;
  }

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* error = nullptr;
      gst_message_parse_error(msg, &error, nullptr);
      fmt::print(stderr, "Error: {}\n", error->message);
      g_error_free(error);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
