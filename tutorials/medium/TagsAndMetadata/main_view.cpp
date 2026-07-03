#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include <gst/gst.h>

#include "gstreamer.hpp"

namespace {

void print_tag(const GstTagList* list, const gchar* tag, gpointer /*user_data*/) {
  const GValue* gval = gst_tag_list_get_value_index(list, tag, 0);
  if(nullptr == gval) { return; }
  gchar* str = g_strdup_value_contents(gval);
  fmt::print(stdout, "  {} = {}\n", tag, str);
  g_free(str);
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("tags-metadata");
  auto source   = gst::element_factory_make("videotestsrc", "source");
  auto sink     = gst::element_factory_make("autovideosink","sink");

  if(!pipeline || !source || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  auto raw_source = gst::bin_add(*pipeline, *source);
  auto raw_sink   = gst::bin_add(*pipeline, *sink);
  if(!raw_source || !raw_sink) {
    fmt::print(stderr, "Failed to add elements.\n"); return EXIT_FAILURE;
  }
  if(auto l = gst::element_link(*raw_source, *raw_sink); !l) {
    fmt::print(stderr, "{}\n", l.error()); return EXIT_FAILURE;
  }

  if(auto s = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !s) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", s.error());
    return EXIT_FAILURE;
  }

  GstTagList* tags = gst_tag_list_new(
      GST_TAG_TITLE,   "Tutorial Video",
      GST_TAG_ARTIST,  "GStreamer Tutorial",
      GST_TAG_COMMENT, "deepstream.hpp tags tutorial",
      nullptr);
  gst_element_post_message(pipeline->get(), gst_message_new_tag(GST_OBJECT(pipeline->get()), tags));

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) { fmt::print(stderr, "Failed to get bus.\n"); return EXIT_FAILURE; }

  bool running = true;
  while(running) {
    auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE,
        gst::MessageType::Tag | gst::MessageType::EOS | gst::MessageType::Error);
    if(!msg_result) { break; }

    const auto& msg = msg_result.value();
    switch(static_cast<GstMessageType>(GST_MESSAGE_TYPE(msg.get()))) {
      case GST_MESSAGE_TAG: {
        GstTagList* recv_tags = nullptr;
        gst_message_parse_tag(msg.get(), &recv_tags);
        fmt::print(stdout, "TAG message received:\n");
        gst_tag_list_foreach(recv_tags, print_tag, nullptr);
        gst_tag_list_unref(recv_tags);
        gst_element_send_event(pipeline->get(), gst_event_new_eos());
        break;
      }
      case GST_MESSAGE_EOS:
        fmt::print(stdout, "End of stream reached.\n");
        running = false;
        break;
      case GST_MESSAGE_ERROR: {
        auto parsed = gst::message_parse_error(msg.get());
        if(parsed) { fmt::print(stderr, "Error: {}\n", parsed->first); }
        running = false;
        break;
      }
      default:
        break;
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  return EXIT_SUCCESS;
}
