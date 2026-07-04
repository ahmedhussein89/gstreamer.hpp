#include <type_traits>

#include <gst/gst.h>
#include <gtest/gtest.h>

#include <gstreamer_raii.hpp>

namespace {

// ============================================================================
// gst::raii::Bus — owning wrapper
// ============================================================================

TEST(RaiiBusTest, ConstructFromRawPointer) {
  GstBus* raw = gst_bus_new();
  ASSERT_NE(raw, nullptr);
  gst::raii::Bus bus(raw);
  EXPECT_EQ(bus.get(), raw);
  EXPECT_TRUE(static_cast<bool>(bus));
}

TEST(RaiiBusTest, ConstructFromBusPtr) {
  GstBus* raw = gst_bus_new();
  ASSERT_NE(raw, nullptr);
  gst::BusPtr ptr(raw);
  gst::raii::Bus bus(std::move(ptr));
  EXPECT_EQ(bus.get(), raw);
  EXPECT_TRUE(static_cast<bool>(bus));
}

TEST(RaiiBusTest, DefaultConstructorIsNull) {
  gst::raii::Bus bus;
  EXPECT_EQ(bus.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(bus));
}

TEST(RaiiBusTest, ImplicitConversionToHandle) {
  GstBus* raw = gst_bus_new();
  ASSERT_NE(raw, nullptr);
  gst::raii::Bus bus(raw);
  gst::Bus handle = bus;
  EXPECT_EQ(handle.get(), raw);
}

TEST(RaiiBusTest, ReleaseTransfersOwnership) {
  GstBus* raw = gst_bus_new();
  ASSERT_NE(raw, nullptr);
  gst::raii::Bus bus(raw);
  GstBus* released = bus.release();
  EXPECT_EQ(released, raw);
  EXPECT_EQ(bus.get(), nullptr);
  gst_object_unref(released);
}

TEST(RaiiBusTest, MoveOnly) {
  static_assert(!std::is_copy_constructible_v<gst::raii::Bus>);
  static_assert(!std::is_copy_assignable_v<gst::raii::Bus>);
  static_assert(std::is_move_constructible_v<gst::raii::Bus>);
  static_assert(std::is_move_assignable_v<gst::raii::Bus>);
}

// ============================================================================
// gst::raii::Pad — owning wrapper
// ============================================================================

TEST(RaiiPadTest, ConstructFromRawPointer) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);
  GstPad* raw = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw, nullptr);
  {
    gst::raii::Pad pad(raw);
    EXPECT_EQ(pad.get(), raw);
    EXPECT_TRUE(static_cast<bool>(pad));
  }
  gst_object_unref(elem);
}

TEST(RaiiPadTest, ConstructFromPadPtr) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);
  GstPad* raw = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw, nullptr);
  {
    gst::PadPtr ptr(raw);
    gst::raii::Pad pad(std::move(ptr));
    EXPECT_EQ(pad.get(), raw);
    EXPECT_TRUE(static_cast<bool>(pad));
  }
  gst_object_unref(elem);
}

TEST(RaiiPadTest, DefaultConstructorIsNull) {
  gst::raii::Pad pad;
  EXPECT_EQ(pad.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(pad));
}

TEST(RaiiPadTest, ImplicitConversionToHandle) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);
  GstPad* raw = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw, nullptr);
  {
    gst::raii::Pad pad(raw);
    gst::Pad handle = pad;
    EXPECT_EQ(handle.get(), raw);
  }
  gst_object_unref(elem);
}

TEST(RaiiPadTest, ReleaseTransfersOwnership) {
  GstElement* elem = gst_element_factory_make("fakesrc", nullptr);
  ASSERT_NE(elem, nullptr);
  GstPad* raw = gst_element_get_static_pad(elem, "src");
  ASSERT_NE(raw, nullptr);
  {
    gst::raii::Pad pad(raw);
    GstPad* released = pad.release();
    EXPECT_EQ(released, raw);
    EXPECT_EQ(pad.get(), nullptr);
    gst_object_unref(released);
  }
  gst_object_unref(elem);
}

TEST(RaiiPadTest, MoveOnly) {
  static_assert(!std::is_copy_constructible_v<gst::raii::Pad>);
  static_assert(!std::is_copy_assignable_v<gst::raii::Pad>);
  static_assert(std::is_move_constructible_v<gst::raii::Pad>);
  static_assert(std::is_move_assignable_v<gst::raii::Pad>);
}

// ============================================================================
// gst::raii::Caps — owning wrapper
// ============================================================================

TEST(RaiiCapsTest, ConstructFromRawPointer) {
  GstCaps* raw = gst_caps_new_any();
  ASSERT_NE(raw, nullptr);
  gst::raii::Caps caps(raw);
  EXPECT_EQ(caps.get(), raw);
  EXPECT_TRUE(static_cast<bool>(caps));
}

TEST(RaiiCapsTest, ConstructFromCapsPtr) {
  GstCaps* raw = gst_caps_new_any();
  ASSERT_NE(raw, nullptr);
  gst::CapsPtr ptr(raw);
  gst::raii::Caps caps(std::move(ptr));
  EXPECT_EQ(caps.get(), raw);
}

TEST(RaiiCapsTest, DefaultConstructorIsNull) {
  gst::raii::Caps caps;
  EXPECT_EQ(caps.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(caps));
}

TEST(RaiiCapsTest, ImplicitConversionToHandle) {
  GstCaps* raw = gst_caps_new_any();
  ASSERT_NE(raw, nullptr);
  gst::raii::Caps caps(raw);
  gst::Caps handle = caps;
  EXPECT_EQ(handle.get(), raw);
}

TEST(RaiiCapsTest, ReleaseTransfersOwnership) {
  GstCaps* raw = gst_caps_new_any();
  ASSERT_NE(raw, nullptr);
  gst::raii::Caps caps(raw);
  GstCaps* released = caps.release();
  EXPECT_EQ(released, raw);
  EXPECT_EQ(caps.get(), nullptr);
  gst_caps_unref(released);
}

TEST(RaiiCapsTest, MoveOnly) {
  static_assert(!std::is_copy_constructible_v<gst::raii::Caps>);
  static_assert(!std::is_copy_assignable_v<gst::raii::Caps>);
  static_assert(std::is_move_constructible_v<gst::raii::Caps>);
  static_assert(std::is_move_assignable_v<gst::raii::Caps>);
}

// ============================================================================
// gst::raii::Message — owning wrapper
// ============================================================================

TEST(RaiiMessageTest, ConstructFromRawPointer) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);
  gst::raii::Message msg(raw);
  EXPECT_EQ(msg.get(), raw);
  EXPECT_TRUE(static_cast<bool>(msg));
}

TEST(RaiiMessageTest, ConstructFromMessagePtr) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);
  gst::MessagePtr ptr(raw);
  gst::raii::Message msg(std::move(ptr));
  EXPECT_EQ(msg.get(), raw);
}

TEST(RaiiMessageTest, DefaultConstructorIsNull) {
  gst::raii::Message msg;
  EXPECT_EQ(msg.get(), nullptr);
  EXPECT_FALSE(static_cast<bool>(msg));
}

TEST(RaiiMessageTest, ImplicitConversionToHandle) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);
  gst::raii::Message msg(raw);
  gst::Message handle = msg;
  EXPECT_EQ(handle.get(), raw);
}

TEST(RaiiMessageTest, ReleaseTransfersOwnership) {
  GstMessage* raw = gst_message_new_eos(nullptr);
  ASSERT_NE(raw, nullptr);
  gst::raii::Message msg(raw);
  GstMessage* released = msg.release();
  EXPECT_EQ(released, raw);
  EXPECT_EQ(msg.get(), nullptr);
  gst_message_unref(released);
}

TEST(RaiiMessageTest, MoveOnly) {
  static_assert(!std::is_copy_constructible_v<gst::raii::Message>);
  static_assert(!std::is_copy_assignable_v<gst::raii::Message>);
  static_assert(std::is_move_constructible_v<gst::raii::Message>);
  static_assert(std::is_move_assignable_v<gst::raii::Message>);
}

// ============================================================================
// gst::raii factory functions
// ============================================================================

TEST(RaiiFactoryTest, PipelineNewSucceedsWithName) {
  auto result = gst::raii::pipeline_new("raii-pipe-named");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  EXPECT_TRUE(GST_IS_PIPELINE(result->get()));
}

TEST(RaiiFactoryTest, PipelineNewSucceedsWithoutName) {
  auto result = gst::raii::pipeline_new();
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
  EXPECT_TRUE(GST_IS_PIPELINE(result->get()));
}

TEST(RaiiFactoryTest, ElementFactoryMakeSuccess) {
  auto result = gst::raii::element_factory_make("fakesrc");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(RaiiFactoryTest, ElementFactoryMakeWithNameSuccess) {
  auto result = gst::raii::element_factory_make("fakesrc", "raii-named-src");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(RaiiFactoryTest, ElementFactoryMakeFailure) {
  auto result = gst::raii::element_factory_make("nonexistent-factory-xyz");
  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("nonexistent-factory-xyz"), std::string::npos);
}

TEST(RaiiFactoryTest, BinAddSuccess) {
  auto pipeline = gst::raii::pipeline_new("bin-add-raii-test");
  ASSERT_TRUE(pipeline.has_value());
  auto elem = gst::raii::element_factory_make("fakesrc");
  ASSERT_TRUE(elem.has_value());
  auto result = gst::raii::bin_add(*pipeline, std::move(*elem));
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(RaiiFactoryTest, BinAddFailureOnDuplicate) {
  auto pipeline = gst::raii::pipeline_new("bin-add-dup-test");
  ASSERT_TRUE(pipeline.has_value());
  auto elem1 = gst::raii::element_factory_make("fakesrc", "dup-elem");
  ASSERT_TRUE(elem1.has_value());
  auto elem2 = gst::raii::element_factory_make("fakesink", "dup-elem");
  ASSERT_TRUE(elem2.has_value());
  // Add first element — should succeed
  auto r1 = gst::raii::bin_add(*pipeline, std::move(*elem1));
  ASSERT_TRUE(r1.has_value());
  // Add second element with same name — should fail (duplicate name in bin)
  auto r2 = gst::raii::bin_add(*pipeline, std::move(*elem2));
  EXPECT_FALSE(r2.has_value());
}

TEST(RaiiFactoryTest, ElementGetBusSuccess) {
  auto pipeline = gst::raii::pipeline_new("get-bus-test");
  ASSERT_TRUE(pipeline.has_value());
  auto result = gst::raii::element_get_bus(gst::Element{pipeline->get()});
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(RaiiFactoryTest, ElementGetStaticPadSuccess) {
  auto elem = gst::raii::element_factory_make("fakesrc");
  ASSERT_TRUE(elem.has_value());
  auto result = gst::raii::element_get_static_pad(gst::Element{elem->get()}, "src");
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST(RaiiFactoryTest, ElementGetStaticPadFailure) {
  auto elem = gst::raii::element_factory_make("fakesrc");
  ASSERT_TRUE(elem.has_value());
  auto result = gst::raii::element_get_static_pad(gst::Element{elem->get()}, "nonexistent-pad-xyz");
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("nonexistent-pad-xyz"), std::string::npos);
}

}    // namespace

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
