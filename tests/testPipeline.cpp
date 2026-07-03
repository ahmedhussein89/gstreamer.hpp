#include <gst/gst.h>
#include <gtest/gtest.h>

#include <gstreamer_raii.hpp>

class PipelineTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() { gst_init(nullptr, nullptr); }
};

TEST_F(PipelineTest, BuildEmptyPipelineReturnsError) {
  auto result = gst::build(gst::PipelineDesc{});
  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST_F(PipelineTest, BuildSingleElementPipeline) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"fakesrc"}});
  EXPECT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST_F(PipelineTest, BuildLinearPipeline) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"fakesrc"}, gst::Node{"fakesink"}});
  EXPECT_TRUE(result.has_value());
  EXPECT_NE(result->get(), nullptr);
}

TEST_F(PipelineTest, BuildLinearPipelineThreeElements) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"videotestsrc"}, gst::Node{"videoconvert"}, gst::Node{"fakesink"}});
  EXPECT_TRUE(result.has_value());
}

TEST_F(PipelineTest, BuildFailsForUnknownFactory) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"nonexistent-element-xyz"}});
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("nonexistent-element-xyz"), std::string::npos);
}

TEST_F(PipelineTest, BuildFailsForUnknownFactoryInChain) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"fakesrc"}, gst::Node{"no-such-element-abc"}});
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("no-such-element-abc"), std::string::npos);
}

TEST_F(PipelineTest, NodePropChainingStoresAllProperties) {
  auto node = gst::Node{"fakesrc"}.prop("num-buffers", 10).prop("signal-handoffs", true);
  EXPECT_EQ(node.properties.size(), 2u);
  EXPECT_EQ(node.properties[0].first, "num-buffers");
  EXPECT_EQ(node.properties[1].first, "signal-handoffs");
}

TEST_F(PipelineTest, BuildWithIntPropertyApplied) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"fakesrc", "src"}.prop("num-buffers", 42), gst::Node{"fakesink"}});
  ASSERT_TRUE(result.has_value());

  GstElement* src = gst_bin_get_by_name(GST_BIN(result->get()), "src");
  ASSERT_NE(src, nullptr);

  gint val = 0;
  g_object_get(G_OBJECT(src), "num-buffers", &val, nullptr);
  EXPECT_EQ(val, 42);

  gst_object_unref(src);
}

TEST_F(PipelineTest, BuildWithBoolPropertyApplied) {
  auto result =
      gst::build(gst::PipelineDesc{gst::Node{"fakesrc", "src"}.prop("signal-handoffs", true), gst::Node{"fakesink"}});
  ASSERT_TRUE(result.has_value());

  GstElement* src = gst_bin_get_by_name(GST_BIN(result->get()), "src");
  ASSERT_NE(src, nullptr);

  gboolean val = FALSE;
  g_object_get(G_OBJECT(src), "signal-handoffs", &val, nullptr);
  EXPECT_TRUE(val);

  gst_object_unref(src);
}

TEST_F(PipelineTest, PropertyValueVariantHoldsCorrectTypes) {
  gst::PropertyValue bool_val{true};
  gst::PropertyValue int_val{42};
  gst::PropertyValue str_val{std::string{"hello"}};

  EXPECT_TRUE(std::holds_alternative<bool>(bool_val));
  EXPECT_TRUE(std::holds_alternative<int>(int_val));
  EXPECT_TRUE(std::holds_alternative<std::string>(str_val));
}

TEST_F(PipelineTest, BuiltPipelineReturnsGstPipelineType) {
  auto result = gst::build(gst::PipelineDesc{gst::Node{"fakesrc"}, gst::Node{"fakesink"}});
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(GST_IS_PIPELINE(result->get()));
}
