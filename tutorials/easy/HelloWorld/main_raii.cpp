#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include "gstreamer_raii.hpp"

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::raii::parse_launch("videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! autovideosink");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to play the pipeline.\n");
    return EXIT_FAILURE;
  }

  g_main_loop_run(g_main_loop_new(nullptr, FALSE));

  // *pipeline destructor calls gst_object_unref
  return EXIT_SUCCESS;
}
