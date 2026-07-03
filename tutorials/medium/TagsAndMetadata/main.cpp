#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

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
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("tags-metadata");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* sink     = gst_element_factory_make("autovideosink","sink");

  if(nullptr == pipeline || nullptr == source || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
  if(TRUE != gst_element_link(source, sink)) {
    fmt::print(stderr, "Failed to link elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Post a TAG message on the bus so we can demonstrate parsing it.
  GstTagList* tags = gst_tag_list_new(
      GST_TAG_TITLE,   "Tutorial Video",
      GST_TAG_ARTIST,  "GStreamer Tutorial",
      GST_TAG_COMMENT, "deepstream.hpp tags tutorial",
      nullptr);
  gst_element_post_message(pipeline, gst_message_new_tag(GST_OBJECT(pipeline), tags));

  auto* bus = gst_element_get_bus(pipeline);
  bool  running = true;

  while(running) {
    GstMessage* msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_TAG | GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if(nullptr == msg) { break; }

    switch(GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_TAG: {
        GstTagList* recv_tags = nullptr;
        gst_message_parse_tag(msg, &recv_tags);
        fmt::print(stdout, "TAG message received:\n");
        gst_tag_list_foreach(recv_tags, print_tag, nullptr);
        gst_tag_list_unref(recv_tags);
        // Done demonstrating tags — send EOS to stop the pipeline.
        gst_element_send_event(pipeline, gst_event_new_eos());
        break;
      }
      case GST_MESSAGE_EOS:
        fmt::print(stdout, "End of stream reached.\n");
        running = false;
        break;
      case GST_MESSAGE_ERROR: {
        GError* err = nullptr;
        gst_message_parse_error(msg, &err, nullptr);
        fmt::print(stderr, "Error: {}\n", err->message);
        g_error_free(err);
        running = false;
        break;
      }
      default:
        break;
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
