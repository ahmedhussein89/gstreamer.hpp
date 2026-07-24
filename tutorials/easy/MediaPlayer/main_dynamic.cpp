#include <cstdlib>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <gst/gst.h>

namespace {

// Create each factory in `factories` (null-terminated), add it to the pipeline,
// sync it to the running state, and link the chain head-to-tail; finally link
// `src_pad` into the chain's head. Returns false on any failure.
bool build_branch(GstElement* pipeline, GstPad* src_pad, const char* const* factories) {
  GstElement* head = nullptr;
  GstElement* prev = nullptr;
  for(const char* const* factory = factories; nullptr != *factory; ++factory) {
    GstElement* element = gst_element_factory_make(*factory, nullptr);
    if(nullptr == element) {
      fmt::print(stderr, "Failed to create {}.\n", *factory);
      return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    gst_bin_add(GST_BIN(pipeline), element);
    gst_element_sync_state_with_parent(element);
    if(nullptr == head) {
      head = element;
    }
    if(nullptr != prev && TRUE != gst_element_link(prev, element)) {
      fmt::print(stderr, "Failed to link {} branch.\n", factories[0]);
      return false;
    }
    prev = element;
  }

  GstPad* sink_pad = gst_element_get_static_pad(head, "sink");
  const GstPadLinkReturn link = gst_pad_link(src_pad, sink_pad);
  gst_object_unref(sink_pad);
  return GST_PAD_LINK_OK == link;
}

// decodebin exposes its decoded output pads only after examining the stream, so
// we build the matching sink branch on demand here. Building lazily (rather than
// pre-adding both branches) means a file with only audio or only video never
// leaves an unfed sink stuck in preroll, which would stop the pipeline from ever
// reaching EOS.
void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* pipeline = static_cast<GstElement*>(user_data);

  GstCaps* caps = gst_pad_get_current_caps(new_pad);
  if(nullptr == caps) {
    return;
  }
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  const char* name = gst_structure_get_name(structure);

  if(TRUE == g_str_has_prefix(name, "video/x-raw")) {
    const char* const branch[] = {"videoconvert", "autovideosink", nullptr};
    build_branch(pipeline, new_pad, branch);
  } else if(TRUE == g_str_has_prefix(name, "audio/x-raw")) {
    const char* const branch[] = {"audioconvert", "audioresample", "autoaudiosink", nullptr};
    build_branch(pipeline, new_pad, branch);
  }

  gst_caps_unref(caps);
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <media-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("media-player");
  auto* source   = gst_element_factory_make("filesrc", "source");
  auto* decode   = gst_element_factory_make("decodebin", "decoder");

  if(!pipeline || !source || !decode) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "location", argv[1], nullptr);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  gst_bin_add_many(GST_BIN(pipeline), source, decode, nullptr);

  // filesrc and decodebin both have static pads, so they link now; decodebin
  // links to the sink branches later from the pad-added callback.
  if(TRUE != gst_element_link(source, decode)) {
    fmt::print(stderr, "Failed to link source to decoder.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  g_signal_connect(decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), pipeline);

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* error = nullptr;
      gst_message_parse_error(msg, &error, nullptr);
      fmt::print(stderr, "Error: {}\n", error->message);
      g_error_free(error);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
