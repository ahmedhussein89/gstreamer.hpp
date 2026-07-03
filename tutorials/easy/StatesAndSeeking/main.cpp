#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers = 600;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("states-seeking");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* sink     = gst_element_factory_make("autovideosink", "sink");

  if(nullptr == pipeline || nullptr == source || nullptr == convert || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, nullptr);
  if(TRUE != gst_element_link_many(source, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Cycle through states explicitly to show each transition.
  gst_element_set_state(pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  gst_element_set_state(pipeline, GST_STATE_PAUSED);
  // Block until PAUSED is reached so the clock is assigned.
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: READY → PAUSED\n");

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  // Query current position and duration.
  gint64 pos = 0;
  gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos);
  fmt::print(stdout, "Position: {:.3f}s\n", static_cast<double>(pos) / GST_SECOND);

  gint64 dur = -1;
  if(gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur)) {
    fmt::print(stdout, "Duration: {:.3f}s\n", static_cast<double>(dur) / GST_SECOND);
  } else {
    fmt::print(stdout, "Duration: unknown (videotestsrc has no fixed duration)\n");
  }

  // Seek to 2 s. GST_SEEK_FLAG_FLUSH discards buffered data for an immediate effect.
  const gint64 seek_pos = 2 * GST_SECOND;
  if(gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
         static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), seek_pos)) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(seek_pos) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
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
