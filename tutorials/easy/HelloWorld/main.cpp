#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_parse_launch("videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! autovideosink", nullptr);
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to play the pipeline.\n");
    return EXIT_FAILURE;
  }

  g_main_loop_run(g_main_loop_new(nullptr, FALSE));

  return EXIT_SUCCESS;
}
