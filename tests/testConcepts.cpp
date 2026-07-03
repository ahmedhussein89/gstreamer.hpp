#include <concepts>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <core/concepts.hpp>
#include <core/flags.hpp>
#include <deepstream_raii.hpp>

// ============================================================================
// gst::GstHandlePointer
// ============================================================================

// Satisfied by GObject/Gst struct types (C structs are class types in C++).
static_assert(gst::GstHandlePointer<GstElement>);
static_assert(gst::GstHandlePointer<GstPad>);
static_assert(gst::GstHandlePointer<GstBus>);
static_assert(gst::GstHandlePointer<GstCaps>);
static_assert(gst::GstHandlePointer<GstMessage>);
static_assert(gst::GstHandlePointer<GstStructure>);
static_assert(gst::GstHandlePointer<GstBuffer>);

// Rejected: primitive/non-class types.
static_assert(!gst::GstHandlePointer<int>);
static_assert(!gst::GstHandlePointer<float>);
static_assert(!gst::GstHandlePointer<void*>);
static_assert(!gst::GstHandlePointer<GstElement*>);   // pointer, not class

// ============================================================================
// gst::FlagEnum
// ============================================================================

// Satisfied by scoped enums.
static_assert(gst::FlagEnum<gst::MessageType>);
static_assert(gst::FlagEnum<gst::State>);
static_assert(gst::FlagEnum<gst::FlowReturn>);
static_assert(gst::FlagEnum<gst::PadDirection>);

// Rejected: non-enum types.
static_assert(!gst::FlagEnum<int>);
static_assert(!gst::FlagEnum<bool>);
static_assert(!gst::FlagEnum<std::string>);
static_assert(!gst::FlagEnum<GstElement>);

// ============================================================================
// gst::ArrayElement
// ============================================================================

// Satisfied by copyable types.
static_assert(gst::ArrayElement<int>);
static_assert(gst::ArrayElement<double>);
static_assert(gst::ArrayElement<std::string>);
static_assert(gst::ArrayElement<GstElement*>);

// Rejected: move-only (non-copyable) types.
static_assert(!gst::ArrayElement<std::unique_ptr<int>>);

// ============================================================================
// gst::PropertyValueType  (defined in gstreamer.hpp)
// ============================================================================

// Satisfied by the exact variant alternatives.
static_assert(gst::PropertyValueType<bool>);
static_assert(gst::PropertyValueType<std::int32_t>);
static_assert(gst::PropertyValueType<std::uint32_t>);
static_assert(gst::PropertyValueType<std::int64_t>);
static_assert(gst::PropertyValueType<std::uint64_t>);
static_assert(gst::PropertyValueType<double>);
static_assert(gst::PropertyValueType<std::string>);

// Also accepted: types implicitly convertible to a variant alternative.
static_assert(gst::PropertyValueType<const char*>);  // converts to std::string
static_assert(gst::PropertyValueType<float>);        // converts to double

// Rejected: types with no conversion into any variant alternative.
static_assert(!gst::PropertyValueType<std::vector<int>>);
static_assert(!gst::PropertyValueType<int*>);

// ============================================================================
// gst::PipelineNodeType  (defined in gstreamer.hpp)
// ============================================================================

// Satisfied by gst::Node and its reference/cv-qualified forms.
static_assert(gst::PipelineNodeType<gst::Node>);
static_assert(gst::PipelineNodeType<gst::Node&>);
static_assert(gst::PipelineNodeType<gst::Node&&>);
static_assert(gst::PipelineNodeType<const gst::Node&>);

// Rejected: unrelated types.
static_assert(!gst::PipelineNodeType<int>);
static_assert(!gst::PipelineNodeType<std::string>);

// ============================================================================
// ds::DsElement  (defined in core/concepts.hpp)
// ============================================================================

// Satisfied by gst::raii types and ds:: element types via their get()/release()
// returning GstElement*.  Tested indirectly — verify the concept itself rejects
// obvious non-elements.
static_assert(!ds::DsElement<int>);
static_assert(!ds::DsElement<std::string>);
static_assert(!ds::DsElement<GstElement*>);

// A mock that satisfies the concept (structural).
struct MockElement {
  GstElement* get() const { return nullptr; }
  GstElement* release() { return nullptr; }
};
static_assert(ds::DsElement<MockElement>);

// A mock missing release() is rejected.
struct MockElementNoRelease {
  GstElement* get() const { return nullptr; }
};
static_assert(!ds::DsElement<MockElementNoRelease>);

// A mock with wrong return type is rejected.
struct MockElementWrongReturn {
  int*  get()     { return nullptr; }
  int*  release() { return nullptr; }
};
static_assert(!ds::DsElement<MockElementWrongReturn>);

// ============================================================================
// Trivial runtime test so ctest registers this binary.
// ============================================================================

TEST(ConceptsTest, StaticAssertionsCompile) {
  // All compile-time checks above already ran at translation time.
  // Nothing to do at runtime.
  SUCCEED();
}
