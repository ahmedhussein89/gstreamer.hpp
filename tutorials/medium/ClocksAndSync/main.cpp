#include <chrono>
#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers = 120;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  const bool no_sync = (argc > 1 && std::string_view(argv[1]) == "nosync");

  auto* pipeline = gst_pipeline_new("clocks-sync");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* sink     = gst_element_factory_make("autovideosink","sink");

  if(nullptr == pipeline || nullptr == source || nullptr == convert || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);
  if(no_sync) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(sink), "sync", FALSE, nullptr);
    fmt::print(stdout, "sync=false: sink renders as fast as possible.\n");
  }

  gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, nullptr);
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
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

  // Query and display the pipeline clock.
  GstClock* clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));
  if(nullptr != clock) {
    const GstClockTime now = gst_clock_get_time(clock);
    fmt::print(stdout, "Clock type: {}\n", G_OBJECT_TYPE_NAME(clock));
    fmt::print(stdout, "Clock time at start: {:.3f}s\n", static_cast<double>(now) / GST_SECOND);
    gst_object_unref(clock);
  }

  const auto wall_start = std::chrono::steady_clock::now();

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  const auto wall_elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - wall_start).count();

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

  fmt::print(stdout, "Elapsed wall time: {:.3f}s ({} frames)\n", wall_elapsed, NumBuffers);

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
