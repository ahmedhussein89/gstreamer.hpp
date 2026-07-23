#include <array>
#include <cstdlib>
#include <string>

#include <fmt/core.h>
#include <fmt/format.h>

#include "gstreamer.hpp"

namespace {
constexpr auto NumBuffers = 150;
constexpr auto CellWidth  = 320;
constexpr auto CellHeight = 240;
constexpr auto NumCameras = 3;

struct Cell {
  gint pattern;
  gint xpos;
  gint ypos;
};

// 3-camera grid: two on the top row, one on the bottom-left. The bottom-right
// quadrant is simply left uncovered by the background color.
constexpr std::array<Cell, NumCameras> Cells{{
    {/*pattern=*/0, /*xpos=*/0, /*ypos=*/0},                        // smpte bars, top-left
    {/*pattern=*/1, /*xpos=*/CellWidth, /*ypos=*/0},                // snow, top-right
    {/*pattern=*/2, /*xpos=*/0, /*ypos=*/CellHeight},               // black, bottom-left
}};

// Request pads must be handed back to the element, not merely unreffed. Every early
// return below has to undo the pads acquired on previous loop iterations.
void release_request_pads(GstElement* compositor, const std::array<GstPad*, NumCameras>& pads, int count) {
  for(int i = 0; i < count; ++i) {
    GstPad* pad = pads[static_cast<size_t>(i)];
    if(nullptr != pad) {
      gst_element_release_request_pad(compositor, pad);
      gst_object_unref(pad);
    }
  }
}
}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("multi-camera-viewer");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto compositor = gst::element_factory_make("compositor", "compositor");
  auto convert    = gst::element_factory_make("videoconvert", "convert");
  auto sink       = gst::element_factory_make("fakesink", "sink");

  if(!compositor || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-bad/base provide 'compositor'.\n");
    return EXIT_FAILURE;
  }

  auto raw_compositor = gst::bin_add(*pipeline, std::move(*compositor));
  auto raw_convert     = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_sink        = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_compositor || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_compositor, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link compositor to convert: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link convert to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  std::array<GstPad*, NumCameras> sink_pads{};

  for(int i = 0; i < NumCameras; ++i) {
    const std::string src_name  = fmt::format("source{}", i);
    const std::string conv_name = fmt::format("convert{}", i);

    auto source  = gst::element_factory_make("videotestsrc", src_name);
    auto convert_i = gst::element_factory_make("videoconvert", conv_name);

    if(!source || !convert_i) {
      fmt::print(stderr, "Failed to create source/convert for camera {}.\n", i);
      release_request_pads(*raw_compositor, sink_pads, i);
      return EXIT_FAILURE;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(source->get()), "pattern", Cells[static_cast<size_t>(i)].pattern, "num-buffers", NumBuffers, nullptr);

    auto raw_source     = gst::bin_add(*pipeline, std::move(*source));
    auto raw_convert_i  = gst::bin_add(*pipeline, std::move(*convert_i));

    if(!raw_source || !raw_convert_i) {
      fmt::print(stderr, "Failed to add source/convert for camera {}.\n", i);
      release_request_pads(*raw_compositor, sink_pads, i);
      return EXIT_FAILURE;
    }

    if(auto link = gst::element_link(*raw_source, *raw_convert_i); !link) {
      fmt::print(stderr, "Failed to link source to convert for camera {}: {}\n", i, link.error());
      release_request_pads(*raw_compositor, sink_pads, i);
      return EXIT_FAILURE;
    }

    // compositor sink pads are request pads with per-pad xpos/ypos/width/height
    // properties — no gst:: wrapper for request pads, so use raw GStreamer API.
    sink_pads[static_cast<size_t>(i)] = gst_element_request_pad_simple(*raw_compositor, "sink_%u");
    if(sink_pads[static_cast<size_t>(i)] == nullptr) {
      fmt::print(stderr, "Failed to request compositor sink pad for camera {}.\n", i);
      release_request_pads(*raw_compositor, sink_pads, i);
      return EXIT_FAILURE;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    g_object_set(G_OBJECT(sink_pads[static_cast<size_t>(i)]), "xpos", Cells[static_cast<size_t>(i)].xpos,
                 "ypos", Cells[static_cast<size_t>(i)].ypos, "width", CellWidth, "height", CellHeight, nullptr);

    GstPad* src_pad = gst_element_get_static_pad(*raw_convert_i, "src");
    if(GST_PAD_LINK_OK != gst_pad_link(src_pad, sink_pads[static_cast<size_t>(i)])) {
      fmt::print(stderr, "Failed to link convert{} to compositor.\n", i);
      release_request_pads(*raw_compositor, sink_pads, i + 1);
      gst_object_unref(src_pad);
      return EXIT_FAILURE;
    }
    gst_object_unref(src_pad);
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    release_request_pads(*raw_compositor, sink_pads, NumCameras);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Compositing {} cameras into one output ({} frames each).\n", NumCameras, NumBuffers);

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
      fmt::print(stdout, "Composite stream finished.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  release_request_pads(*raw_compositor, sink_pads, NumCameras);

  return EXIT_SUCCESS;
}
