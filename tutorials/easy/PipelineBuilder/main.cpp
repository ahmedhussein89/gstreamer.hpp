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
    fmt::println(stderr, "Failed to parse pipeline: {}", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }
  if(nullptr == pipeline) {
    fmt::println(stderr, "Failed to build pipeline.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to change pipeline state to PLAYING.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Pipeline running. Press Ctrl+C to stop.");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::println(stderr, "Failed to get bus from pipeline.");
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
      fmt::println(stderr, "Error: {}", err->message);
      g_error_free(err);
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
