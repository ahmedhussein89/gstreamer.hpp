#include <atomic>
#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {

constexpr auto NumBuffers  = 10;
constexpr auto FrameWidth  = 320;
constexpr auto FrameHeight = 240;

std::atomic<int> g_frame_count{0};

GstPadProbeReturn on_buffer(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer /*user_data*/) {
  GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_READ);

  const int  n         = g_frame_count.fetch_add(1);
  const auto pts_s     = GST_BUFFER_PTS_IS_VALID(buf)
                             ? static_cast<double>(GST_BUFFER_PTS(buf)) / GST_SECOND
                             : -1.0;
  const auto dur_s     = GST_BUFFER_DURATION_IS_VALID(buf)
                             ? static_cast<double>(GST_BUFFER_DURATION(buf)) / GST_SECOND
                             : -1.0;
  const auto flags     = static_cast<unsigned>(GST_BUFFER_FLAGS(buf));
  const auto mem_count = gst_buffer_n_memory(buf);

  fmt::print("Buffer {:2d}: size={} pts={:.3f}s dur={:.3f}s flags={:#010x} mem_blocks={}\n",
             n, map.size, pts_s, dur_s, flags, mem_count);

  gst_buffer_unmap(buf, &map);
  return GST_PAD_PROBE_OK;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline   = gst_pipeline_new("buffers-memory");
  auto* source     = gst_element_factory_make("videotestsrc", "source");
  auto* convert    = gst_element_factory_make("videoconvert", "convert");
  auto* capsfilter = gst_element_factory_make("capsfilter",   "cap");
  auto* sink       = gst_element_factory_make("autovideosink","sink");

  if(nullptr == pipeline || nullptr == source || nullptr == convert ||
     nullptr == capsfilter || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

  GstCaps* caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      "width",  G_TYPE_INT,    FrameWidth,
      "height", G_TYPE_INT,    FrameHeight,
      nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(capsfilter), "caps", caps, nullptr);
  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, capsfilter, sink, nullptr);
  if(TRUE != gst_element_link_many(source, convert, capsfilter, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Install a buffer probe on the capsfilter src pad to inspect each buffer.
  auto* probe_pad = gst_element_get_static_pad(capsfilter, "src");
  gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
      reinterpret_cast<GstPadProbeCallback>(on_buffer), nullptr, nullptr);
  gst_object_unref(probe_pad);

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
