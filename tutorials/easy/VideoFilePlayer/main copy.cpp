#include <cstdlib>

#include "gstreamer.hpp"

namespace {
static void pad_added_handler(GstElement* src, GstPad* new_pad, GstElement* convert) {
  GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
  GstPadLinkReturn ret;
  GstCaps* new_pad_caps = NULL;
  GstStructure* new_pad_struct = NULL;
  const gchar* new_pad_type = NULL;

  g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

  /* If our converter is already linked, we have nothing to do here */
  if(gst_pad_is_linked(sink_pad)) {
    g_print("We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps(new_pad);
  new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  new_pad_type = gst_structure_get_name(new_pad_struct);
  if(!g_str_has_prefix(new_pad_type, "audio/x-raw")) {
    g_print("It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    goto exit;
  }

  /* Attempt the link */
  ret = gst_pad_link(new_pad, sink_pad);
  if(GST_PAD_LINK_FAILED(ret)) {
    g_print("Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print("Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if(new_pad_caps != NULL)
    gst_caps_unref(new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref(sink_pad);
}
}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("video-player");
  if(nullptr == pipeline) {
    return EXIT_FAILURE;
  }

  auto* file = gst_element_factory_make("filesrc", "file-source");
  if(nullptr == file) {
    return EXIT_FAILURE;
  }
  g_object_set(file, "location", "big-buck-bunny-480p-30sec.mp4", nullptr);

  auto* decode = gst_element_factory_make("decodebin", "decoder");
  if(nullptr == decode) {
    return EXIT_FAILURE;
  }

  auto* sink = gst_element_factory_make("autovideosink", "video-output");
  if(nullptr == sink) {
    return EXIT_FAILURE;
  }

  gst_bin_add_many(GST_BIN(pipeline), file, decode, sink, nullptr);
  if(gst_element_link(file, decode) != TRUE) {
    return EXIT_FAILURE;
  }

  g_signal_connect(decode, "pad-added", G_CALLBACK(pad_added_handler), nullptr);

  /* Start playing */
  const auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    return EXIT_FAILURE;
  }

  /* Listen to the bus */
  auto terminate = false;
  auto bus = gst_element_get_bus(pipeline);
  do {
    auto msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Parse message */
    if(msg != nullptr) {
      GError* err;
      gchar* debug_info;

      switch(GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);
        terminate = true;
        break;
      case GST_MESSAGE_EOS:
        g_print("End-Of-Stream reached.\n");
        terminate = true;
        break;
      case GST_MESSAGE_STATE_CHANGED:
        /* We are only interested in state-changed messages from the pipeline */
        if(GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
          GstState old_state, new_state, pending_state;
          gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
          // g_print("Pipeline state changed from %s to %s:\n", gst_state_get_name(old_state), gst_state_get_name(new_state));
        }
        break;
      default:
        /* We should not reach here */
        g_printerr("Unexpected message received.\n");
        break;
      }
      gst_message_unref(msg);
    }
  } while(!terminate);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
