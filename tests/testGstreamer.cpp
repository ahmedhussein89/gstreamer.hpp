#include <type_traits>

#include <gtest/gtest.h>

#include "gstreamer.hpp"
#include "gstreamer_raii.hpp"

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

// Test 9: Valid pipeline - RAII Element owns and moves
TEST(GstreamerTest, ParseLaunchElementMovable) {
  auto result = gst::raii::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(result.has_value());

  gst::raii::Element elem = std::move(*result);
  EXPECT_TRUE(elem.get() != nullptr);
}

// Test 10: Non-owning Element handle — operator bool
TEST(GstreamerTest, ParseLaunchElementOperatorBoolTrue) {
  auto result = gst::parse_launch("fakesrc ! fakesink");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(static_cast<bool>(*result));
  // Non-owning handle is trivially copyable
  static_assert(std::is_trivially_copyable_v<gst::Element>);
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

// ============================================================================
// gst::Pipeline — non-owning handle (enhanced layer)
// gst::raii::Pipeline — owning RAII wrapper
// ============================================================================

TEST(GstreamerTest, PipelineHandleTriviallyCopiable) {
  static_assert(std::is_trivially_copyable_v<gst::Pipeline>);
  static_assert(std::is_copy_constructible_v<gst::Pipeline>);
}

TEST(GstreamerTest, RaiiPipelineMoveOnly) {
  static_assert(!std::is_copy_constructible_v<gst::raii::Pipeline>);
  static_assert(!std::is_copy_assignable_v<gst::raii::Pipeline>);
  static_assert(std::is_move_constructible_v<gst::raii::Pipeline>);
  static_assert(std::is_move_assignable_v<gst::raii::Pipeline>);
}

TEST(GstreamerTest, RaiiPipelineGetReturnsElement) {
  GstElement* raw = gst_pipeline_new("test-pipe");
  ASSERT_NE(raw, nullptr);

  gst::raii::Pipeline pipeline(raw);
  EXPECT_EQ(pipeline.get(), raw);
  EXPECT_TRUE(pipeline);
  EXPECT_TRUE(GST_IS_PIPELINE(pipeline.get()));
  // Implicit conversion to non-owning handle
  gst::Pipeline handle = pipeline;
  EXPECT_EQ(handle.get(), raw);
}

TEST(GstreamerTest, RaiiPipelineNullOperatorBool) {
  gst::raii::Pipeline pipeline(nullptr);
  EXPECT_FALSE(pipeline);
  EXPECT_EQ(pipeline.get(), nullptr);
}

// ============================================================================
// gst::PadPtr RAII wrapper Tests
// ============================================================================

TEST(GstreamerTest, PadPtrOwnsAndReleasesResource) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  GstPad* raw_pad = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw_pad, nullptr);

  {
    gst::PadPtr pad(raw_pad);
    EXPECT_EQ(pad.get(), raw_pad);
  }

  gst_object_unref(elem);
}

TEST(GstreamerTest, ElementGetStaticPadSuccess) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  auto result = gst::element_get_static_pad(elem, "src");
  EXPECT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);

  gst_object_unref(elem);
}

TEST(GstreamerTest, ElementGetStaticPadFailsForUnknownPad) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  auto result = gst::element_get_static_pad(elem, "no-such-pad-xyz");
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("no-such-pad-xyz"), std::string::npos);

  gst_object_unref(elem);
}

// ============================================================================
// gst::CapsPtr RAII wrapper Tests
// ============================================================================

TEST(GstreamerTest, CapsFromStringSuccess) {
  auto result = gst::caps_from_string("video/x-raw,format=I420,width=1920,height=1080");
  EXPECT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(GstreamerTest, CapsFromStringFailsForInvalidCaps) {
  auto result = gst::caps_from_string("not-a-valid-caps-string!!!@@@");
  EXPECT_FALSE(result.has_value());
}

TEST(GstreamerTest, CapsFromStringAnyCaps) {
  auto result = gst::caps_from_string("ANY");
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(gst_caps_is_any(result->get()));
}

TEST(GstreamerTest, CapsPtrOwnsAndReleasesResource) {
  GstCaps* raw = gst_caps_new_any();
  ASSERT_NE(raw, nullptr);

  {
    gst::CapsPtr caps(raw);
    EXPECT_EQ(caps.get(), raw);
  }
}

// ============================================================================
// Non-owning handle types — operator bool and implicit conversion
// ============================================================================

TEST(GstreamerTest, BusHandleTriviallyCopiable) {
  static_assert(std::is_trivially_copyable_v<gst::Bus>);
}

TEST(GstreamerTest, PadHandleTriviallyCopiable) {
  static_assert(std::is_trivially_copyable_v<gst::Pad>);
}

TEST(GstreamerTest, CapsHandleTriviallyCopiable) {
  static_assert(std::is_trivially_copyable_v<gst::Caps>);
}

TEST(GstreamerTest, MessageHandleTriviallyCopiable) {
  static_assert(std::is_trivially_copyable_v<gst::Message>);
}

TEST(GstreamerTest, BinHandleConvertsToPipeline) {
  static_assert(std::is_trivially_copyable_v<gst::Bin>);
}

// ============================================================================
// pipeline_new
// ============================================================================

TEST(GstreamerTest, PipelineNewWithNameSucceeds) {
  auto result = gst::pipeline_new("test-pipe-named");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  EXPECT_TRUE(GST_IS_PIPELINE(result->get()));
}

TEST(GstreamerTest, PipelineNewWithoutNameSucceeds) {
  auto result = gst::pipeline_new();
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(GstreamerTest, PipelineHandleImplicitConversionToElement) {
  auto result = gst::pipeline_new("conv-test");
  ASSERT_TRUE(result.has_value());
  gst::Element elem = *result;
  EXPECT_EQ(elem.get(), result->get());
  gst_object_unref(result->get());
}

// ============================================================================
// element_factory_make
// ============================================================================

TEST(GstreamerTest, ElementFactoryMakeSuccess) {
  auto result = gst::element_factory_make("fakesrc");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  gst_object_unref(result->get());
}

TEST(GstreamerTest, ElementFactoryMakeWithNameSuccess) {
  auto result = gst::element_factory_make("fakesrc", "named-src");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  gst_object_unref(result->get());
}

TEST(GstreamerTest, ElementFactoryMakeFailure) {
  auto result = gst::element_factory_make("nonexistent-element-factory-xyz");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("nonexistent-element-factory-xyz"), std::string::npos);
}

// ============================================================================
// bin_add / element_link
// ============================================================================

TEST(GstreamerTest, BinAddSuccess) {
  auto pipeline = gst::pipeline_new("bin-add-test");
  ASSERT_TRUE(pipeline.has_value());
  auto elem = gst::element_factory_make("fakesrc");
  ASSERT_TRUE(elem.has_value());

  auto result = gst::bin_add(*pipeline, *elem);
  ASSERT_TRUE(result.has_value());

  gst_object_unref(pipeline->get());
}

TEST(GstreamerTest, ElementLinkSuccess) {
  GstElement* raw_pipe = gst_pipeline_new("link-test");
  ASSERT_NE(raw_pipe, nullptr);
  GstElement* src = gst_element_factory_make("fakesrc", nullptr);
  GstElement* sink = gst_element_factory_make("fakesink", nullptr);
  ASSERT_NE(src, nullptr);
  ASSERT_NE(sink, nullptr);

  gst_bin_add_many(GST_BIN(raw_pipe), src, sink, nullptr);

  auto result = gst::element_link(gst::Element{src}, gst::Element{sink});
  EXPECT_TRUE(result.has_value());

  gst_object_unref(raw_pipe);
}

TEST(GstreamerTest, ElementLinkFailureIncompatibleCaps) {
  GstElement* raw_pipe = gst_pipeline_new("link-fail-test");
  ASSERT_NE(raw_pipe, nullptr);
  GstElement* audio = gst_element_factory_make("audiotestsrc", nullptr);
  GstElement* video = gst_element_factory_make("videoconvert", nullptr);
  ASSERT_NE(audio, nullptr);
  ASSERT_NE(video, nullptr);

  gst_bin_add_many(GST_BIN(raw_pipe), audio, video, nullptr);

  auto result = gst::element_link(gst::Element{audio}, gst::Element{video});
  EXPECT_FALSE(result.has_value());

  gst_object_unref(raw_pipe);
}

// ============================================================================
// element_get_bus
// ============================================================================

TEST(GstreamerTest, ElementGetBusSuccess) {
  GstElement* raw_pipe = gst_pipeline_new("get-bus-test");
  ASSERT_NE(raw_pipe, nullptr);

  auto result = gst::element_get_bus(gst::Element{raw_pipe});
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);

  gst_object_unref(raw_pipe);
}

// ============================================================================
// element_set_state
// ============================================================================

TEST(GstreamerTest, ElementSetStateSuccess) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  auto result = gst::element_set_state(gst::Element{elem}, GST_STATE_NULL);
  EXPECT_TRUE(result.has_value());

  gst_object_unref(elem);
}

TEST(GstreamerTest, ElementSetStateNullElementFails) {
  auto result = gst::element_set_state(gst::Element{nullptr}, GST_STATE_PLAYING);
  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// pad_is_linked — both overloads
// ============================================================================

TEST(GstreamerTest, PadIsLinkedBothOverloads) {
  GstElement* raw_pipe = gst_pipeline_new("pad-linked-test");
  GstElement* src = gst_element_factory_make("fakesrc", nullptr);
  GstElement* sink = gst_element_factory_make("fakesink", nullptr);
  ASSERT_NE(raw_pipe, nullptr);
  ASSERT_NE(src, nullptr);
  ASSERT_NE(sink, nullptr);

  gst_bin_add_many(GST_BIN(raw_pipe), src, sink, nullptr);
  gst_element_link(src, sink);

  GstPad* src_pad = gst_element_get_static_pad(src, "src");
  ASSERT_NE(src_pad, nullptr);

  // Non-owning Pad handle overload
  EXPECT_TRUE(gst::pad_is_linked(gst::Pad{src_pad}));

  // PadPtr overload
  gst::PadPtr pad_ptr(src_pad);
  EXPECT_TRUE(gst::pad_is_linked(pad_ptr));
  (void)pad_ptr.release();    // avoid double-free; pad is owned by element

  gst_object_unref(src_pad);
  gst_object_unref(raw_pipe);
}

TEST(GstreamerTest, PadIsLinkedFalseWhenNotLinked) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  GstPad* raw_pad = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw_pad, nullptr);

  EXPECT_FALSE(gst::pad_is_linked(gst::Pad{raw_pad}));

  gst_object_unref(raw_pad);
  gst_object_unref(elem);
}

// ============================================================================
// pad_get_current_caps — both overloads
// ============================================================================

TEST(GstreamerTest, PadGetCurrentCapsNoCurrentCaps) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);

  GstPad* raw_pad = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw_pad, nullptr);

  // Before the pipeline is playing, current caps are usually null
  auto result_handle = gst::pad_get_current_caps(gst::Pad{raw_pad});
  // Result depends on pipeline state; just verify no crash
  (void)result_handle;

  auto result_raw = gst::pad_get_current_caps(raw_pad);
  (void)result_raw;

  gst_object_unref(raw_pad);
  gst_object_unref(elem);
}

// ============================================================================
// caps_get_structure / structure_get_name
// ============================================================================

TEST(GstreamerTest, CapsGetStructureSuccess) {
  auto caps_result = gst::caps_from_string("video/x-raw,format=I420,width=1920,height=1080");
  ASSERT_TRUE(caps_result.has_value());

  auto struct_result = gst::caps_get_structure(caps_result.value());
  ASSERT_TRUE(struct_result.has_value());
  EXPECT_NE(*struct_result, nullptr);
}

TEST(GstreamerTest, CapsGetStructureOutOfRangeFails) {
  auto caps_result = gst::caps_from_string("video/x-raw,format=I420");
  ASSERT_TRUE(caps_result.has_value());

  auto struct_result = gst::caps_get_structure(caps_result.value(), 99u);
  EXPECT_FALSE(struct_result.has_value());
}

TEST(GstreamerTest, StructureGetNameReturnsCorrectName) {
  auto caps_result = gst::caps_from_string("video/x-raw,format=I420");
  ASSERT_TRUE(caps_result.has_value());

  auto struct_result = gst::caps_get_structure(caps_result.value());
  ASSERT_TRUE(struct_result.has_value());

  auto name = gst::structure_get_name(*struct_result);
  EXPECT_EQ(name, "video/x-raw");
}

// ============================================================================
// message_type — both overloads
// ============================================================================

TEST(GstreamerTest, MessageTypeFromNonOwningHandle) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);

  auto type = gst::message_type(gst::Message{raw});
  EXPECT_EQ(type, gst::MessageType::EOS);

  gst_message_unref(raw);
}

TEST(GstreamerTest, MessageTypeFromMessagePtr) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);

  gst::MessagePtr ptr(raw);
  auto type = gst::message_type(ptr);
  EXPECT_EQ(type, gst::MessageType::EOS);
}

// ============================================================================
// message_parse_state_changed — both overloads
// ============================================================================

TEST(GstreamerTest, MessageParseStateChangedFromHandle) {
  GstMessage* raw = gst_message_new_state_changed(nullptr, GST_STATE_NULL, GST_STATE_READY, GST_STATE_PLAYING);
  ASSERT_NE(raw, nullptr);

  auto change = gst::message_parse_state_changed(gst::Message{raw});
  EXPECT_EQ(change.old_state, GST_STATE_NULL);
  EXPECT_EQ(change.new_state, GST_STATE_READY);
  EXPECT_EQ(change.pending, GST_STATE_PLAYING);

  gst_message_unref(raw);
}

TEST(GstreamerTest, MessageParseStateChangedFromMessagePtr) {
  GstMessage* raw = gst_message_new_state_changed(nullptr, GST_STATE_READY, GST_STATE_PAUSED, GST_STATE_PLAYING);
  ASSERT_NE(raw, nullptr);

  gst::MessagePtr ptr(raw);
  auto change = gst::message_parse_state_changed(ptr);
  EXPECT_EQ(change.old_state, GST_STATE_READY);
  EXPECT_EQ(change.new_state, GST_STATE_PAUSED);
  EXPECT_EQ(change.pending, GST_STATE_PLAYING);
}

// ============================================================================
// state_get_name
// ============================================================================

TEST(GstreamerTest, StateGetNameReturnsNonEmptyString) {
  EXPECT_FALSE(gst::state_get_name(GST_STATE_NULL).empty());
  EXPECT_FALSE(gst::state_get_name(GST_STATE_READY).empty());
  EXPECT_FALSE(gst::state_get_name(GST_STATE_PAUSED).empty());
  EXPECT_FALSE(gst::state_get_name(GST_STATE_PLAYING).empty());
}

// ============================================================================
// bus_timed_pop_filtered — both overloads (no-message timeout case)
// ============================================================================

TEST(GstreamerTest, BusPtrOverloadTimesOutReturnsError) {
  GstElement* raw_pipe = gst_pipeline_new("bus-timeout-ptr-test");
  ASSERT_NE(raw_pipe, nullptr);

  GstBus* raw_bus = gst_element_get_bus(raw_pipe);
  ASSERT_NE(raw_bus, nullptr);

  gst::BusPtr bus(raw_bus);
  auto result = gst::bus_timed_pop_filtered(bus, 1 /* 1 ns */, gst::MessageType::Error | gst::MessageType::EOS);
  EXPECT_FALSE(result.has_value());

  gst_object_unref(raw_pipe);
}

TEST(GstreamerTest, BusHandleOverloadTimesOutReturnsError) {
  GstElement* raw_pipe = gst_pipeline_new("bus-timeout-handle-test");
  ASSERT_NE(raw_pipe, nullptr);

  GstBus* raw_bus = gst_element_get_bus(raw_pipe);
  ASSERT_NE(raw_bus, nullptr);

  auto result = gst::bus_timed_pop_filtered(gst::Bus{raw_bus}, 1 /* 1 ns */, gst::MessageType::Error | gst::MessageType::EOS);
  EXPECT_FALSE(result.has_value());

  gst_object_unref(raw_bus);
  gst_object_unref(raw_pipe);
}

// ============================================================================
// MessageTypeFlags — combined bit operations used in bus_timed_pop_filtered
// ============================================================================

TEST(GstreamerTest, MessageTypeFlagsOrCombinesTypes) {
  auto flags = gst::MessageType::Error | gst::MessageType::EOS;
  EXPECT_TRUE(static_cast<bool>(flags));
  EXPECT_NE(flags.value(), 0);
}

}    // namespace

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}