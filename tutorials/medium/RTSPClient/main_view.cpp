#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* convert = static_cast<GstElement*>(user_data);
  auto sink_pad = gst::element_get_static_pad(convert, "sink");
  if(!sink_pad) {
    return;
  }
  if(gst::pad_is_linked(*sink_pad)) {
    return;
  }

  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    return;
  }
  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    return;
  }

  if(!gst::structure_get_name(*structure).starts_with("video/x-raw")) {
    return;
  }

  if(auto link = gst::pad_link(new_pad, *sink_pad); !link) {
    fmt::print(stderr, "Failed to link decoded pad: {}\n", link.error());
  }
}

struct PipelineData {
  GstElement* decodebin;
};

void on_rtspsrc_pad_added(GstElement* /*rtspsrc*/, GstPad* new_pad, gpointer user_data) {
  auto* data = static_cast<PipelineData*>(user_data);
  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    return;
  }
  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    return;
  }

  const auto* media = gst_structure_get_string(*structure, "media");
  if(g_strcmp0(media, "video") != 0) {
    return;
  }

  auto sink_pad = gst::element_get_static_pad(data->decodebin, "sink");
  if(!sink_pad || gst::pad_is_linked(*sink_pad)) {
    return;
  }

  if(auto link = gst::pad_link(new_pad, *sink_pad); !link) {
    fmt::print(stderr, "Failed to link rtspsrc pad: {}\n", link.error());
  }
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <rtsp-url>\n", argv[0]);
    fmt::print(stderr, "Example: {} rtsp://example.com/stream\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto pipeline = gst::pipeline_new("rtsp-client");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source  = gst::element_factory_make("rtspsrc", "source");
  auto decode  = gst::element_factory_make("decodebin", "decoder");
  auto convert = gst::element_factory_make("videoconvert", "convert");
  auto sink    = gst::element_factory_make("autovideosink", "display");

  if(!source || !decode || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", argv[1], "latency", 200, nullptr);

  auto raw_source  = gst::bin_add(*pipeline, std::move(*source));
  auto raw_decode  = gst::bin_add(*pipeline, std::move(*decode));
  auto raw_convert = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_sink    = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_decode || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_convert, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link convert to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  PipelineData data{*raw_decode};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
  g_signal_connect(*raw_source, "pad-added", G_CALLBACK(on_rtspsrc_pad_added), &data);
  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), *raw_convert);
#pragma clang diagnostic pop

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Connecting to {}. Press Ctrl+C to stop.\n", argv[1]);

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
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
