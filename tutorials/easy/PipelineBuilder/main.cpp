#include <cstdlib>
#include <string_view>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

namespace {
constexpr std::string_view DefaultPipeline = "videotestsrc ! autovideosink";
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  const char* pipeline_str = (argc > 1) ? argv[1] : DefaultPipeline.data();

  GError* error = nullptr;
  auto* pipeline = gst_parse_launch(pipeline_str, &error);
  if(nullptr != error) {
    fmt::print(stderr, "Failed to parse pipeline: {}\n", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to build pipeline.\n");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running. Press Ctrl+C to stop.\n");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
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
