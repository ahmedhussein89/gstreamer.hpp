#include <cstdlib>
#include <string>

#include <fmt/core.h>
#include <fmt/format.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers  = 150;
constexpr auto CellWidth   = 320;
constexpr auto CellHeight  = 240;
constexpr auto NumCameras  = 3;

struct Cell {
  gint pattern;
  gint xpos;
  gint ypos;
};

// 3-camera grid: two on the top row, one on the bottom-left. The bottom-right
// quadrant is simply left uncovered by the background color.
constexpr Cell Cells[NumCameras] = {
    {/*pattern=*/0, /*xpos=*/0, /*ypos=*/0},                        // smpte bars, top-left
    {/*pattern=*/1, /*xpos=*/CellWidth, /*ypos=*/0},                // snow, top-right
    {/*pattern=*/2, /*xpos=*/0, /*ypos=*/CellHeight},               // black, bottom-left
};

// Request pads must be handed back to the element, not merely unreffed. Every early
// return below has to undo the pads acquired on previous loop iterations.
void release_request_pads(GstElement* compositor, GstPad* const* pads, int count) {
  for(int i = 0; i < count; ++i) {
    if(nullptr != pads[i]) {
      gst_element_release_request_pad(compositor, pads[i]);
      gst_object_unref(pads[i]);
    }
  }
}
}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline   = gst_pipeline_new("multi-camera-viewer");
  auto* compositor = gst_element_factory_make("compositor", "compositor");
  auto* convert    = gst_element_factory_make("videoconvert", "convert");
  auto* sink       = gst_element_factory_make("fakesink", "sink");

  if(!pipeline || !compositor || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-bad/base provide 'compositor'.\n");
    return EXIT_FAILURE;
  }

  gst_bin_add_many(GST_BIN(pipeline), compositor, convert, sink, nullptr);

  if(TRUE != gst_element_link_many(compositor, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link compositor -> convert -> sink.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  GstElement* sources[NumCameras] = {};
  GstElement* converts[NumCameras] = {};
  GstPad* sink_pads[NumCameras] = {};

  for(int i = 0; i < NumCameras; ++i) {
    const std::string src_name = fmt::format("source{}", i);
    const std::string conv_name = fmt::format("convert{}", i);

    sources[i]  = gst_element_factory_make("videotestsrc", src_name.c_str());
    converts[i] = gst_element_factory_make("videoconvert", conv_name.c_str());

    if(!sources[i] || !converts[i]) {
      fmt::print(stderr, "Failed to create source/convert for camera {}.\n", i);
      if(sources[i]) { gst_object_unref(sources[i]); }
      if(converts[i]) { gst_object_unref(converts[i]); }
      release_request_pads(compositor, sink_pads, i);
      gst_object_unref(pipeline);
      return EXIT_FAILURE;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(sources[i]), "pattern", Cells[i].pattern, "num-buffers", NumBuffers, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), sources[i], converts[i], nullptr);

    if(TRUE != gst_element_link(sources[i], converts[i])) {
      fmt::print(stderr, "Failed to link source to convert for camera {}.\n", i);
      release_request_pads(compositor, sink_pads, i);
      gst_object_unref(pipeline);
      return EXIT_FAILURE;
    }

    // compositor sink pads are request pads with per-pad xpos/ypos/width/height
    // properties — no gst:: wrapper for request pads, so use raw GStreamer API.
    sink_pads[i] = gst_element_request_pad_simple(compositor, "sink_%u");
    if(sink_pads[i] == nullptr) {
      fmt::print(stderr, "Failed to request compositor sink pad for camera {}.\n", i);
      release_request_pads(compositor, sink_pads, i);
      gst_object_unref(pipeline);
      return EXIT_FAILURE;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(sink_pads[i]), "xpos", Cells[i].xpos, "ypos", Cells[i].ypos,
                 "width", CellWidth, "height", CellHeight, nullptr);

    GstPad* src_pad = gst_element_get_static_pad(converts[i], "src");
    if(GST_PAD_LINK_OK != gst_pad_link(src_pad, sink_pads[i])) {
      fmt::print(stderr, "Failed to link convert{} to compositor.\n", i);
      gst_object_unref(src_pad);
      release_request_pads(compositor, sink_pads, i + 1);
      gst_object_unref(pipeline);
      return EXIT_FAILURE;
    }
    gst_object_unref(src_pad);
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    release_request_pads(compositor, sink_pads, NumCameras);
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Compositing {} cameras into one output ({} frames each).\n", NumCameras, NumBuffers);

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
      fmt::print(stdout, "Composite stream finished.\n");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  release_request_pads(compositor, sink_pads, NumCameras);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
