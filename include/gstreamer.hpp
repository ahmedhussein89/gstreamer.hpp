#pragma once
#include <concepts>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <functional>

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstcaps.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gststructure.h>
#include <gst/gstclock.h>
#include <gst/gstevent.h>
#include <gst/gstelementfactory.h>
#include <gst/gstpipeline.h>
#include <gst/gstsystemclock.h>

#include <core/core.hpp>
#include <nonstd/expected.hpp>

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
  operator Element() const noexcept {
    return Element{get()};
  }    // NOLINT(google-explicit-constructor)
};
struct Bin : Handle<GstElement> {
  using Handle::Handle;
  operator Element() const noexcept {
    return Element{get()};
  }    // NOLINT(google-explicit-constructor)
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
struct Clock : Handle<GstClock> {
  using Handle::Handle;
};
struct Event : Handle<GstEvent> {
  using Handle::Handle;
};
struct ElementFactory : Handle<GstElementFactory> {
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
  Unknown = GST_MESSAGE_UNKNOWN,
  EOS = GST_MESSAGE_EOS,
  Error = GST_MESSAGE_ERROR,
  Warning = GST_MESSAGE_WARNING,
  Info = GST_MESSAGE_INFO,
  Tag = GST_MESSAGE_TAG,
  Buffering = GST_MESSAGE_BUFFERING,
  StateChanged = GST_MESSAGE_STATE_CHANGED,
  StateDirty = GST_MESSAGE_STATE_DIRTY,
  StepDone = GST_MESSAGE_STEP_DONE,
  ClockProvide = GST_MESSAGE_CLOCK_PROVIDE,
  ClockLost = GST_MESSAGE_CLOCK_LOST,
  NewClock = GST_MESSAGE_NEW_CLOCK,
  StructureChange = GST_MESSAGE_STRUCTURE_CHANGE,
  StreamStatus = GST_MESSAGE_STREAM_STATUS,
  Application = GST_MESSAGE_APPLICATION,
  ElementMsg = GST_MESSAGE_ELEMENT,
  SegmentStart = GST_MESSAGE_SEGMENT_START,
  SegmentDone = GST_MESSAGE_SEGMENT_DONE,
  DurationChanged = GST_MESSAGE_DURATION_CHANGED,
  Latency = GST_MESSAGE_LATENCY,
  AsyncStart = GST_MESSAGE_ASYNC_START,
  AsyncDone = GST_MESSAGE_ASYNC_DONE,
  RequestState = GST_MESSAGE_REQUEST_STATE,
  StepStart = GST_MESSAGE_STEP_START,
  QoS = GST_MESSAGE_QOS,
  Progress = GST_MESSAGE_PROGRESS,
  Toc = GST_MESSAGE_TOC,
  ResetTime = GST_MESSAGE_RESET_TIME,
  StreamStart = GST_MESSAGE_STREAM_START,
  NeedContext = GST_MESSAGE_NEED_CONTEXT,
  HaveContext = GST_MESSAGE_HAVE_CONTEXT,
  Extended = GST_MESSAGE_EXTENDED,
  DeviceAdded = GST_MESSAGE_DEVICE_ADDED,
  DeviceRemoved = GST_MESSAGE_DEVICE_REMOVED,
  PropertyNotify = GST_MESSAGE_PROPERTY_NOTIFY,
  StreamCollection = GST_MESSAGE_STREAM_COLLECTION,
  StreamsSelected = GST_MESSAGE_STREAMS_SELECTED,
  Redirect = GST_MESSAGE_REDIRECT,
  DeviceChanged = GST_MESSAGE_DEVICE_CHANGED,
  InstantRateRequest = GST_MESSAGE_INSTANT_RATE_REQUEST,
  Any = GST_MESSAGE_ANY,
};

template <>
struct FlagTraits<MessageType> {
  using MaskType = std::int32_t;
  static constexpr MaskType allFlags = static_cast<MaskType>(GST_MESSAGE_ANY);
};
using MessageTypeFlags = Flags<MessageType>;

enum class SeekFlags : std::int32_t {
  None        = GST_SEEK_FLAG_NONE,
  Flush       = GST_SEEK_FLAG_FLUSH,
  Accurate    = GST_SEEK_FLAG_ACCURATE,
  KeyUnit     = GST_SEEK_FLAG_KEY_UNIT,
  Segment     = GST_SEEK_FLAG_SEGMENT,
  Trickmode   = GST_SEEK_FLAG_TRICKMODE,
  SnapBefore  = GST_SEEK_FLAG_SNAP_BEFORE,
  SnapAfter   = GST_SEEK_FLAG_SNAP_AFTER,
  SnapNearest = GST_SEEK_FLAG_SNAP_NEAREST,
};
template <>
struct FlagTraits<SeekFlags> {
  using MaskType = std::int32_t;
  static constexpr MaskType allFlags =
      static_cast<MaskType>(GST_SEEK_FLAG_SNAP_NEAREST) | static_cast<MaskType>(GST_SEEK_FLAG_SNAP_AFTER) |
      static_cast<MaskType>(GST_SEEK_FLAG_SNAP_BEFORE) | static_cast<MaskType>(GST_SEEK_FLAG_TRICKMODE) |
      static_cast<MaskType>(GST_SEEK_FLAG_SEGMENT) | static_cast<MaskType>(GST_SEEK_FLAG_KEY_UNIT) |
      static_cast<MaskType>(GST_SEEK_FLAG_ACCURATE) | static_cast<MaskType>(GST_SEEK_FLAG_FLUSH);
};
using SeekFlagsFlags = Flags<SeekFlags>;

enum class MapFlags : std::uint32_t {
  Read      = GST_MAP_READ,
  Write     = GST_MAP_WRITE,
  ReadWrite = static_cast<std::uint32_t>(GST_MAP_READ) | static_cast<std::uint32_t>(GST_MAP_WRITE),
};

enum class PadProbeReturn : std::int32_t {
  Drop    = GST_PAD_PROBE_DROP,
  Ok      = GST_PAD_PROBE_OK,
  Remove  = GST_PAD_PROBE_REMOVE,
  Pass    = GST_PAD_PROBE_PASS,
  Handled = GST_PAD_PROBE_HANDLED,
};

enum class PadProbeType : std::uint32_t {
  Invalid           = GST_PAD_PROBE_TYPE_INVALID,
  Idle              = GST_PAD_PROBE_TYPE_IDLE,
  Block             = GST_PAD_PROBE_TYPE_BLOCK,
  Buffer            = GST_PAD_PROBE_TYPE_BUFFER,
  BufferList        = GST_PAD_PROBE_TYPE_BUFFER_LIST,
  EventDownstream   = GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
  EventUpstream     = GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
  EventFlush        = GST_PAD_PROBE_TYPE_EVENT_FLUSH,
  QueryDownstream   = GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
  QueryUpstream     = GST_PAD_PROBE_TYPE_QUERY_UPSTREAM,
  Push              = GST_PAD_PROBE_TYPE_PUSH,
  Pull              = GST_PAD_PROBE_TYPE_PULL,
  Blocking          = GST_PAD_PROBE_TYPE_BLOCKING,
  DataDownstream    = GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
  DataUpstream      = GST_PAD_PROBE_TYPE_DATA_UPSTREAM,
  DataBoth          = GST_PAD_PROBE_TYPE_DATA_BOTH,
  BlockDownstream   = GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
  BlockUpstream     = GST_PAD_PROBE_TYPE_BLOCK_UPSTREAM,
  EventBoth         = GST_PAD_PROBE_TYPE_EVENT_BOTH,
  QueryBoth         = GST_PAD_PROBE_TYPE_QUERY_BOTH,
  AllBoth           = GST_PAD_PROBE_TYPE_ALL_BOTH,
  Scheduling        = GST_PAD_PROBE_TYPE_SCHEDULING,
};
template <>
struct FlagTraits<PadProbeType> {
  using MaskType = std::uint32_t;
  static constexpr MaskType allFlags = static_cast<MaskType>(GST_PAD_PROBE_TYPE_SCHEDULING);
};
using PadProbeTypeFlags = Flags<PadProbeType>;

// ============================================================================
// RAII resource deleters + unique_ptr aliases
// ============================================================================
// These remain in the enhanced layer so free functions that must return owned
// refs (e.g. element_get_bus, element_get_static_pad) have a type to use.

struct GstElementDeleter final {
  void operator()(GstElement* elem) const noexcept {
    if(elem != nullptr) {
      gst_object_unref(elem);
    }
  }
};
using ElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;

struct GstBusDeleter final {
  void operator()(GstBus* bus) const noexcept {
    if(bus != nullptr) {
      gst_object_unref(bus);
    }
  }
};
using BusPtr = std::unique_ptr<GstBus, GstBusDeleter>;

struct GstErrorDeleter final {
  void operator()(GError* err) const noexcept {
    if(err != nullptr) {
      g_error_free(err);
    }
  }
};
using ErrorPtr = std::unique_ptr<GError, GstErrorDeleter>;

struct GstMessageDeleter final {
  void operator()(GstMessage* msg) const noexcept {
    if(msg != nullptr) {
      gst_message_unref(msg);
    }
  }
};
using MessagePtr = std::unique_ptr<GstMessage, GstMessageDeleter>;

struct GstPadDeleter final {
  void operator()(GstPad* pad) const noexcept {
    if(pad != nullptr) {
      gst_object_unref(pad);
    }
  }
};
using PadPtr = std::unique_ptr<GstPad, GstPadDeleter>;

struct GstCapsDeleter final {
  void operator()(GstCaps* caps) const noexcept {
    if(caps != nullptr) {
      gst_caps_unref(caps);
    }
  }
};
using CapsPtr = std::unique_ptr<GstCaps, GstCapsDeleter>;

struct GstClockDeleter final {
  void operator()(GstClock* clock) const noexcept {
    if(clock != nullptr) {
      gst_object_unref(clock);
    }
  }
};
using ClockPtr = std::unique_ptr<GstClock, GstClockDeleter>;

struct GstEventDeleter final {
  void operator()(GstEvent* event) const noexcept {
    if(event != nullptr) {
      gst_event_unref(event);
    }
  }
};
using EventPtr = std::unique_ptr<GstEvent, GstEventDeleter>;

struct GstElementFactoryDeleter final {
  void operator()(GstElementFactory* factory) const noexcept {
    if(factory != nullptr) {
      gst_object_unref(factory);
    }
  }
};
using ElementFactoryPtr = std::unique_ptr<GstElementFactory, GstElementFactoryDeleter>;

// ============================================================================
// POD helpers
// ============================================================================

struct StateChange {
  GstState old_state;
  GstState new_state;
  GstState pending;
};

// ============================================================================
// Time constants
// ============================================================================

inline constexpr GstClockTime kSecond        = GST_SECOND;
inline constexpr GstClockTime kMsecond       = GST_MSECOND;
inline constexpr GstClockTime kClockTimeNone = GST_CLOCK_TIME_NONE;

// ============================================================================
// init
// ============================================================================

inline void init(std::span<char*> args) {
  if(gst_is_initialized()) {
    return;
  }
  int argc = static_cast<int>(args.size());
  char** argv = args.data();
  gst_init(&argc, &argv);
}

// ============================================================================
// deinit
// ============================================================================

inline void deinit() {
  if(gst_is_initialized()) {
    gst_deinit();
  }
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

inline nonstd::expected<Element, std::string> element_factory_make(std::string_view factory, std::string_view name = {}) {
  GstElement* elem = gst_element_factory_make(std::string{factory}.c_str(), name.empty() ? nullptr : std::string{name}.c_str());
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

inline nonstd::expected<const GstStructure*, std::string> caps_get_structure(const CapsPtr& caps, guint index = 0) {
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

inline nonstd::expected<std::pair<std::string, std::string>, std::string> message_parse_error(Message msg) {
  GError* error = nullptr;
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

inline nonstd::expected<std::pair<std::string, std::string>, std::string> message_parse_error(GstMessage* message) {
  GError* error = nullptr;
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

inline nonstd::expected<MessagePtr, std::string> bus_timed_pop_filtered(Bus bus, GstClockTime timeout, MessageTypeFlags types) {
  GstMessage* msg = gst_bus_timed_pop_filtered(bus.get(), timeout, static_cast<GstMessageType>(types.value()));
  if(msg == nullptr) {
    return nonstd::make_unexpected(std::string("No message received from bus"));
  }
  return MessagePtr(msg);
}

inline nonstd::expected<MessagePtr, std::string> bus_timed_pop_filtered(const BusPtr& bus,
                                                                        GstClockTime timeout,
                                                                        MessageTypeFlags types) {
  GstMessage* msg = gst_bus_timed_pop_filtered(bus.get(), timeout, static_cast<GstMessageType>(types.value()));
  if(msg == nullptr) {
    return nonstd::make_unexpected(std::string("No message received from bus"));
  }
  return MessagePtr(msg);
}


// ============================================================================
// element_get_state
// ============================================================================

struct ElementState {
  GstState state;
  GstState pending;
};

inline nonstd::expected<ElementState, std::string> element_get_state(
    Element element, GstClockTime timeout = kClockTimeNone) {
  GstState state   = GST_STATE_NULL;
  GstState pending = GST_STATE_NULL;
  const GstStateChangeReturn ret =
      gst_element_get_state(element.get(), &state, &pending, timeout);
  if(ret == GST_STATE_CHANGE_FAILURE) {
    return nonstd::make_unexpected(std::string("Failed to get element state"));
  }
  return ElementState{state, pending};
}

// ============================================================================
// element_query_position / element_query_duration
// ============================================================================

inline nonstd::expected<gint64, std::string> element_query_position(Element element, GstFormat format) {
  gint64 pos = 0;
  if(!gst_element_query_position(element.get(), format, &pos)) {
    return nonstd::make_unexpected(std::string("Failed to query position"));
  }
  return pos;
}

inline nonstd::expected<gint64, std::string> element_query_duration(Element element, GstFormat format) {
  gint64 dur = 0;
  if(!gst_element_query_duration(element.get(), format, &dur)) {
    return nonstd::make_unexpected(std::string("Failed to query duration"));
  }
  return dur;
}

// ============================================================================
// element_seek_simple
// ============================================================================

inline nonstd::expected<void, std::string> element_seek_simple(
    Element element, GstFormat format, SeekFlagsFlags flags, gint64 seek_pos) {
  if(!gst_element_seek_simple(element.get(), format,
         static_cast<GstSeekFlags>(flags.value()), seek_pos)) {
    return nonstd::make_unexpected(std::string("Seek failed"));
  }
  return {};
}

// ============================================================================
// element_sync_state_with_parent
// ============================================================================

inline nonstd::expected<void, std::string> element_sync_state_with_parent(Element element) {
  if(!gst_element_sync_state_with_parent(element.get())) {
    return nonstd::make_unexpected(std::string("Failed to sync element state with parent"));
  }
  return {};
}

// ============================================================================
// element_set_base_time
// ============================================================================

inline void element_set_base_time(Element element, GstClockTime base_time) noexcept {
  gst_element_set_base_time(element.get(), base_time);
}

// ============================================================================
// element_post_message / element_send_event
// ============================================================================

inline nonstd::expected<void, std::string> element_post_message(Element element, GstMessage* message) {
  if(!gst_element_post_message(element.get(), message)) {
    return nonstd::make_unexpected(std::string("Failed to post message"));
  }
  return {};
}

inline nonstd::expected<void, std::string> element_send_event(Element element, EventPtr event_ptr) {
  if(!gst_element_send_event(element.get(), event_ptr.release())) {
    return nonstd::make_unexpected(std::string("Failed to send event"));
  }
  return {};
}

// ============================================================================
// element_release_request_pad / element_request_pad_simple
// ============================================================================

inline void element_release_request_pad(Element element, Pad pad) noexcept {
  gst_element_release_request_pad(element.get(), pad.get());
}

inline nonstd::expected<PadPtr, std::string> element_request_pad_simple(
    Element element, std::string_view pad_name) {
  std::string name_str(pad_name);
  GstPad* pad = gst_element_request_pad_simple(element.get(), name_str.c_str());
  if(pad == nullptr) {
    return nonstd::make_unexpected(
        fmt::format("Failed to request pad '{}' from element", pad_name));
  }
  return PadPtr{pad};
}

// ============================================================================
// element_factory_find
// ============================================================================

inline nonstd::expected<ElementFactoryPtr, std::string> element_factory_find(
    std::string_view factory_name) {
  std::string name_str(factory_name);
  GstElementFactory* factory = gst_element_factory_find(name_str.c_str());
  if(factory == nullptr) {
    return nonstd::make_unexpected(
        fmt::format("Element factory '{}' not found", factory_name));
  }
  return ElementFactoryPtr{factory};
}

// ============================================================================
// bin_add_many / bin_remove / bin_get_by_name
// ============================================================================

inline nonstd::expected<void, std::string> bin_add_many(
    Pipeline pipeline, std::initializer_list<Element> elements) {
  for(Element elem : elements) {
    if(!gst_bin_add(GST_BIN(pipeline.get()), elem.get())) {
      return nonstd::make_unexpected(std::string("Failed to add element to pipeline"));
    }
  }
  return {};
}

inline nonstd::expected<void, std::string> bin_remove(Pipeline pipeline, Element element) {
  if(!gst_bin_remove(GST_BIN(pipeline.get()), element.get())) {
    return nonstd::make_unexpected(std::string("Failed to remove element from pipeline"));
  }
  return {};
}

inline nonstd::expected<ElementPtr, std::string> bin_get_by_name(
    Pipeline pipeline, std::string_view element_name) {
  std::string name_str(element_name);
  GstElement* elem = gst_bin_get_by_name(GST_BIN(pipeline.get()), name_str.c_str());
  if(elem == nullptr) {
    return nonstd::make_unexpected(
        fmt::format("No element named '{}' in pipeline", element_name));
  }
  return ElementPtr{elem};
}

// ============================================================================
// pad_unlink
// ============================================================================

inline nonstd::expected<void, std::string> pad_unlink(Pad src, Pad sink) {
  if(!gst_pad_unlink(src.get(), sink.get())) {
    return nonstd::make_unexpected(std::string("Failed to unlink pads"));
  }
  return {};
}

// ============================================================================
// caps_to_string
// ============================================================================

inline std::string caps_to_string(const CapsPtr& caps) {
  gchar* str = gst_caps_to_string(caps.get());
  std::string result{str};
  g_free(str);
  return result;
}

inline std::string caps_to_string(Caps caps) {
  gchar* str = gst_caps_to_string(caps.get());
  std::string result{str};
  g_free(str);
  return result;
}

// ============================================================================
// structure_get_string
// ============================================================================

inline std::string_view structure_get_string(const GstStructure* structure,
                                              std::string_view field_name) {
  std::string field_str(field_name);
  const gchar* val = gst_structure_get_string(structure, field_str.c_str());
  return val != nullptr ? std::string_view{val} : std::string_view{};
}

// ============================================================================
// pipeline_get_clock / clock_get_time / system_clock_obtain
// ============================================================================

inline nonstd::expected<ClockPtr, std::string> pipeline_get_clock(Pipeline pipeline) {
  GstClock* clk = gst_pipeline_get_clock(GST_PIPELINE(pipeline.get()));
  if(clk == nullptr) {
    return nonstd::make_unexpected(std::string("Pipeline has no clock"));
  }
  return ClockPtr{clk};
}

inline GstClockTime clock_get_time(Clock clock) noexcept {
  return gst_clock_get_time(clock.get());
}

inline GstClockTime clock_get_time(const ClockPtr& clock) noexcept {
  return gst_clock_get_time(clock.get());
}

inline ClockPtr system_clock_obtain() noexcept {
  return ClockPtr{gst_system_clock_obtain()};
}

// ============================================================================
// bus_add_watch
// ============================================================================
// ponytail: heap-allocates std::function; freed via GDestroyNotify when watch removed.

inline guint bus_add_watch(Bus bus, std::function<bool(Message)> callback) {
  auto* cb_ptr = new std::function<bool(Message)>(std::move(callback));
  return gst_bus_add_watch_full(
      bus.get(), G_PRIORITY_DEFAULT,
      [](GstBus* /*bus*/, GstMessage* msg, gpointer data) -> gboolean {
        return (*static_cast<std::function<bool(Message)>*>(data))(Message{msg}) ? TRUE : FALSE;
      },
      cb_ptr,
      [](gpointer data) { delete static_cast<std::function<bool(Message)>*>(data); });
}

inline guint bus_add_watch(const BusPtr& bus, std::function<bool(Message)> callback) {
  return bus_add_watch(Bus{bus.get()}, std::move(callback));
}

// ============================================================================
// event_new_eos
// ============================================================================

inline EventPtr event_new_eos() noexcept {
  return EventPtr{gst_event_new_eos()};
}

// ============================================================================
// Pipeline DSL — descriptor types (no GStreamer resources; build() is in
// gstreamer_raii.hpp since it creates and returns owned objects)
// ============================================================================

using PropertyValue = std::variant<bool, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, double, std::string>;

template <typename T>
concept PropertyValueType = std::constructible_from<PropertyValue, T>;

struct Node {
  std::string factory;
  std::string name;
  std::vector<std::pair<std::string, PropertyValue>> properties;

  explicit Node(std::string factory_, std::string name_ = {}) : factory{std::move(factory_)}, name{std::move(name_)} {}

  template <PropertyValueType T>
  [[nodiscard]] Node prop(std::string key, T value) && {
    properties.emplace_back(std::move(key), PropertyValue{std::move(value)});
    return std::move(*this);
  }

  [[nodiscard]] Node prop(std::string key, const char* value) && {
    properties.emplace_back(std::move(key), PropertyValue{std::string{value}});
    return std::move(*this);
  }
};

template <typename T>
concept PipelineNodeType = std::same_as<std::remove_cvref_t<T>, Node>;

struct PipelineDesc {
  std::vector<Node> elements;

  template <typename... Nodes>
    requires(PipelineNodeType<Nodes> && ...)
  explicit PipelineDesc(Nodes&&... nodes) : elements{std::forward<Nodes>(nodes)...} {}
};

}    // namespace gst
