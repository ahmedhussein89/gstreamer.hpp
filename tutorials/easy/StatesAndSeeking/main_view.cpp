#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer.hpp"

namespace {
constexpr auto NumBuffers = 600;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("states-seeking");
  auto source   = gst::element_factory_make("videotestsrc", "source");
  auto convert  = gst::element_factory_make("videoconvert", "convert");
  auto sink     = gst::element_factory_make("autovideosink", "sink");

  if(!pipeline || !source || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto raw_source  = gst::bin_add(*pipeline, *source);
  auto raw_convert = gst::bin_add(*pipeline, *convert);
  auto raw_sink    = gst::bin_add(*pipeline, *sink);

  if(!raw_source || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
    fmt::print(stderr, "Failed to link source→convert: {}\n", l.error());
    return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_convert, *raw_sink); !l) {
    fmt::print(stderr, "Failed to link convert→sink: {}\n", l.error());
    return EXIT_FAILURE;
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_PAUSED);
  gst_element_get_state(pipeline->get(), nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: READY → PAUSED\n");

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    return EXIT_FAILURE;
  }
  gst_element_get_state(pipeline->get(), nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  gint64 pos = 0;
  gst_element_query_position(pipeline->get(), GST_FORMAT_TIME, &pos);
  fmt::print(stdout, "Position: {:.3f}s\n", static_cast<double>(pos) / GST_SECOND);

  gint64 dur = -1;
  if(gst_element_query_duration(pipeline->get(), GST_FORMAT_TIME, &dur)) {
    fmt::print(stdout, "Duration: {:.3f}s\n", static_cast<double>(dur) / GST_SECOND);
  } else {
    fmt::print(stdout, "Duration: unknown (videotestsrc has no fixed duration)\n");
  }

  const gint64 seek_pos = 2 * GST_SECOND;
  if(gst_element_seek_simple(pipeline->get(), GST_FORMAT_TIME,
         static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), seek_pos)) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(seek_pos) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

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
