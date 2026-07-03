#include <cstdlib>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("webcam-viewer");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("v4l2src", "source");
  if(nullptr == source) {
    fmt::print(stderr, "Failed to create v4l2src.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "device", "/dev/video0", nullptr);

  auto* convert = gst_element_factory_make("videoconvert", "convert");
  if(nullptr == convert) {
    fmt::print(stderr, "Failed to create videoconvert.\n");
    return EXIT_FAILURE;
  }

  auto* sink = gst_element_factory_make("autovideosink", "sink");
  if(nullptr == sink) {
    fmt::print(stderr, "Failed to create autovideosink.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), source)) {
    fmt::print(stderr, "Failed to add source to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), convert)) {
    fmt::print(stderr, "Failed to add convert to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_bin_add(GST_BIN(pipeline), sink)) {
    fmt::print(stderr, "Failed to add sink to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(source, convert)) {
    fmt::print(stderr, "Failed to link source to convert.\n");
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(convert, sink)) {
    fmt::print(stderr, "Failed to link convert to sink.\n");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Webcam viewer running. Press Ctrl+C to stop.\n");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);
  if(nullptr != msg) {
    GError* error = nullptr;
    gst_message_parse_error(msg, &error, nullptr);
    fmt::print(stderr, "Error: {}\n", error->message);
    g_error_free(error);
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
