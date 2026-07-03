#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {

constexpr auto NumBuffers = 90;
constexpr auto FrameWidth = 320;
constexpr auto FrameHeight = 240;
constexpr auto BorderWidth = 20;

void draw_border(guint8* data, int width, int height) {
  const int stride = width * 3;
  for(int y = 0; y < height; ++y) {
    for(int x = 0; x < width; ++x) {
      if(x < BorderWidth || x >= width - BorderWidth || y < BorderWidth || y >= height - BorderWidth) {
        data[y * stride + x * 3 + 0] = 255;
        data[y * stride + x * 3 + 1] = 0;
        data[y * stride + x * 3 + 2] = 0;
      }
    }
  }
}

struct AppData {
  GstElement* appsrc;
  int frame_count{0};
};

GstFlowReturn on_new_sample(GstElement* appsink, gpointer user_data) {
  auto* data = static_cast<AppData*>(user_data);
  GstSample* sample = nullptr;
  g_signal_emit_by_name(appsink, "pull-sample", &sample);
  if(nullptr == sample) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* in_buffer = gst_sample_get_buffer(sample);
  GstBuffer* out_buffer = gst_buffer_copy(in_buffer);

  GstMapInfo map;
  gst_buffer_map(out_buffer, &map, GST_MAP_READWRITE);
  draw_border(map.data, FrameWidth, FrameHeight);
  gst_buffer_unmap(out_buffer, &map);

  ++data->frame_count;

  GstFlowReturn ret = GST_FLOW_OK;
  g_signal_emit_by_name(data->appsrc, "push-buffer", out_buffer, &ret);

  gst_buffer_unref(out_buffer);
  gst_sample_unref(sample);
  return ret;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("cpu-processing");
  auto* source = gst_element_factory_make("videotestsrc", "source");
  auto* convert1 = gst_element_factory_make("videoconvert", "convert-in");
  auto* appsink = gst_element_factory_make("appsink", "appsink");
  auto* appsrc = gst_element_factory_make("appsrc", "appsrc");
  auto* convert2 = gst_element_factory_make("videoconvert", "convert-out");
  auto* display = gst_element_factory_make("autovideosink", "display");

  if(!pipeline || !source || !convert1 || !appsink || !appsrc || !convert2 || !display) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  GstCaps* caps = gst_caps_new_simple(
      "video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, FrameWidth, "height", G_TYPE_INT, FrameHeight, nullptr);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "caps", caps, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(appsrc), "caps", caps, "format", GST_FORMAT_TIME, nullptr);
  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline), source, convert1, appsink, appsrc, convert2, display, nullptr);

  if(TRUE != gst_element_link_many(source, convert1, appsink, nullptr)) {
    fmt::print(stderr, "Failed to link input chain.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  if(TRUE != gst_element_link_many(appsrc, convert2, display, nullptr)) {
    fmt::print(stderr, "Failed to link output chain.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  AppData app_data{appsrc};
  g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), &app_data);

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Processing {} frames with CPU (red border overlay).\n", NumBuffers);

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::print(stdout, "Processed {} frames.\n", app_data.frame_count);
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
