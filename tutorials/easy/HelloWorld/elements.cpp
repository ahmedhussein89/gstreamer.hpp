#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

// gst-launch-1.0 -v videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! autovideosink
int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_pipeline_new("video-player");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("videotestsrc", "source");
  if(nullptr == source) {
    fmt::print(stderr, "Failed to create source element.\n");
    return EXIT_FAILURE;
  }
  g_object_set(source, "pattern", 1, nullptr);

  auto* capsfilter = gst_element_factory_make("capsfilter", "caps");
  if(nullptr == capsfilter) {
    fmt::print(stderr, "Failed to create capsfilter element.\n");
    return EXIT_FAILURE;
  }
  g_object_set(capsfilter, "caps", gst_caps_from_string("video/x-raw,width=1280,height=720"), nullptr);

  auto* sink = gst_element_factory_make("autovideosink", "video-output");
  if(nullptr == sink) {
    fmt::print(stderr, "Failed to create sink element.\n");
    return EXIT_FAILURE;
  }

  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, sink, nullptr);
  if(!gst_element_link(source, capsfilter) || !gst_element_link(capsfilter, sink)) {
    fmt::print(stderr, "Failed to link elements.\n");
    return EXIT_FAILURE;
  }

  if(gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    fmt::print(stderr, "Failed to set pipeline to playing state.\n");
    return EXIT_FAILURE;
  }

  // Run the main event loop
  auto* loop = g_main_loop_new(nullptr, FALSE);
  g_main_loop_run(loop);

  // Cleanup
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  g_main_loop_unref(loop);

  return EXIT_SUCCESS;
}
