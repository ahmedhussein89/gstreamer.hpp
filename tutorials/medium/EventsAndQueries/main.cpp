#include <cstdlib>

#include <fmt/format.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers = 300;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("events-queries");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("videotestsrc", "source");
  if(nullptr == source) {
    fmt::print(stderr, "Failed to create source element.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

  auto* convert = gst_element_factory_make("videoconvert", "convert");
  if(nullptr == convert) {
    fmt::print(stderr, "Failed to create convert element.\n");
    return EXIT_FAILURE;
  }

  auto* sink = gst_element_factory_make("autovideosink", "sink");
  if(nullptr == sink) {
    fmt::print(stderr, "Failed to create sink element.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
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
  // Wait for state transition to complete so the clock is running.
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

  // --- Position query ---
  gint64 pos = 0;
  if(TRUE == gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)) {
    fmt::print(stdout, "Position: {:.3f}s\n", static_cast<double>(pos) / GST_SECOND);
  } else {
    fmt::print(stdout, "Position: unknown\n");
  }

  // --- Duration query ---
  gint64 dur = -1;
  if(TRUE == gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur)) {
    fmt::print(stdout, "Duration: {:.3f}s\n", static_cast<double>(dur) / GST_SECOND);
  } else {
    fmt::print(stdout, "Duration: unknown (videotestsrc has no fixed duration)\n");
  }

  // --- Latency query ---
  GstQuery* latency_query = gst_query_new_latency();
  if(TRUE == gst_element_query(pipeline, latency_query)) {
    gboolean live = FALSE;
    GstClockTime min_lat = 0;
    GstClockTime max_lat = 0;
    gst_query_parse_latency(latency_query, &live, &min_lat, &max_lat);
    fmt::print(stdout,
               "Latency: live={} min={:.3f}ms{}\n",
               (live != 0) ? "yes" : "no",
               static_cast<double>(min_lat) / GST_MSECOND,
               max_lat == GST_CLOCK_TIME_NONE ? "" : fmt::format(" max={:.3f}ms", static_cast<double>(max_lat) / GST_MSECOND));
  }
  gst_query_unref(latency_query);

  auto* bus = gst_element_get_bus(pipeline);

  // Wait for EOS or ERROR message.
  if(auto* msg = gst_bus_timed_pop_filtered(
         bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
     nullptr != msg) {
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
