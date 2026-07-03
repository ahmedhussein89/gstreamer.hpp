#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr auto NumBuffers = 300;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::raii::pipeline_new("events-queries");
  auto source   = gst::raii::element_factory_make("videotestsrc", "source");
  auto convert  = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink     = gst::raii::element_factory_make("autovideosink","sink");

  if(!pipeline || !source || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);

  auto raw_source  = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink    = gst::raii::bin_add(*pipeline, std::move(*sink));
  if(!raw_source || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n"); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_convert); !l) {
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

  gint64 pos = 0;
  if(gst_element_query_position(pipeline->get(), GST_FORMAT_TIME, &pos)) {
    fmt::print(stdout, "Position: {:.3f}s\n", static_cast<double>(pos) / GST_SECOND);
  } else {
    fmt::print(stdout, "Position: unknown\n");
  }

  gint64 dur = -1;
  if(gst_element_query_duration(pipeline->get(), GST_FORMAT_TIME, &dur)) {
    fmt::print(stdout, "Duration: {:.3f}s\n", static_cast<double>(dur) / GST_SECOND);
  } else {
    fmt::print(stdout, "Duration: unknown (videotestsrc has no fixed duration)\n");
  }

  GstQuery* lq = gst_query_new_latency();
  if(gst_element_query(pipeline->get(), lq)) {
    gboolean     live    = FALSE;
    GstClockTime min_lat = 0;
    GstClockTime max_lat = 0;
    gst_query_parse_latency(lq, &live, &min_lat, &max_lat);
    fmt::print(stdout, "Latency: live={} min={:.3f}ms{}\n",
               live ? "yes" : "no",
               static_cast<double>(min_lat) / GST_MSECOND,
               max_lat == GST_CLOCK_TIME_NONE ? "" :
                   fmt::format(" max={:.3f}ms", static_cast<double>(max_lat) / GST_MSECOND));
  }
  gst_query_unref(lq);

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
