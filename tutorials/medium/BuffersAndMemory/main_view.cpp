#include <atomic>
#include <cstdlib>
#include <span>
#include <string>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer.hpp"

namespace {

constexpr auto NumBuffers  = 10;
constexpr auto FrameWidth  = 320;
constexpr auto FrameHeight = 240;

std::atomic<int> g_frame_count{0};

GstPadProbeReturn on_buffer(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer /*user_data*/) {
  GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_READ);

  const int  n     = g_frame_count.fetch_add(1);
  const auto pts_s = GST_BUFFER_PTS_IS_VALID(buf)
                         ? static_cast<double>(GST_BUFFER_PTS(buf)) / GST_SECOND
                         : -1.0;
  const auto dur_s = GST_BUFFER_DURATION_IS_VALID(buf)
                         ? static_cast<double>(GST_BUFFER_DURATION(buf)) / GST_SECOND
                         : -1.0;

  fmt::print("Buffer {:2d}: size={} pts={:.3f}s dur={:.3f}s flags={:#010x} mem_blocks={}\n",
             n, map.size, pts_s, dur_s,
             static_cast<unsigned>(GST_BUFFER_FLAGS(buf)),
             gst_buffer_n_memory(buf));

  gst_buffer_unmap(buf, &map);
  return GST_PAD_PROBE_OK;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline   = gst::pipeline_new("buffers-memory");
  auto source     = gst::element_factory_make("videotestsrc", "source");
  auto convert    = gst::element_factory_make("videoconvert", "convert");
  auto capsfilter = gst::element_factory_make("capsfilter",   "cap");
  auto sink       = gst::element_factory_make("autovideosink","sink");

  if(!pipeline || !source || !convert || !capsfilter || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto caps = gst::caps_from_string(
      "video/x-raw,format=RGB,width=" + std::to_string(FrameWidth) +
      ",height=" + std::to_string(FrameHeight));
  if(!caps) { fmt::print(stderr, "Bad caps: {}\n", caps.error()); return EXIT_FAILURE; }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(capsfilter->get()), "caps", caps->get(), nullptr);

  auto raw_source     = gst::bin_add(*pipeline, *source);
  auto raw_convert    = gst::bin_add(*pipeline, *convert);
  auto raw_capsfilter = gst::bin_add(*pipeline, *capsfilter);
  auto raw_sink       = gst::bin_add(*pipeline, *sink);

  if(!raw_source || !raw_convert || !raw_capsfilter || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n"); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_capsfilter); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_capsfilter, *raw_sink); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  auto probe_pad = gst::element_get_static_pad(*raw_capsfilter, "src");
  if(probe_pad) {
    gst_pad_add_probe(probe_pad->get(), GST_PAD_PROBE_TYPE_BUFFER,
        reinterpret_cast<GstPadProbeCallback>(on_buffer), nullptr, nullptr);
  }

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    return EXIT_FAILURE;
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) { fmt::print(stderr, "Failed to get bus.\n"); return EXIT_FAILURE; }

  auto msg = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg) {
    if(gst::MessageType::Error == gst::message_type(*msg)) {
      auto parsed = gst::message_parse_error(msg->get());
      if(parsed) { fmt::print(stderr, "Error: {}\n", parsed->first); }
    } else if(gst::MessageType::EOS == gst::message_type(*msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
