// ${CMAKE_SOURCE_DIR}/tutorials/hard/TestingElements/main_view.cpp
//
// GstHarness (gst/check/gstharness.h) has no gst:: wrapper — the enhanced layer targets
// pipelines/elements/bus, not the check library. This track therefore only uses gst::init()
// for setup and otherwise falls back to the same raw C harness calls as main.cpp; it exists to
// show that gst::-based code and plain-C GstHarness code interoperate freely, not to add coverage.
#include <cstdlib>
#include <cstring>
#include <span>

#include <fmt/core.h>

#include <gst/check/gstharness.h>

#include "gstreamer.hpp"
#include "myedgedetector.h"

namespace {
// Same input/expected bytes as main.cpp — see that file for the derivation.
constexpr int    Width  = 4;
constexpr int    Height = 2;
constexpr guint8 kInput[Width * Height] = {
    10, 50, 52, 200,
    0,  0,  100, 100,
};
constexpr guint8 kExpected[Width * Height] = {
    0, 255, 0, 255,
    0, 0,   255, 0,
};

// Deliberately NOT assert(): this binary is registered with CTest, and assert() compiles
// out under NDEBUG (any -DCMAKE_BUILD_TYPE=Release build), which would make the test pass
// unconditionally. check() is always evaluated.
bool check(bool condition, const char* what) {
  if(!condition) {
    fmt::print(stderr, "FAIL: {}\n", what);
  }
  return condition;
}
}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(TRUE != my_edge_detector_register()) {
    fmt::print(stderr, "Failed to register '{}' element.\n", MY_EDGE_DETECTOR_NAME);
    return EXIT_FAILURE;
  }

  GstHarness* harness = gst_harness_new(MY_EDGE_DETECTOR_NAME);
  if(!check(nullptr != harness, "gst_harness_new returned NULL")) {
    return EXIT_FAILURE;
  }

  gst_harness_set_src_caps_str(harness, "video/x-raw,format=GRAY8,width=4,height=2,framerate=0/1");

  GstBuffer* buf = gst_harness_create_buffer(harness, sizeof(kInput));
  if(!check(nullptr != buf, "gst_harness_create_buffer returned NULL")) {
    gst_harness_teardown(harness);
    return EXIT_FAILURE;
  }
  gst_buffer_fill(buf, 0, kInput, sizeof(kInput));

  const GstFlowReturn ret = gst_harness_push(harness, buf);
  if(!check(GST_FLOW_OK == ret, "gst_harness_push did not return GST_FLOW_OK")) {
    gst_harness_teardown(harness);
    return EXIT_FAILURE;
  }

  GstBuffer* out = gst_harness_pull(harness);
  if(!check(nullptr != out, "gst_harness_pull returned NULL")) {
    gst_harness_teardown(harness);
    return EXIT_FAILURE;
  }

  GstMapInfo map;
  // The map call must live outside the check, so it still happens in every build.
  const gboolean mapped = gst_buffer_map(out, &map, GST_MAP_READ);
  bool           passed = check(TRUE == mapped, "gst_buffer_map on the output buffer failed");

  if(passed) {
    passed = check(sizeof(kExpected) == map.size, "output buffer size differs from the input frame size") &&
             check(0 == memcmp(map.data, kExpected, sizeof(kExpected)), "output bytes differ from the expected edge map");
    gst_buffer_unmap(out, &map);
  }

  gst_buffer_unref(out);
  gst_harness_teardown(harness);

  if(!passed) {
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "All assertions passed for '{}'.\n", MY_EDGE_DETECTOR_NAME);
  return EXIT_SUCCESS;
}
