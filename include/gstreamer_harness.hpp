#pragma once
// gst::harness_* — thin wrapper over GstHarness (gst/check/gstharness.h).
// Test-only: GstHarness lives in gstreamer-check-1.0, a separate library from
// core GStreamer. This header is intentionally NOT part of the gstreamer_hpp
// INTERFACE target's default sources (see include/CMakeLists.txt) so pulling
// it in never forces a GStreamer::Check link onto ordinary consumers of
// gstreamer.hpp. Callers that need it (test binaries, TestingElements-style
// tutorials) #include it directly and link GStreamer::Check themselves,
// exactly as tutorials/hard/TestingElements/CMakeLists.txt already does.
//
// __has_include guards the whole file so a host without gstreamer-check
// installed still compiles (the header simply contributes nothing).
#if __has_include(<gst/check/gstharness.h>)

#include <memory>
#include <string_view>

#include <fmt/format.h>

#include <gst/check/gstharness.h>

#include <gstreamer.hpp>

#include <nonstd/expected.hpp>

namespace gst {

struct GstHarnessDeleter final {
  void operator()(GstHarness* harness) const noexcept {
    if(harness != nullptr) {
      gst_harness_teardown(harness);
    }
  }
};
using HarnessPtr = std::unique_ptr<GstHarness, GstHarnessDeleter>;

inline nonstd::expected<HarnessPtr, std::string> harness_new(std::string_view element_name) {
  std::string name_str(element_name);
  GstHarness* harness = gst_harness_new(name_str.c_str());
  if(harness == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create harness for '{}'", element_name));
  }
  return HarnessPtr{harness};
}

// harness_teardown is redundant when HarnessPtr is used (its deleter already
// tears down), but exposed for parity with the raw C API / explicit early
// teardown before the unique_ptr goes out of scope.
inline void harness_teardown(HarnessPtr& harness) noexcept {
  harness.reset();
}

inline void harness_set_src_caps_str(const HarnessPtr& harness, std::string_view caps_description) {
  std::string caps_str(caps_description);
  gst_harness_set_src_caps_str(harness.get(), caps_str.c_str());
}

inline nonstd::expected<BufferPtr, std::string> harness_create_buffer(const HarnessPtr& harness, gsize size) {
  GstBuffer* buf = gst_harness_create_buffer(harness.get(), size);
  if(buf == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create harness buffer"));
  }
  return BufferPtr{buf};
}

// Consumes the buffer (transfer-full into the harness), matching gst_harness_push semantics.
inline nonstd::expected<void, std::string> harness_push(const HarnessPtr& harness, BufferPtr buffer) {
  const GstFlowReturn ret = gst_harness_push(harness.get(), buffer.release());
  if(ret != GST_FLOW_OK) {
    return nonstd::make_unexpected(fmt::format("gst_harness_push failed with GstFlowReturn {}", static_cast<int>(ret)));
  }
  return {};
}

inline nonstd::expected<BufferPtr, std::string> harness_pull(const HarnessPtr& harness) {
  GstBuffer* buf = gst_harness_pull(harness.get());
  if(buf == nullptr) {
    return nonstd::make_unexpected(std::string("No buffer available to pull from harness"));
  }
  return BufferPtr{buf};
}

}    // namespace gst

#endif    // __has_include(<gst/check/gstharness.h>)
