// ${CMAKE_SOURCE_DIR}/tutorials/hard/CustomPlugin/main.cpp
#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

#include "myedgedetector.h"

namespace {
constexpr auto NumBuffers = 100;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(TRUE != my_edge_detector_register()) {
    fmt::print(stderr, "Failed to register '{}' element.\n", MY_EDGE_DETECTOR_NAME);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("custom-plugin");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* detector = gst_element_factory_make(MY_EDGE_DETECTOR_NAME, "detector");
  auto* sink     = gst_element_factory_make("fakesink", "sink");

  if(nullptr == pipeline || nullptr == source || nullptr == convert || nullptr == detector || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, detector, sink, nullptr);
  if(TRUE != gst_element_link_many(source, convert, detector, sink, nullptr)) {
    fmt::print(stderr, "Failed to link pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running with custom '{}' element...\n", MY_EDGE_DETECTOR_NAME);

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  if(nullptr != msg) {
    switch(GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR: {
        GError* err = nullptr;
        gst_message_parse_error(msg, &err, nullptr);
        fmt::print(stderr, "Error: {}\n", err->message);
        g_error_free(err);
        break;
      }
      case GST_MESSAGE_EOS:
        fmt::print(stdout, "End of stream reached.\n");
        break;
      default:
        break;
    }
    gst_message_unref(msg);
  }
  gst_object_unref(bus);

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
