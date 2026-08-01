// ${CMAKE_SOURCE_DIR}/tutorials/hard/TestingElements/main_view.cpp
//
// Uses gst::harness_* (include/gstreamer_harness.hpp) plus the buffer wrappers
// from gstreamer.hpp instead of the raw GstHarness/GstBuffer C API used by main.cpp.
#include <cstdlib>
#include <cstring>
#include <span>

#include <fmt/core.h>

#include <gst/check/gstharness.h>

#include "gstreamer.hpp"
#include "gstreamer_harness.hpp"
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

  auto harness = gst::harness_new(MY_EDGE_DETECTOR_NAME);
  if(!check(harness.has_value(), "gst::harness_new failed")) {
    return EXIT_FAILURE;
  }

  gst::harness_set_src_caps_str(*harness, "video/x-raw,format=GRAY8,width=4,height=2,framerate=0/1");

  auto buf = gst::harness_create_buffer(*harness, sizeof(kInput));
  if(!check(buf.has_value(), "gst::harness_create_buffer failed")) {
    return EXIT_FAILURE;
  }
  const auto* input_bytes = reinterpret_cast<const std::byte*>(kInput);    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  if(!check(gst::buffer_fill(gst::Buffer{buf->get()}, 0, std::span(input_bytes, sizeof(kInput))).has_value(),
            "gst::buffer_fill failed")) {
    return EXIT_FAILURE;
  }

  auto pushed = gst::harness_push(*harness, std::move(*buf));
  if(!check(pushed.has_value(), "gst::harness_push did not return GST_FLOW_OK")) {
    return EXIT_FAILURE;
  }

  auto out = gst::harness_pull(*harness);
  if(!check(out.has_value(), "gst::harness_pull failed")) {
    return EXIT_FAILURE;
  }

  auto map = gst::buffer_map(gst::Buffer{out->get()}, gst::MapFlags::Read);
  bool passed = check(map.has_value(), "gst::buffer_map on the output buffer failed");

  if(passed) {
    passed = check(sizeof(kExpected) == map->info().size, "output buffer size differs from the input frame size") &&
             check(0 == memcmp(map->data().data(), kExpected, sizeof(kExpected)), "output bytes differ from the expected edge map");
  }

  if(!passed) {
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "All assertions passed for '{}'.\n", MY_EDGE_DETECTOR_NAME);
  return EXIT_SUCCESS;
}
