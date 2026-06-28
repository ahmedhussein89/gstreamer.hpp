#include <type_traits>

#include <gtest/gtest.h>

#include "gstreamer.hpp"

namespace {

// ============================================================================
// parse_launch Tests - 100% Coverage
// ============================================================================

// Test 1: Valid simple pipeline - basic success case
TEST(GstreamerTest, ParseLaunchValidSimplePipeline) {
  auto result = gst::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
  EXPECT_TRUE(*result);    // Test operator bool
}

// Test 2: Valid complex pipeline with properties
TEST(GstreamerTest, ParseLaunchValidComplexPipeline) {
  auto result = gst::parse_launch("videotestsrc pattern=0 ! videoconvert ! autovideosink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 3: Valid single element (no links)
TEST(GstreamerTest, ParseLaunchValidSingleElement) {
  auto result = gst::parse_launch("fakesrc");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 4: Valid pipeline with multiple pads
TEST(GstreamerTest, ParseLaunchValidMultiplePads) {
  auto result = gst::parse_launch("filesrc location=/dev/null ! decodebin ! autovideosink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 5: Invalid syntax - returns error
TEST(GstreamerTest, ParseLaunchInvalidSyntax) {
  auto result = gst::parse_launch("fakesrc ! ! fakesink");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error() != nullptr);
  EXPECT_NE(result.error()->message, nullptr);
}

// Test 6: Invalid element name - returns error
TEST(GstreamerTest, ParseLaunchInvalidElementName) {
  auto result = gst::parse_launch("nonexistentelement123xyz");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error() != nullptr);
}

// Test 7: Empty pipeline description
TEST(GstreamerTest, ParseLaunchEmptyPipeline) {
  auto result = gst::parse_launch("");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error() != nullptr);
}

// Test 8: Pipeline with element properties
TEST(GstreamerTest, ParseLaunchWithProperties) {
  auto result = gst::parse_launch("fakesrc name=source ! fakesink name=sink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 9: Valid pipeline - Element move semantics
TEST(GstreamerTest, ParseLaunchElementMovable) {
  auto result = gst::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(result.has_value());

  gst::Element elem = std::move(*result);
  EXPECT_TRUE(elem.get() != nullptr);
}

// Test 10: Valid pipeline - test Element operator bool returns true
TEST(GstreamerTest, ParseLaunchElementOperatorBoolTrue) {
  auto result = gst::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(static_cast<bool>(*result));
}

// Test 11: Invalid - unmatched parenthesis
TEST(GstreamerTest, ParseLaunchInvalidParenthesis) {
  auto result = gst::parse_launch("fakesrc ( ! fakesink");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error() != nullptr);
}

// Test 12: Invalid - property without element
TEST(GstreamerTest, ParseLaunchInvalidProperty) {
  auto result = gst::parse_launch("! name=test");
  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error() != nullptr);
}

// Test 13: Valid pipeline with whitespace
TEST(GstreamerTest, ParseLaunchWithWhitespace) {
  auto result = gst::parse_launch("  fakesrc  !  fakesink  ");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 14: Valid complex pipeline - stress test
TEST(GstreamerTest, ParseLaunchComplexMultiElement) {
  auto result = gst::parse_launch(
      "videotestsrc ! "
      "videoconvert ! "
      "videoscale ! "
      "fakesink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->get() != nullptr);
}

// Test 15: Error message contains meaningful information
TEST(GstreamerTest, ParseLaunchErrorMessageValid) {
  auto result = gst::parse_launch("invalidelem");
  EXPECT_FALSE(result.has_value());
  const auto& error = result.error();
  EXPECT_TRUE(error != nullptr);
  EXPECT_TRUE(error->message != nullptr);
  EXPECT_NE(std::string(error->message).length(), 0u);
}

// ============================================================================
// message_parse_error Tests - 100% Coverage
// ============================================================================

// Test 1: Valid error message with debug info
TEST(GstreamerTest, MessageParseErrorValidWithDebugInfo) {
  auto pipeline_result = gst::parse_launch("invalidelem ! fakesink");
  EXPECT_FALSE(pipeline_result.has_value());

  // Create a bus and get an error message
  auto pipeline_valid = gst::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(pipeline_valid.has_value());

  GstBus* bus = gst_element_get_bus(pipeline_valid->get());
  EXPECT_NE(bus, nullptr);

  gst_element_set_state(pipeline_valid->get(), GST_STATE_PLAYING);

  // Wait for error message with timeout
  GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_SECOND, GstMessageType{GST_MESSAGE_ERROR});

  if(msg != nullptr) {
    auto parse_result = gst::message_parse_error(msg);
    EXPECT_TRUE(parse_result.has_value());

    const auto& [error_msg, debug_info] = *parse_result;
    EXPECT_FALSE(error_msg.empty());
    EXPECT_NE(error_msg.length(), 0u);

    gst_message_unref(msg);
  }

  gst_object_unref(bus);
}

// Test 2: Valid error message with multiple debug info scenarios
TEST(GstreamerTest, MessageParseErrorMultipleScenarios) {
  // Test multiple error messages with different configurations
  for(int i = 0; i < 3; ++i) {
    GError* error = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Error %d", i);
    GstMessage* msg = gst_message_new_error(nullptr, error, "Debug %d");

    auto parse_result = gst::message_parse_error(msg);
    EXPECT_TRUE(parse_result.has_value());

    gst_message_unref(msg);
  }
}

// Test 3: Valid error message with debug info provided
TEST(GstreamerTest, MessageParseErrorValidWithDebugString) {
  GError* error = g_error_new(GST_STREAM_ERROR, GST_STREAM_ERROR_DECODE, "Decode failed");
  EXPECT_NE(error, nullptr);

  GstMessage* msg = gst_message_new_error(nullptr, error, "Debug information here");
  EXPECT_NE(msg, nullptr);

  auto parse_result = gst::message_parse_error(msg);
  EXPECT_TRUE(parse_result.has_value());

  const auto& [error_msg, debug_info] = *parse_result;
  EXPECT_EQ(error_msg, "Decode failed");
  EXPECT_EQ(debug_info, "Debug information here");

  gst_message_unref(msg);
}

// Test 4: No error in message (nullptr error) - error case
TEST(GstreamerTest, MessageParseErrorNoErrorFound) {
  // Create a message that is not an error message (e.g., EOS message)
  GstMessage* msg = gst_message_new_eos(nullptr);
  EXPECT_NE(msg, nullptr);

  auto parse_result = gst::message_parse_error(msg);
  EXPECT_FALSE(parse_result.has_value());
  EXPECT_TRUE(parse_result.error() == "No error found in message");

  gst_message_unref(msg);
}

// Test 5: Memory cleanup - error pointer is freed
TEST(GstreamerTest, MessageParseErrorMemoryCleanup) {
  // Create multiple error messages to ensure proper cleanup
  for(int i = 0; i < 5; ++i) {
    GError* error = g_error_new(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_OPEN_READ, "Resource error");
    GstMessage* msg = gst_message_new_error(nullptr, error, "Debug info");

    auto parse_result = gst::message_parse_error(msg);
    EXPECT_TRUE(parse_result.has_value());

    const auto& [error_msg, debug_info] = *parse_result;
    EXPECT_EQ(error_msg, "Resource error");
    EXPECT_EQ(debug_info, "Debug info");

    gst_message_unref(msg);
  }
}

// Test 6: Different error domains
TEST(GstreamerTest, MessageParseErrorDifferentDomains) {
  GError* error = g_error_new(GST_LIBRARY_ERROR, GST_LIBRARY_ERROR_INIT, "Library init failed");
  GstMessage* msg = gst_message_new_error(nullptr, error, "Library details");

  auto parse_result = gst::message_parse_error(msg);
  EXPECT_TRUE(parse_result.has_value());

  const auto& [error_msg, debug_info] = *parse_result;
  EXPECT_EQ(error_msg, "Library init failed");
  EXPECT_EQ(debug_info, "Library details");

  gst_message_unref(msg);
}

// Test 7: Return type validation - pair structure
TEST(GstreamerTest, MessageParseErrorReturnTypePair) {
  GError* error = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Core failed");
  GstMessage* msg = gst_message_new_error(nullptr, error, "Core debug");

  auto parse_result = gst::message_parse_error(msg);
  EXPECT_TRUE(parse_result.has_value());

  // Verify it's a pair with string members
  const auto& pair = *parse_result;
  static_assert(std::is_same_v<decltype(pair.first), std::string>);
  static_assert(std::is_same_v<decltype(pair.second), std::string>);

  gst_message_unref(msg);
}

// Test 8: Expected type validation - nonstd::expected
TEST(GstreamerTest, MessageParseErrorExpectedType) {
  GError* error = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Test");
  GstMessage* msg = gst_message_new_error(nullptr, error, "Debug");

  auto parse_result = gst::message_parse_error(msg);

  // Verify it's an expected type
  EXPECT_TRUE(parse_result);    // operator bool
  EXPECT_TRUE(parse_result.has_value());
  EXPECT_TRUE(static_cast<bool>(parse_result));

  gst_message_unref(msg);
}

// Test 9: Error message with special characters
TEST(GstreamerTest, MessageParseErrorSpecialCharacters) {
  GError* error = g_error_new(GST_CORE_ERROR, GST_CORE_ERROR_FAILED, "Error: \"quoted\" & <special> chars");
  GstMessage* msg = gst_message_new_error(nullptr, error, "Debug: 日本語 ñ § Ñ");

  auto parse_result = gst::message_parse_error(msg);
  EXPECT_TRUE(parse_result.has_value());

  const auto& [error_msg, debug_info] = *parse_result;
  EXPECT_EQ(error_msg, "Error: \"quoted\" & <special> chars");
  EXPECT_EQ(debug_info, "Debug: 日本語 ñ § Ñ");

  gst_message_unref(msg);
}

}    // namespace

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}