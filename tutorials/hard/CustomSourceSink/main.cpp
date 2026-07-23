#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

#include "mysink.h"
#include "mysrc.h"

namespace {

constexpr auto NumBuffers = 50;

gboolean on_bus_msg(GstBus* /*bus*/, GstMessage* msg, gpointer user_data) {
  auto* loop = static_cast<GMainLoop*>(user_data);
  switch(GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_EOS:
      fmt::print(stdout, "End of stream reached.\n");
      g_main_loop_quit(loop);
      break;
    default:
      break;
  }
  return TRUE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(TRUE != my_src_register() || TRUE != my_sink_register()) {
    fmt::print(stderr, "Failed to register custom elements.\n");
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("custom-source-sink");
  auto* source   = gst_element_factory_make("mysrc", "source");
  auto* sink     = gst_element_factory_make("mysink", "sink");

  if(nullptr == pipeline || nullptr == source || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, sink, nullptr);
  if(TRUE != gst_element_link(source, sink)) {
    fmt::print(stderr, "Failed to link source to sink.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Pipeline running ({} buffers)...\n", NumBuffers);

  auto* loop = g_main_loop_new(nullptr, FALSE);
  auto* bus  = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, on_bus_msg, loop);
  gst_object_unref(bus);

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
