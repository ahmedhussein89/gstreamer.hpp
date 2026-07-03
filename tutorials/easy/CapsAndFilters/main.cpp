#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers  = 90;
constexpr auto FrameWidth  = 320;
constexpr auto FrameHeight = 240;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline   = gst_pipeline_new("caps-filters");
  auto* source     = gst_element_factory_make("videotestsrc", "source");
  auto* capsfilter = gst_element_factory_make("capsfilter",   "cap");
  auto* convert    = gst_element_factory_make("videoconvert", "convert");
  auto* sink       = gst_element_factory_make("autovideosink","sink");

  if(nullptr == pipeline || nullptr == source || nullptr == capsfilter ||
     nullptr == convert  || nullptr == sink) {
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

  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, convert, sink, nullptr);
  if(TRUE != gst_element_link_many(source, capsfilter, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Let caps negotiate before querying.
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

  auto* src_pad = gst_element_get_static_pad(capsfilter, "src");
  auto* neg_caps = gst_pad_get_current_caps(src_pad);
  if(nullptr != neg_caps) {
    gchar* caps_str = gst_caps_to_string(neg_caps);
    fmt::print(stdout, "Negotiated caps: {}\n", caps_str);
    g_free(caps_str);
    gst_caps_unref(neg_caps);
  }
  gst_object_unref(src_pad);

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
