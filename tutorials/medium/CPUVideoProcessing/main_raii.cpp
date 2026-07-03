#include <cstdlib>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

constexpr auto NumBuffers  = 90;
constexpr auto FrameWidth  = 320;
constexpr auto FrameHeight = 240;
constexpr auto BorderWidth = 20;

void draw_border(guint8* data, int width, int height) {
  const int stride = width * 3;
  for(int y = 0; y < height; ++y) {
    for(int x = 0; x < width; ++x) {
      if(x < BorderWidth || x >= width - BorderWidth ||
         y < BorderWidth || y >= height - BorderWidth) {
        data[y * stride + x * 3 + 0] = 255;
        data[y * stride + x * 3 + 1] = 0;
        data[y * stride + x * 3 + 2] = 0;
      }
    }
  }
}

struct AppData {
  GstElement* appsrc;
  int         frame_count{0};
};

GstFlowReturn on_new_sample(GstElement* appsink, AppData* data) {
  GstSample* sample = nullptr;
  g_signal_emit_by_name(appsink, "pull-sample", &sample);
  if(nullptr == sample) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* in_buffer  = gst_sample_get_buffer(sample);
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
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("cpu-processing");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source   = gst::element_factory_make("videotestsrc", "source");
  auto convert1 = gst::element_factory_make("videoconvert", "convert-in");
  auto appsink  = gst::element_factory_make("appsink", "appsink");
  auto appsrc   = gst::element_factory_make("appsrc", "appsrc");
  auto convert2 = gst::element_factory_make("videoconvert", "convert-out");
  auto display  = gst::element_factory_make("autovideosink", "display");

  if(!source || !convert1 || !appsink || !appsrc || !convert2 || !display) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  GstCaps* caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      "width",  G_TYPE_INT,    FrameWidth,
      "height", G_TYPE_INT,    FrameHeight,
      nullptr);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()),  "num-buffers",  NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(appsink->get()), "emit-signals", TRUE, "caps", caps, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(appsrc->get()),  "caps",         caps, "format", GST_FORMAT_TIME, nullptr);
  gst_caps_unref(caps);

  auto raw_source   = gst::bin_add(*pipeline, std::move(*source));
  auto raw_convert1 = gst::bin_add(*pipeline, std::move(*convert1));
  auto raw_appsink  = gst::bin_add(*pipeline, std::move(*appsink));
  auto raw_appsrc   = gst::bin_add(*pipeline, std::move(*appsrc));
  auto raw_convert2 = gst::bin_add(*pipeline, std::move(*convert2));
  auto raw_display  = gst::bin_add(*pipeline, std::move(*display));

  if(!raw_source || !raw_convert1 || !raw_appsink || !raw_appsrc || !raw_convert2 || !raw_display) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert1); !link) {
    fmt::print(stderr, "Failed to link source to convert-in: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert1, *raw_appsink); !link) {
    fmt::print(stderr, "Failed to link convert-in to appsink: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_appsrc, *raw_convert2); !link) {
    fmt::print(stderr, "Failed to link appsrc to convert-out: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert2, *raw_display); !link) {
    fmt::print(stderr, "Failed to link convert-out to display: {}\n", link.error());
    return EXIT_FAILURE;
  }

  AppData app_data{*raw_appsrc};
  g_signal_connect(*raw_appsink, "new-sample", G_CALLBACK(on_new_sample), &app_data);

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Processing {} frames with CPU (red border overlay).\n", NumBuffers);

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::print(stderr, "Error: {}\n", parsed->first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "Processed {} frames.\n", app_data.frame_count);
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
