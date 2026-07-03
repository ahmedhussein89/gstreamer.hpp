#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstcaps.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gststructure.h>
#include <nonstd/expected.hpp>

#include <core/core.hpp>

namespace gst {

// ============================================================================
// Non-owning typed handles — the enhanced layer
// ============================================================================
// Each handle is trivially copyable, exactly sizeof(T*), and implicitly
// converts to/from the raw pointer so every C API call works unchanged.

struct Element : Handle<GstElement> {
  using Handle::Handle;
};
struct Pipeline : Handle<GstElement> {
  using Handle::Handle;
  // GstPipeline IS a GstElement — allow passing a Pipeline wherever an Element is needed.
  operator Element() const noexcept { return Element{get()}; }  // NOLINT(google-explicit-constructor)
};
struct Bin : Handle<GstElement> {
  using Handle::Handle;
  operator Element() const noexcept { return Element{get()}; }  // NOLINT(google-explicit-constructor)
};
struct Bus : Handle<GstBus> {
  using Handle::Handle;
};
struct Pad : Handle<GstPad> {
  using Handle::Handle;
};
struct GhostPad : Handle<GstPad> {
  using Handle::Handle;
};
struct Caps : Handle<GstCaps> {
  using Handle::Handle;
};
struct Message : Handle<GstMessage> {
  using Handle::Handle;
};
struct Buffer : Handle<GstBuffer> {
  using Handle::Handle;
};
struct Structure : Handle<GstStructure> {
  using Handle::Handle;
};

static_assert(sizeof(Element) == sizeof(GstElement*));
static_assert(std::is_trivially_copyable_v<Element>);
static_assert(sizeof(Pipeline) == sizeof(GstElement*));
static_assert(std::is_trivially_copyable_v<Pipeline>);

// ============================================================================
// MessageType bitmask — enum bits + Flags<> layer
// ============================================================================

enum class MessageType : std::int32_t {
  Unknown           = GST_MESSAGE_UNKNOWN,
  EOS               = GST_MESSAGE_EOS,
  Error             = GST_MESSAGE_ERROR,
  Warning           = GST_MESSAGE_WARNING,
  Info              = GST_MESSAGE_INFO,
  Tag               = GST_MESSAGE_TAG,
  Buffering         = GST_MESSAGE_BUFFERING,
  StateChanged      = GST_MESSAGE_STATE_CHANGED,
  StateDirty        = GST_MESSAGE_STATE_DIRTY,
  StepDone          = GST_MESSAGE_STEP_DONE,
  ClockProvide      = GST_MESSAGE_CLOCK_PROVIDE,
  ClockLost         = GST_MESSAGE_CLOCK_LOST,
  NewClock          = GST_MESSAGE_NEW_CLOCK,
  StructureChange   = GST_MESSAGE_STRUCTURE_CHANGE,
  StreamStatus      = GST_MESSAGE_STREAM_STATUS,
  Application       = GST_MESSAGE_APPLICATION,
  ElementMsg        = GST_MESSAGE_ELEMENT,
  SegmentStart      = GST_MESSAGE_SEGMENT_START,
  SegmentDone       = GST_MESSAGE_SEGMENT_DONE,
  DurationChanged   = GST_MESSAGE_DURATION_CHANGED,
  Latency           = GST_MESSAGE_LATENCY,
  AsyncStart        = GST_MESSAGE_ASYNC_START,
  AsyncDone         = GST_MESSAGE_ASYNC_DONE,
  RequestState      = GST_MESSAGE_REQUEST_STATE,
  StepStart         = GST_MESSAGE_STEP_START,
  QoS               = GST_MESSAGE_QOS,
  Progress          = GST_MESSAGE_PROGRESS,
  Toc               = GST_MESSAGE_TOC,
  ResetTime         = GST_MESSAGE_RESET_TIME,
  StreamStart       = GST_MESSAGE_STREAM_START,
  NeedContext       = GST_MESSAGE_NEED_CONTEXT,
  HaveContext       = GST_MESSAGE_HAVE_CONTEXT,
  Extended          = GST_MESSAGE_EXTENDED,
  DeviceAdded       = GST_MESSAGE_DEVICE_ADDED,
  DeviceRemoved     = GST_MESSAGE_DEVICE_REMOVED,
  PropertyNotify    = GST_MESSAGE_PROPERTY_NOTIFY,
  StreamCollection  = GST_MESSAGE_STREAM_COLLECTION,
  StreamsSelected   = GST_MESSAGE_STREAMS_SELECTED,
  Redirect          = GST_MESSAGE_REDIRECT,
  DeviceChanged     = GST_MESSAGE_DEVICE_CHANGED,
  InstantRateRequest = GST_MESSAGE_INSTANT_RATE_REQUEST,
  Any               = GST_MESSAGE_ANY,
};

template <>
struct FlagTraits<MessageType> {
  using MaskType = std::int32_t;
  static constexpr MaskType allFlags = static_cast<MaskType>(GST_MESSAGE_ANY);
};
using MessageTypeFlags = Flags<MessageType>;

// ============================================================================
// RAII resource deleters + unique_ptr aliases
// ============================================================================
// These remain in the enhanced layer so free functions that must return owned
// refs (e.g. element_get_bus, element_get_static_pad) have a type to use.

struct GstElementDeleter final {
  void operator()(GstElement* e) const noexcept {
    if(e != nullptr) {
      gst_object_unref(e);
    }
  }
};
using ElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;

struct GstBusDeleter final {
  void operator()(GstBus* b) const noexcept {
    if(b != nullptr) {
      gst_object_unref(b);
    }
  }
};
using BusPtr = std::unique_ptr<GstBus, GstBusDeleter>;

struct GstErrorDeleter final {
  void operator()(GError* e) const noexcept {
    if(e != nullptr) {
      g_error_free(e);
    }
  }
};
using ErrorPtr = std::unique_ptr<GError, GstErrorDeleter>;

struct GstMessageDeleter final {
  void operator()(GstMessage* m) const noexcept {
    if(m != nullptr) {
      gst_message_unref(m);
    }
  }
};
using MessagePtr = std::unique_ptr<GstMessage, GstMessageDeleter>;

struct GstPadDeleter final {
  void operator()(GstPad* p) const noexcept {
    if(p != nullptr) {
      gst_object_unref(p);
    }
  }
};
using PadPtr = std::unique_ptr<GstPad, GstPadDeleter>;

struct GstCapsDeleter final {
  void operator()(GstCaps* c) const noexcept {
    if(c != nullptr) {
      gst_caps_unref(c);
    }
  }
};
using CapsPtr = std::unique_ptr<GstCaps, GstCapsDeleter>;

// ============================================================================
// POD helpers
// ============================================================================

struct StateChange {
  GstState old_state;
  GstState new_state;
  GstState pending;
};

// ============================================================================
// init
// ============================================================================

inline void init(std::span<char*> args) {
  static const bool kInit = [&args]() {
    int argc    = static_cast<int>(args.size());
    char** argv = args.data();
    gst_init(&argc, &argv);
    return true;
  }();
  (void)kInit;
}

// ============================================================================
// parse_launch
// ============================================================================

inline nonstd::expected<Element, ErrorPtr> parse_launch(std::string_view pipeline_description) {
  GError* error = nullptr;
  std::string pipeline_str(pipeline_description);
  GstElement* element = gst_parse_launch(pipeline_str.c_str(), &error);
  if(error != nullptr) {
    return nonstd::make_unexpected(ErrorPtr(error));
  }
  return Element{element};
}

// ============================================================================
// pipeline_new / element_factory_make
// ============================================================================

inline nonstd::expected<Pipeline, std::string> pipeline_new(std::string_view name = {}) {
  GstElement* p = gst_pipeline_new(name.empty() ? nullptr : std::string{name}.c_str());
  if(p == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create pipeline"));
  }
  return Pipeline{p};
}

inline nonstd::expected<Element, std::string> element_factory_make(
    std::string_view factory, std::string_view name = {}) {
  GstElement* elem = gst_element_factory_make(
      std::string{factory}.c_str(), name.empty() ? nullptr : std::string{name}.c_str());
  if(elem == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create element '{}'", factory));
  }
  return Element{elem};
}

// ============================================================================
// bin_add
// ============================================================================
// Adds an element to a pipeline bin. The bin sinks the element's floating
// reference. Returns the same handle for use in subsequent linking calls.
// After this call the pipeline bin owns the GstElement* lifetime.

inline nonstd::expected<Element, std::string> bin_add(Pipeline pipeline, Element element) {
  if(gst_bin_add(GST_BIN(pipeline.get()), element.get()) != TRUE) {
    return nonstd::make_unexpected(std::string("Failed to add element to pipeline"));
  }
  return element;
}

// ============================================================================
// element_link
// ============================================================================

inline nonstd::expected<void, std::string> element_link(Element src, Element sink) {
  if(gst_element_link(src.get(), sink.get()) != TRUE) {
    return nonstd::make_unexpected(std::string("Failed to link elements"));
  }
  return {};
}

// ============================================================================
// element_get_bus
// ============================================================================
// Returns an owned bus ref — the caller (or BusPtr destructor) unrefs it.

inline nonstd::expected<BusPtr, std::string> element_get_bus(Element element) {
  GstBus* bus = gst_element_get_bus(element.get());
  if(bus == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to get bus from element"));
  }
  return BusPtr{bus};
}

// ============================================================================
// element_set_state
// ============================================================================

inline nonstd::expected<void, std::string> element_set_state(Element element, GstState state) {
  if(!element) {
    return nonstd::make_unexpected(std::string("Element is null"));
  }
  if(gst_element_set_state(element.get(), state) == GST_STATE_CHANGE_FAILURE) {
    return nonstd::make_unexpected(std::string("Failed to change element state"));
  }
  return {};
}

// ============================================================================
// element_get_static_pad
// ============================================================================
// Returns an owned pad ref — the caller (or PadPtr destructor) unrefs it.

inline nonstd::expected<PadPtr, std::string> element_get_static_pad(Element element, std::string_view name) {
  std::string name_str(name);
  GstPad* pad = gst_element_get_static_pad(element.get(), name_str.c_str());
  if(pad == nullptr) {
    return nonstd::make_unexpected(std::string("No static pad '") + name_str + "' on element");
  }
  return PadPtr(pad);
}

// ============================================================================
// pad_is_linked
// ============================================================================

inline bool pad_is_linked(Pad pad) noexcept {
  return gst_pad_is_linked(pad.get()) == TRUE;
}

inline bool pad_is_linked(const PadPtr& pad) noexcept {
  return gst_pad_is_linked(pad.get()) == TRUE;
}

// ============================================================================
// pad_link
// ============================================================================

inline nonstd::expected<void, std::string> pad_link(Pad src, Pad sink) {
  const GstPadLinkReturn ret = gst_pad_link(src.get(), sink.get());
  if(GST_PAD_LINK_FAILED(ret)) {
    return nonstd::make_unexpected(fmt::format("Failed to link pads (code {})", static_cast<int>(ret)));
  }
  return {};
}

inline nonstd::expected<void, std::string> pad_link(GstPad* src, const PadPtr& sink) {
  const GstPadLinkReturn ret = gst_pad_link(src, sink.get());
  if(GST_PAD_LINK_FAILED(ret)) {
    return nonstd::make_unexpected(fmt::format("Failed to link pads (code {})", static_cast<int>(ret)));
  }
  return {};
}

// ============================================================================
// pad_get_current_caps
// ============================================================================

inline nonstd::expected<CapsPtr, std::string> pad_get_current_caps(Pad pad) {
  GstCaps* caps = gst_pad_get_current_caps(pad.get());
  if(caps == nullptr) {
    return nonstd::make_unexpected(std::string("Pad has no current caps"));
  }
  return CapsPtr{caps};
}

inline nonstd::expected<CapsPtr, std::string> pad_get_current_caps(GstPad* pad) {
  GstCaps* caps = gst_pad_get_current_caps(pad);
  if(caps == nullptr) {
    return nonstd::make_unexpected(std::string("Pad has no current caps"));
  }
  return CapsPtr{caps};
}

// ============================================================================
// caps_from_string / caps_get_structure / structure_get_name
// ============================================================================

inline nonstd::expected<CapsPtr, std::string> caps_from_string(std::string_view description) {
  std::string desc_str(description);
  GstCaps* caps = gst_caps_from_string(desc_str.c_str());
  if(caps == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to parse caps: ") + desc_str);
  }
  return CapsPtr(caps);
}

inline nonstd::expected<const GstStructure*, std::string> caps_get_structure(
    const CapsPtr& caps, guint index = 0) {
  const GstStructure* structure = gst_caps_get_structure(caps.get(), index);
  if(structure == nullptr) {
    return nonstd::make_unexpected(fmt::format("No structure at index {} in caps", index));
  }
  return structure;
}

inline std::string_view structure_get_name(const GstStructure* structure) {
  return std::string_view{gst_structure_get_name(structure)};
}

// ============================================================================
// message_type / message_parse_error / message_parse_state_changed
// ============================================================================

inline MessageType message_type(Message msg) noexcept {
  return static_cast<MessageType>(GST_MESSAGE_TYPE(msg.get()));
}

inline MessageType message_type(const MessagePtr& msg) noexcept {
  return static_cast<MessageType>(GST_MESSAGE_TYPE(msg.get()));
}

inline nonstd::expected<std::pair<std::string, std::string>, std::string>
message_parse_error(Message msg) {
  GError* error     = nullptr;
  gchar* debug_info = nullptr;
  gst_message_parse_error(msg.get(), &error, &debug_info);
  if(error == nullptr) {
    return nonstd::make_unexpected(std::string("No error found in message"));
  }
  auto result = std::make_pair(std::string{error->message}, std::string{GST_STR_NULL(debug_info)});
  g_error_free(error);
  g_free(debug_info);
  return result;
}

inline nonstd::expected<std::pair<std::string, std::string>, std::string>
message_parse_error(GstMessage* message) {
  GError* error     = nullptr;
  gchar* debug_info = nullptr;
  gst_message_parse_error(message, &error, &debug_info);
  if(error == nullptr) {
    return nonstd::make_unexpected(std::string("No error found in message"));
  }
  auto result = std::make_pair(std::string{error->message}, std::string{GST_STR_NULL(debug_info)});
  g_error_free(error);
  g_free(debug_info);
  return result;
}

inline StateChange message_parse_state_changed(Message msg) noexcept {
  StateChange result{};
  gst_message_parse_state_changed(msg.get(), &result.old_state, &result.new_state, &result.pending);
  return result;
}

inline StateChange message_parse_state_changed(const MessagePtr& msg) noexcept {
  StateChange result{};
  gst_message_parse_state_changed(msg.get(), &result.old_state, &result.new_state, &result.pending);
  return result;
}

// ============================================================================
// state_get_name
// ============================================================================

inline std::string_view state_get_name(GstState state) noexcept {
  return std::string_view{gst_element_state_get_name(state)};
}

// ============================================================================
// bus_timed_pop_filtered
// ============================================================================

inline nonstd::expected<MessagePtr, std::string>
bus_timed_pop_filtered(Bus bus, GstClockTime timeout, MessageTypeFlags types) {
  GstMessage* msg = gst_bus_timed_pop_filtered(
      bus.get(), timeout,
      static_cast<GstMessageType>(types.value()));
  if(msg == nullptr) {
    return nonstd::make_unexpected(std::string("No message received from bus"));
  }
  return MessagePtr(msg);
}

inline nonstd::expected<MessagePtr, std::string>
bus_timed_pop_filtered(const BusPtr& bus, GstClockTime timeout, MessageTypeFlags types) {
  GstMessage* msg = gst_bus_timed_pop_filtered(
      bus.get(), timeout,
      static_cast<GstMessageType>(types.value()));
  if(msg == nullptr) {
    return nonstd::make_unexpected(std::string("No message received from bus"));
  }
  return MessagePtr(msg);
}

}  // namespace gst
