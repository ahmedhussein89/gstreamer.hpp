#include <cstdlib>
#include <span>
#include <string>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr auto NumBuffers  = 90;
constexpr auto FrameWidth  = 320;
constexpr auto FrameHeight = 240;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline   = gst::raii::pipeline_new("caps-filters");
  auto source     = gst::raii::element_factory_make("videotestsrc", "source");
  auto capsfilter = gst::raii::element_factory_make("capsfilter",   "cap");
  auto convert    = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink       = gst::raii::element_factory_make("autovideosink","sink");

  if(!pipeline || !source || !capsfilter || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto caps = gst::caps_from_string(
      "video/x-raw,format=RGB,width=" + std::to_string(FrameWidth) +
      ",height=" + std::to_string(FrameHeight));
  if(!caps) {
    fmt::print(stderr, "Failed to parse caps: {}\n", caps.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(capsfilter->get()), "caps", caps->get(), nullptr);

  auto raw_source     = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_capsfilter = gst::raii::bin_add(*pipeline, std::move(*capsfilter));
  auto raw_convert    = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink       = gst::raii::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_capsfilter || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n");
    return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_capsfilter); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_capsfilter, *raw_convert); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_sink); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    return EXIT_FAILURE;
  }
  gst_element_get_state(pipeline->get(), nullptr, nullptr, GST_CLOCK_TIME_NONE);

  auto src_pad = gst::raii::element_get_static_pad(*raw_capsfilter, "src");
  if(src_pad) {
    auto neg_caps = gst::pad_get_current_caps(src_pad->get());
    if(neg_caps) {
      gchar* str = gst_caps_to_string(neg_caps->get());
      fmt::print(stdout, "Negotiated caps: {}\n", str);
      g_free(str);
    }
  }

  auto bus = gst::raii::element_get_bus(*pipeline);
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
