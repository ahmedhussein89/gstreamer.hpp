#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers = 150;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("element-by-hand");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("videotestsrc", "source");
  if(nullptr == source) {
    fmt::print(stderr, "Failed to create videotestsrc.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

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

  // Each gst_bin_add sinks the element's floating reference.
  // After a successful call, the pipeline owns the element — never call gst_object_unref on it.
  if(TRUE != gst_bin_add(GST_BIN(pipeline), source)) {
    fmt::print(stderr, "Failed to add source to pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  if(TRUE != gst_bin_add(GST_BIN(pipeline), convert)) {
    fmt::print(stderr, "Failed to add convert to pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  if(TRUE != gst_bin_add(GST_BIN(pipeline), sink)) {
    fmt::print(stderr, "Failed to add sink to pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link_many(source, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
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
