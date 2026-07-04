#include <array>
#include <string>
#include <vector>

#include <gst/gst.h>
#include <gtest/gtest.h>

#include <core/array_proxy.hpp>
#include <core/enums.hpp>
#include <core/flags.hpp>
#include <gstreamer.hpp>

// ============================================================================
// ArrayProxy<T> — all constructors and member functions
// ============================================================================

TEST(ArrayProxyTest, DefaultConstructorIsEmpty) {
  gst::ArrayProxy<int> proxy;
  EXPECT_EQ(proxy.size(), 0u);
  EXPECT_EQ(proxy.data(), nullptr);
  EXPECT_TRUE(proxy.empty());
}

TEST(ArrayProxyTest, SingleElementConstructor) {
  int value = 42;
  gst::ArrayProxy<int> proxy(value);
  EXPECT_EQ(proxy.size(), 1u);
  EXPECT_FALSE(proxy.empty());
  EXPECT_EQ(proxy.data(), &value);
  EXPECT_EQ(proxy[0], 42);
}

TEST(ArrayProxyTest, InitializerListConstructor) {
  auto fn = [](gst::ArrayProxy<int> proxy) {
    ASSERT_EQ(proxy.size(), 3u);
    EXPECT_EQ(proxy[0], 1);
    EXPECT_EQ(proxy[1], 2);
    EXPECT_EQ(proxy[2], 3);
  };
  fn({1, 2, 3});
}

TEST(ArrayProxyTest, StdArrayConstructor) {
  std::array<int, 4> arr = {10, 20, 30, 40};
  gst::ArrayProxy<int> proxy(arr);
  EXPECT_EQ(proxy.size(), 4u);
  EXPECT_EQ(proxy[0], 10);
  EXPECT_EQ(proxy[3], 40);
}

TEST(ArrayProxyTest, StdVectorConstructor) {
  std::vector<int> vec = {5, 6, 7};
  gst::ArrayProxy<int> proxy(vec);
  EXPECT_EQ(proxy.size(), 3u);
  EXPECT_EQ(proxy[0], 5);
  EXPECT_EQ(proxy[2], 7);
}

TEST(ArrayProxyTest, RawPointerAndSizeConstructor) {
  int arr[] = {100, 200, 300};
  gst::ArrayProxy<int> proxy(arr, 3u);
  EXPECT_EQ(proxy.size(), 3u);
  EXPECT_EQ(proxy[0], 100);
  EXPECT_EQ(proxy[2], 300);
}

TEST(ArrayProxyTest, BeginEndIteration) {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  gst::ArrayProxy<int> proxy(vec);
  int sum = 0;
  for(auto it = proxy.begin(); it != proxy.end(); ++it) {
    sum += *it;
  }
  EXPECT_EQ(sum, 15);
}

TEST(ArrayProxyTest, RangeForLoop) {
  std::vector<int> vec = {10, 20, 30};
  gst::ArrayProxy<int> proxy(vec);
  int sum = 0;
  for(const int v : proxy) {
    sum += v;
  }
  EXPECT_EQ(sum, 60);
}

TEST(ArrayProxyTest, StringElements) {
  std::vector<std::string> vec = {"hello", "world"};
  gst::ArrayProxy<std::string> proxy(vec);
  EXPECT_EQ(proxy.size(), 2u);
  EXPECT_EQ(proxy[0], "hello");
  EXPECT_EQ(proxy[1], "world");
}

TEST(ArrayProxyTest, EmptyVectorIsEmpty) {
  std::vector<int> vec;
  gst::ArrayProxy<int> proxy(vec);
  EXPECT_TRUE(proxy.empty());
  EXPECT_EQ(proxy.size(), 0u);
}

// ============================================================================
// Flags<MessageType> — runtime operator and accessor tests
// ============================================================================

TEST(FlagsTest, DefaultConstructorHasZeroMask) {
  gst::MessageTypeFlags flags;
  EXPECT_EQ(flags.value(), 0);
  EXPECT_FALSE(static_cast<bool>(flags));
}

TEST(FlagsTest, ConstructFromBitEnumIsNonZero) {
  gst::MessageTypeFlags flags{gst::MessageType::EOS};
  EXPECT_NE(flags.value(), 0);
  EXPECT_TRUE(static_cast<bool>(flags));
}

TEST(FlagsTest, ExplicitMaskConstructor) {
  auto mask = static_cast<gst::MessageTypeFlags::MaskType>(GST_MESSAGE_ERROR);
  gst::MessageTypeFlags flags{mask};
  EXPECT_EQ(flags.value(), mask);
}

TEST(FlagsTest, OperatorOrCombinesBits) {
  gst::MessageTypeFlags a{gst::MessageType::EOS};
  gst::MessageTypeFlags b{gst::MessageType::Error};
  auto c = a | b;
  EXPECT_EQ(c.value(), a.value() | b.value());
}

TEST(FlagsTest, OperatorAndMasksBits) {
  gst::MessageTypeFlags a{gst::MessageType::Any};
  gst::MessageTypeFlags b{gst::MessageType::EOS};
  auto c = a & b;
  EXPECT_NE(c.value(), 0);
}

TEST(FlagsTest, OperatorXorZerosIdenticalMasks) {
  gst::MessageTypeFlags a{gst::MessageType::EOS};
  gst::MessageTypeFlags b{gst::MessageType::EOS};
  auto c = a ^ b;
  EXPECT_EQ(c.value(), 0);
}

TEST(FlagsTest, OperatorNotInvertsMask) {
  gst::MessageTypeFlags flags{gst::MessageType::Unknown};
  auto notFlags = ~flags;
  EXPECT_NE(notFlags.value(), flags.value());
}

TEST(FlagsTest, CompoundOrAssignAccumulatesBits) {
  gst::MessageTypeFlags flags{gst::MessageType::EOS};
  gst::MessageTypeFlags::MaskType original = flags.value();
  flags |= gst::MessageTypeFlags{gst::MessageType::Error};
  EXPECT_NE(flags.value(), original);
}

TEST(FlagsTest, CompoundAndAssign) {
  gst::MessageTypeFlags flags{gst::MessageType::Any};
  flags &= gst::MessageTypeFlags{gst::MessageType::EOS};
  EXPECT_NE(flags.value(), 0);
}

TEST(FlagsTest, CompoundXorAssignCancels) {
  gst::MessageTypeFlags flags{gst::MessageType::EOS};
  flags ^= gst::MessageTypeFlags{gst::MessageType::EOS};
  EXPECT_EQ(flags.value(), 0);
}

TEST(FlagsTest, EqualityOperator) {
  gst::MessageTypeFlags a{gst::MessageType::EOS};
  gst::MessageTypeFlags b{gst::MessageType::EOS};
  gst::MessageTypeFlags c{gst::MessageType::Error};
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(FlagsTest, ExplicitMaskTypeCast) {
  gst::MessageTypeFlags flags{gst::MessageType::EOS};
  auto mask = static_cast<gst::MessageTypeFlags::MaskType>(flags);
  EXPECT_EQ(mask, flags.value());
}

TEST(FlagsTest, AdlBitOrOperator) {
  auto flags = gst::MessageType::EOS | gst::MessageType::Error;
  EXPECT_NE(flags.value(), 0);
}

TEST(FlagsTest, AdlBitAndOperator) {
  auto flags = gst::MessageType::Any & gst::MessageType::EOS;
  EXPECT_NE(flags.value(), 0);
}

TEST(FlagsTest, AdlBitXorOperatorSameValueIsZero) {
  auto flags = gst::MessageType::EOS ^ gst::MessageType::EOS;
  EXPECT_EQ(flags.value(), 0);
}

TEST(FlagsTest, AdlBitNotOperatorProducesNonZero) {
  auto flags = ~gst::MessageType::Unknown;
  EXPECT_TRUE(static_cast<bool>(flags));
}

// ============================================================================
// gst::State and other enums — to_string() coverage
// ============================================================================

TEST(EnumsTest, StateToStringNonEmpty) {
  EXPECT_FALSE(gst::to_string(gst::State::VoidPending).empty());
  EXPECT_FALSE(gst::to_string(gst::State::Null).empty());
  EXPECT_FALSE(gst::to_string(gst::State::Ready).empty());
  EXPECT_FALSE(gst::to_string(gst::State::Paused).empty());
  EXPECT_FALSE(gst::to_string(gst::State::Playing).empty());
}

TEST(EnumsTest, StateChangeReturnToStringNonEmpty) {
  EXPECT_FALSE(gst::to_string(gst::StateChangeReturn::Failure).empty());
  EXPECT_FALSE(gst::to_string(gst::StateChangeReturn::Success).empty());
  EXPECT_FALSE(gst::to_string(gst::StateChangeReturn::Async).empty());
  EXPECT_FALSE(gst::to_string(gst::StateChangeReturn::NoPreroll).empty());
}

TEST(EnumsTest, FlowReturnToStringNonEmpty) {
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::Ok).empty());
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::Eos).empty());
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::Error).empty());
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::NotLinked).empty());
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::Flushing).empty());
  EXPECT_FALSE(gst::to_string(gst::FlowReturn::NotNegotiated).empty());
}

TEST(EnumsTest, PadDirectionToString) {
  EXPECT_EQ(gst::to_string(gst::PadDirection::Src), "src");
  EXPECT_EQ(gst::to_string(gst::PadDirection::Sink), "sink");
  EXPECT_EQ(gst::to_string(gst::PadDirection::Unknown), "unknown");
}

TEST(EnumsTest, PadPresenceToString) {
  EXPECT_EQ(gst::to_string(gst::PadPresence::Always), "always");
  EXPECT_EQ(gst::to_string(gst::PadPresence::Sometimes), "sometimes");
  EXPECT_EQ(gst::to_string(gst::PadPresence::Request), "request");
}

TEST(EnumsTest, DISABLED_FormatToStringNonEmpty) {
  EXPECT_FALSE(gst::to_string(gst::Format::Undefined).empty());
  EXPECT_FALSE(gst::to_string(gst::Format::Default).empty());
  EXPECT_FALSE(gst::to_string(gst::Format::Bytes).empty());
  EXPECT_FALSE(gst::to_string(gst::Format::Time).empty());
  EXPECT_FALSE(gst::to_string(gst::Format::Buffers).empty());
  EXPECT_FALSE(gst::to_string(gst::Format::Percent).empty());
}

TEST(EnumsTest, SeekTypeToString) {
  EXPECT_EQ(gst::to_string(gst::SeekType::None), "none");
  EXPECT_EQ(gst::to_string(gst::SeekType::Set), "set");
  EXPECT_EQ(gst::to_string(gst::SeekType::End), "end");
}

int main(int argc, char** argv) {
  gst_init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
