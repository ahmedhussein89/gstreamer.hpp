#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {

void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* convert    = static_cast<GstElement*>(user_data);
  GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
  if(gst_pad_is_linked(sink_pad)) {
    gst_object_unref(sink_pad);
    return;
  }

  GstCaps*      caps = gst_pad_get_current_caps(new_pad);
  GstStructure* s    = gst_caps_get_structure(caps, 0);
  const char*   name = gst_structure_get_name(s);
  gst_caps_unref(caps);

  if(!g_str_has_prefix(name, "video/x-raw")) {
    gst_object_unref(sink_pad);
    return;
  }

  if(GST_PAD_LINK_OK != gst_pad_link(new_pad, sink_pad)) {
    fmt::print(stderr, "Failed to link decoded pad to videoconvert.\n");
  }
  gst_object_unref(sink_pad);
}

struct PipelineData {
  GstElement* decodebin;
};

void on_rtspsrc_pad_added(GstElement* /*rtspsrc*/, GstPad* new_pad, gpointer user_data) {
  auto* data = static_cast<PipelineData*>(user_data);
  GstCaps*      caps  = gst_pad_get_current_caps(new_pad);
  GstStructure* s     = gst_caps_get_structure(caps, 0);
  const char*   media = gst_structure_get_string(s, "media");
  gst_caps_unref(caps);

  if(g_strcmp0(media, "video") != 0) {
    return;
  }

  GstPad* sink_pad = gst_element_get_static_pad(data->decodebin, "sink");
  if(!gst_pad_is_linked(sink_pad)) {
    if(GST_PAD_LINK_OK != gst_pad_link(new_pad, sink_pad)) {
      fmt::print(stderr, "Failed to link rtspsrc pad to decodebin.\n");
    }
  }
  gst_object_unref(sink_pad);
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <rtsp-url>\n", argv[0]);
    fmt::print(stderr, "Example: {} rtsp://example.com/stream\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("rtsp-client");
  auto* source   = gst_element_factory_make("rtspsrc", "source");
  auto* decode   = gst_element_factory_make("decodebin", "decoder");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* sink     = gst_element_factory_make("autovideosink", "display");

  if(!pipeline || !source || !decode || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "location", argv[1], "latency", 200, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, decode, convert, sink, nullptr);

  // Convert and sink are linked statically; rtspsrc and decodebin use dynamic pads
  if(TRUE != gst_element_link(convert, sink)) {
    fmt::print(stderr, "Failed to link convert to sink.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  PipelineData data{decode};
  g_signal_connect(source, "pad-added", G_CALLBACK(on_rtspsrc_pad_added), &data);
  g_signal_connect(decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), convert);

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Connecting to {}. Press Ctrl+C to stop.\n", argv[1]);

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
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
