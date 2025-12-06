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
  EXPECT_TRUE(*result);  // Test operator bool
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
    "fakesink"
  );
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

}    // namespace

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}