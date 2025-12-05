#include <cstdlib>

#include <fmt/color.h>
#include <fmt/printf.h>

#include <gst/gst.h>
#include <gst/gstmessage.h>

#ifdef __APPLE__
#  include <TargetConditionals.h>
#endif

#include "gstreamer.hpp"

namespace {
constexpr auto PIPELINE_STR = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm";

int tutorial_main(int argc, char* argv[]) {
  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  GError* error = nullptr;
  /* Build the pipeline */
  const auto pipeline_str = fmt::format("playbin uri={}", PIPELINE_STR);
  auto pipeline_result = gst::parse_launch(pipeline_str);
  if(!pipeline_result) {
    auto error = std::move(pipeline_result.error());
    fmt::print(stderr, fmt::fg(fmt::color::red), "Failed to create pipeline: {}\n", (error) ? error->message : "unknown error");
    return EXIT_FAILURE;
  }

  auto pipeline = std::move(*pipeline_result);
  /* Start playing */
  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline.get(), GST_STATE_PLAYING)) {
    fmt::print(stderr, fmt::fg(fmt::color::red), "Unable to set the pipeline to the playing state.\n");
    return EXIT_FAILURE;
  }

  /* Wait until error or EOS */
  auto bus = gst::BusPtr(gst_element_get_bus(pipeline.get()));
  if(!bus) {
    fmt::print(stderr, fmt::fg(fmt::color::red), "Unable to get the bus from the pipeline.\n");
    if(gst_element_set_state(pipeline.get(), GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
      fmt::print(stderr, fmt::fg(fmt::color::yellow), "Unable to set the pipeline to the NULL state.\n");
    }
    return EXIT_FAILURE;
  }

  auto msg = gst::bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(!msg) {
    fmt::print(stderr, fmt::fg(fmt::color::red), "Unexpected NULL message from bus.\n");
    if(gst_element_set_state(pipeline.get(), GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
      fmt::print(stderr, fmt::fg(fmt::color::yellow), "Unable to set the pipeline to the NULL state.\n");
    }
    return EXIT_FAILURE;
  }

  auto msg_ptr = std::move(msg.value());
  /* Parse and handle messages properly */
  if(gst::MessageType::Error == gst::message_type(msg_ptr)) {
    if(auto result = gst::message_parse_error(msg_ptr.get())) {
      const auto& [error_msg, debug_info] = result.value();
      fmt::print(
          stderr, fmt::fg(fmt::color::red), "Error received from element {}: {}\n", GST_OBJECT_NAME(msg_ptr.get()->src), error_msg);
      fmt::print(stderr, fmt::fg(fmt::color::red), "Debugging information: {}\n", debug_info);
    }
  } else if(gst::MessageType::EOS == gst::message_type(msg_ptr)) {
    fmt::print("End-Of-Stream reached.\n");
  }

  /* Free resources */
  if(gst_element_set_state(pipeline.get(), GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
    fmt::print(stderr, fmt::fg(fmt::color::yellow), "Unable to set the pipeline to the NULL state.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
}    // namespace

int main(int argc, char* argv[]) {
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
  return gst_macos_main((GstMainFunc)tutorial_main, argc, argv, nullptr);
#else
  return tutorial_main(argc, argv);
#endif
}
