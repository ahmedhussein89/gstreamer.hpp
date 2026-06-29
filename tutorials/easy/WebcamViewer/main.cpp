#include <cstdlib>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("webcam-viewer");
  if(nullptr == pipeline) {
    fmt::println(stderr, "Failed to create pipeline.");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("v4l2src", "source");
  if(nullptr == source) {
    fmt::println(stderr, "Failed to create v4l2src.");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "device", "/dev/video0", nullptr);

  auto* convert = gst_element_factory_make("videoconvert", "convert");
  if(nullptr == convert) {
    fmt::println(stderr, "Failed to create videoconvert.");
    return EXIT_FAILURE;
  }

  auto* sink = gst_element_factory_make("autovideosink", "sink");
  if(nullptr == sink) {
    fmt::println(stderr, "Failed to create autovideosink.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), source)) {
    fmt::println(stderr, "Failed to add source to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), convert)) {
    fmt::println(stderr, "Failed to add convert to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), sink)) {
    fmt::println(stderr, "Failed to add sink to pipeline.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(source, convert)) {
    fmt::println(stderr, "Failed to link source to convert.");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(convert, sink)) {
    fmt::println(stderr, "Failed to link convert to sink.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to change pipeline state to PLAYING.");
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Webcam viewer running. Press Ctrl+C to stop.");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::println(stderr, "Failed to get bus from pipeline.");
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);
  if(nullptr != msg) {
    GError* error = nullptr;
    gst_message_parse_error(msg, &error, nullptr);
    fmt::println(stderr, "Error: {}", error->message);
    g_error_free(error);
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
