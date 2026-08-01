#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstcaps.h>
#include <gst/gstclock.h>
#include <gst/gstelementfactory.h>
#include <gst/gstevent.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gstpipeline.h>
#include <gst/gstplugin.h>
#include <gst/gstpluginfeature.h>
#include <gst/gstquery.h>
#include <gst/gstregistry.h>
#include <gst/gstsample.h>
#include <gst/gststructure.h>
#include <gst/gstsystemclock.h>
#include <gst/gsttaglist.h>
#include <gst/net/gstnetclientclock.h>
#include <gst/net/gstnettimeprovider.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/video/navigation.h>

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
struct Sample : Handle<GstSample> {
  using Handle::Handle;
};
struct TagList : Handle<GstTagList> {
  using Handle::Handle;
};
struct Query : Handle<GstQuery> {
  using Handle::Handle;
};
struct Plugin : Handle<GstPlugin> {
  using Handle::Handle;
};
struct PluginFeature : Handle<GstPluginFeature> {
  using Handle::Handle;
};
struct Registry : Handle<GstRegistry> {
  using Handle::Handle;
};
struct EncodingContainerProfile : Handle<GstEncodingContainerProfile> {
  using Handle::Handle;
};
struct EncodingVideoProfile : Handle<GstEncodingVideoProfile> {
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
  None = GST_SEEK_FLAG_NONE,
  Flush = GST_SEEK_FLAG_FLUSH,
  Accurate = GST_SEEK_FLAG_ACCURATE,
  KeyUnit = GST_SEEK_FLAG_KEY_UNIT,
  Segment = GST_SEEK_FLAG_SEGMENT,
  Trickmode = GST_SEEK_FLAG_TRICKMODE,
  SnapBefore = GST_SEEK_FLAG_SNAP_BEFORE,
  SnapAfter = GST_SEEK_FLAG_SNAP_AFTER,
  SnapNearest = GST_SEEK_FLAG_SNAP_NEAREST,
};
template <>
struct FlagTraits<SeekFlags> {
  using MaskType = std::int32_t;
  static constexpr MaskType allFlags = static_cast<MaskType>(GST_SEEK_FLAG_SNAP_NEAREST) |
      static_cast<MaskType>(GST_SEEK_FLAG_SNAP_AFTER) | static_cast<MaskType>(GST_SEEK_FLAG_SNAP_BEFORE) |
      static_cast<MaskType>(GST_SEEK_FLAG_TRICKMODE) | static_cast<MaskType>(GST_SEEK_FLAG_SEGMENT) |
      static_cast<MaskType>(GST_SEEK_FLAG_KEY_UNIT) | static_cast<MaskType>(GST_SEEK_FLAG_ACCURATE) |
      static_cast<MaskType>(GST_SEEK_FLAG_FLUSH);
};
using SeekFlagsFlags = Flags<SeekFlags>;

enum class MapFlags : std::uint32_t {
  Read = GST_MAP_READ,
  Write = GST_MAP_WRITE,
  ReadWrite = static_cast<std::uint32_t>(GST_MAP_READ) | static_cast<std::uint32_t>(GST_MAP_WRITE),
};

enum class PadProbeReturn : std::int32_t {
  Drop = GST_PAD_PROBE_DROP,
  Ok = GST_PAD_PROBE_OK,
  Remove = GST_PAD_PROBE_REMOVE,
  Pass = GST_PAD_PROBE_PASS,
  Handled = GST_PAD_PROBE_HANDLED,
};

enum class PadProbeType : std::uint32_t {
  Invalid = GST_PAD_PROBE_TYPE_INVALID,
  Idle = GST_PAD_PROBE_TYPE_IDLE,
  Block = GST_PAD_PROBE_TYPE_BLOCK,
  Buffer = GST_PAD_PROBE_TYPE_BUFFER,
  BufferList = GST_PAD_PROBE_TYPE_BUFFER_LIST,
  EventDownstream = GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
  EventUpstream = GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
  EventFlush = GST_PAD_PROBE_TYPE_EVENT_FLUSH,
  QueryDownstream = GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
  QueryUpstream = GST_PAD_PROBE_TYPE_QUERY_UPSTREAM,
  Push = GST_PAD_PROBE_TYPE_PUSH,
  Pull = GST_PAD_PROBE_TYPE_PULL,
  Blocking = GST_PAD_PROBE_TYPE_BLOCKING,
  DataDownstream = GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
  DataUpstream = GST_PAD_PROBE_TYPE_DATA_UPSTREAM,
  DataBoth = GST_PAD_PROBE_TYPE_DATA_BOTH,
  BlockDownstream = GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
  BlockUpstream = GST_PAD_PROBE_TYPE_BLOCK_UPSTREAM,
  EventBoth = GST_PAD_PROBE_TYPE_EVENT_BOTH,
  QueryBoth = GST_PAD_PROBE_TYPE_QUERY_BOTH,
  AllBoth = GST_PAD_PROBE_TYPE_ALL_BOTH,
  Scheduling = GST_PAD_PROBE_TYPE_SCHEDULING,
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

struct GstBufferDeleter final {
  void operator()(GstBuffer* buf) const noexcept {
    if(buf != nullptr) {
      gst_buffer_unref(buf);
    }
  }
};
using BufferPtr = std::unique_ptr<GstBuffer, GstBufferDeleter>;

struct GstSampleDeleter final {
  void operator()(GstSample* sample) const noexcept {
    if(sample != nullptr) {
      gst_sample_unref(sample);
    }
  }
};
using SamplePtr = std::unique_ptr<GstSample, GstSampleDeleter>;

struct GstStructureDeleter final {
  void operator()(GstStructure* structure) const noexcept {
    if(structure != nullptr) {
      gst_structure_free(structure);
    }
  }
};
using StructurePtr = std::unique_ptr<GstStructure, GstStructureDeleter>;

struct GstTagListDeleter final {
  void operator()(GstTagList* tags) const noexcept {
    if(tags != nullptr) {
      gst_tag_list_unref(tags);
    }
  }
};
using TagListPtr = std::unique_ptr<GstTagList, GstTagListDeleter>;

struct GstQueryDeleter final {
  void operator()(GstQuery* query) const noexcept {
    if(query != nullptr) {
      gst_query_unref(query);
    }
  }
};
using QueryPtr = std::unique_ptr<GstQuery, GstQueryDeleter>;

struct GstPluginDeleter final {
  void operator()(GstPlugin* plugin) const noexcept {
    if(plugin != nullptr) {
      gst_object_unref(plugin);
    }
  }
};
using PluginPtr = std::unique_ptr<GstPlugin, GstPluginDeleter>;

struct GstPluginFeatureDeleter final {
  void operator()(GstPluginFeature* feature) const noexcept {
    if(feature != nullptr) {
      gst_object_unref(feature);
    }
  }
};
using PluginFeaturePtr = std::unique_ptr<GstPluginFeature, GstPluginFeatureDeleter>;

// Shared GstEncodingProfile base: both concrete profile deleters call
// gst_encoding_profile_unref (a g_object_unref wrapper) via a cast, per the
// GstEncodingContainerProfile/GstEncodingVideoProfile hierarchy in
// gst/pbutils/encoding-profile.h.
struct GstEncodingContainerProfileDeleter final {
  void operator()(GstEncodingContainerProfile* profile) const noexcept {
    if(profile != nullptr) {
      gst_encoding_profile_unref(
          reinterpret_cast<GstEncodingProfile*>(profile));    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }
  }
};
using EncodingContainerProfilePtr = std::unique_ptr<GstEncodingContainerProfile, GstEncodingContainerProfileDeleter>;

struct GstEncodingVideoProfileDeleter final {
  void operator()(GstEncodingVideoProfile* profile) const noexcept {
    if(profile != nullptr) {
      gst_encoding_profile_unref(
          reinterpret_cast<GstEncodingProfile*>(profile));    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    }
  }
};
using EncodingVideoProfilePtr = std::unique_ptr<GstEncodingVideoProfile, GstEncodingVideoProfileDeleter>;

struct GstNetTimeProviderDeleter final {
  void operator()(GstNetTimeProvider* provider) const noexcept {
    if(provider != nullptr) {
      gst_object_unref(provider);
    }
  }
};
using NetTimeProviderPtr = std::unique_ptr<GstNetTimeProvider, GstNetTimeProviderDeleter>;

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

inline constexpr GstClockTime kSecond = GST_SECOND;
inline constexpr GstClockTime kMsecond = GST_MSECOND;
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
    // A recoverable parse error yields both a partial pipeline and an error;
    // the partial pipeline is ours to drop.
    if(element != nullptr) {
      gst_object_unref(element);
    }
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

inline nonstd::expected<void, std::string> element_set_state(Element element, State state) {
  return element_set_state(element, static_cast<GstState>(state));
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

inline nonstd::expected<ElementState, std::string> element_get_state(Element element, GstClockTime timeout = kClockTimeNone) {
  GstState state = GST_STATE_NULL;
  GstState pending = GST_STATE_NULL;
  const GstStateChangeReturn ret = gst_element_get_state(element.get(), &state, &pending, timeout);
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

inline nonstd::expected<void, std::string> element_seek_simple(Element element,
                                                               GstFormat format,
                                                               SeekFlagsFlags flags,
                                                               gint64 seek_pos) {
  if(!gst_element_seek_simple(element.get(), format, static_cast<GstSeekFlags>(flags.value()), seek_pos)) {
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

inline nonstd::expected<PadPtr, std::string> element_request_pad_simple(Element element, std::string_view pad_name) {
  std::string name_str(pad_name);
  GstPad* pad = gst_element_request_pad_simple(element.get(), name_str.c_str());
  if(pad == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to request pad '{}' from element", pad_name));
  }
  return PadPtr{pad};
}

// ============================================================================
// element_factory_find
// ============================================================================

inline nonstd::expected<ElementFactoryPtr, std::string> element_factory_find(std::string_view factory_name) {
  std::string name_str(factory_name);
  GstElementFactory* factory = gst_element_factory_find(name_str.c_str());
  if(factory == nullptr) {
    return nonstd::make_unexpected(fmt::format("Element factory '{}' not found", factory_name));
  }
  return ElementFactoryPtr{factory};
}

// ============================================================================
// bin_add_many / bin_remove / bin_get_by_name
// ============================================================================

inline nonstd::expected<void, std::string> bin_add_many(Pipeline pipeline, std::initializer_list<Element> elements) {
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

inline nonstd::expected<ElementPtr, std::string> bin_get_by_name(Pipeline pipeline, std::string_view element_name) {
  std::string name_str(element_name);
  GstElement* elem = gst_bin_get_by_name(GST_BIN(pipeline.get()), name_str.c_str());
  if(elem == nullptr) {
    return nonstd::make_unexpected(fmt::format("No element named '{}' in pipeline", element_name));
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

inline std::string_view structure_get_string(const GstStructure* structure, std::string_view field_name) {
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
      bus.get(),
      G_PRIORITY_DEFAULT,
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
// bin_add_many / element_link_many / element_query / element_register
// ============================================================================
// bin_add_many already exists above (kept next to bin_add for discoverability).

inline nonstd::expected<void, std::string> element_link_many(std::initializer_list<Element> elements) {
  if(elements.size() < 2) {
    return {};
  }
  auto it = elements.begin();
  Element prev = *it;
  for(++it; it != elements.end(); ++it) {
    if(auto result = element_link(prev, *it); !result) {
      return result;
    }
    prev = *it;
  }
  return {};
}

inline nonstd::expected<void, std::string> element_query(Element element, GstQuery* query) {
  if(!gst_element_query(element.get(), query)) {
    return nonstd::make_unexpected(std::string("Element query failed"));
  }
  return {};
}

inline nonstd::expected<void, std::string> element_register(std::string_view name,
                                                            guint rank,
                                                            GType type,
                                                            GstPlugin* plugin = nullptr) {
  std::string name_str(name);
  if(!gst_element_register(plugin, name_str.c_str(), rank, type)) {
    return nonstd::make_unexpected(fmt::format("Failed to register element '{}'", name));
  }
  return {};
}

// ============================================================================
// Buffers / memory
// ============================================================================

inline nonstd::expected<BufferPtr, std::string> buffer_copy(Buffer buffer) {
  GstBuffer* copy = gst_buffer_copy(buffer.get());
  if(copy == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to copy buffer"));
  }
  return BufferPtr{copy};
}

inline nonstd::expected<gsize, std::string> buffer_fill(Buffer buffer, gsize offset, std::span<const std::byte> data) {
  const gsize written = gst_buffer_fill(buffer.get(), offset, data.data(), data.size());
  if(written != data.size()) {
    return nonstd::make_unexpected(fmt::format("Short buffer fill: wrote {} of {} bytes", written, data.size()));
  }
  return written;
}

inline nonstd::expected<BufferPtr, std::string> buffer_new_allocate(gsize size) {
  GstBuffer* buf = gst_buffer_new_allocate(nullptr, size, nullptr);
  if(buf == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to allocate buffer"));
  }
  return BufferPtr{buf};
}

inline guint buffer_n_memory(Buffer buffer) noexcept {
  return gst_buffer_n_memory(buffer.get());
}

inline Buffer sample_get_buffer(Sample sample) noexcept {
  return Buffer{gst_sample_get_buffer(sample.get())};
}

// BufferMapGuard: move-only RAII guard over GstMapInfo. Unmaps in its
// destructor so callers can't forget to pair gst_buffer_map with
// gst_buffer_unmap (the same footgun myedgedetector.c/mysrc.c/mysink.c
// hand-roll today). Kept minimal: no read/write helpers beyond exposing the
// raw GstMapInfo, mirroring how thin the rest of this layer stays.
class BufferMapGuard {
public:
  BufferMapGuard() noexcept = default;
  BufferMapGuard(GstBuffer* buffer, GstMapInfo info) noexcept : m_buffer(buffer), m_info(info), m_mapped(true) {}

  ~BufferMapGuard() {
    unmap();
  }
  BufferMapGuard(const BufferMapGuard&) = delete;
  BufferMapGuard& operator=(const BufferMapGuard&) = delete;
  BufferMapGuard(BufferMapGuard&& other) noexcept : m_buffer(other.m_buffer), m_info(other.m_info), m_mapped(other.m_mapped) {
    other.m_mapped = false;
  }
  BufferMapGuard& operator=(BufferMapGuard&& other) noexcept {
    if(this != &other) {
      unmap();
      m_buffer = other.m_buffer;
      m_info = other.m_info;
      m_mapped = other.m_mapped;
      other.m_mapped = false;
    }
    return *this;
  }

  [[nodiscard]] const GstMapInfo& info() const noexcept {
    return m_info;
  }
  [[nodiscard]] std::span<std::byte> data() const noexcept {
    return {reinterpret_cast<std::byte*>(m_info.data), m_info.size};    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  }

private:
  void unmap() noexcept {
    if(m_mapped) {
      gst_buffer_unmap(m_buffer, &m_info);
      m_mapped = false;
    }
  }

  GstBuffer* m_buffer = nullptr;
  GstMapInfo m_info{};
  bool m_mapped = false;
};

static_assert(!std::is_copy_constructible_v<BufferMapGuard>);
static_assert(std::is_move_constructible_v<BufferMapGuard>);

inline nonstd::expected<BufferMapGuard, std::string> buffer_map(Buffer buffer, MapFlags flags) {
  GstMapInfo info{};
  if(!gst_buffer_map(buffer.get(), &info, static_cast<GstMapFlags>(flags))) {
    return nonstd::make_unexpected(std::string("Failed to map buffer"));
  }
  return BufferMapGuard{buffer.get(), info};
}

// ============================================================================
// Caps / structure
// ============================================================================
// Both forward straight to the underlying NULL/field-count-free C varargs
// call, exactly like calling gst_caps_new_simple/gst_structure_new by hand;
// the requires clause only guards against an obviously-malformed field list
// (name, GType, value triples).

template <typename... Args>
  requires(sizeof...(Args) % 3 == 0)
inline nonstd::expected<CapsPtr, std::string> caps_new_simple(std::string_view media_type, Args&&... args) {
  std::string media_str(media_type);
  GstCaps* caps = gst_caps_new_simple(media_str.c_str(), std::forward<Args>(args)..., nullptr);
  if(caps == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create caps for '{}'", media_type));
  }
  return CapsPtr{caps};
}

template <typename... Args>
  requires(sizeof...(Args) % 3 == 0)
inline nonstd::expected<StructurePtr, std::string> structure_new(std::string_view name, Args&&... args) {
  std::string name_str(name);
  GstStructure* structure = gst_structure_new(name_str.c_str(), std::forward<Args>(args)..., nullptr);
  if(structure == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create structure '{}'", name));
  }
  return StructurePtr{structure};
}

inline bool structure_has_name(const GstStructure* structure, std::string_view name) noexcept {
  std::string name_str(name);
  return gst_structure_has_name(structure, name_str.c_str()) != FALSE;
}

// ============================================================================
// Tag list
// ============================================================================

template <typename... Args>
  requires(sizeof...(Args) % 2 == 0)
inline TagListPtr tag_list_new(Args&&... args) noexcept {
  return TagListPtr{gst_tag_list_new(std::forward<Args>(args)..., nullptr)};
}

inline void tag_list_foreach(TagList tags, GstTagForeachFunc func, gpointer user_data) noexcept {
  gst_tag_list_foreach(tags.get(), func, user_data);
}

inline const GValue* tag_list_get_value_index(TagList tags, std::string_view tag, guint index) noexcept {
  std::string tag_str(tag);
  return gst_tag_list_get_value_index(tags.get(), tag_str.c_str(), index);
}

// ============================================================================
// Bus / messaging (structure/tag extensions)
// ============================================================================

inline const GstStructure* message_get_structure(Message msg) noexcept {
  return gst_message_get_structure(msg.get());
}

// Both consume the resource they're handed (transfer-full into the message),
// matching gst_message_new_application/gst_message_new_tag semantics.
inline MessagePtr message_new_application(GstObject* src, StructurePtr structure) noexcept {
  return MessagePtr{gst_message_new_application(src, structure.release())};
}

inline MessagePtr message_new_tag(GstObject* src, TagListPtr tags) noexcept {
  return MessagePtr{gst_message_new_tag(src, tags.release())};
}

inline nonstd::expected<TagListPtr, std::string> message_parse_tag(Message msg) {
  GstTagList* tags = nullptr;
  gst_message_parse_tag(msg.get(), &tags);
  if(tags == nullptr) {
    return nonstd::make_unexpected(std::string("No tag list found in message"));
  }
  return TagListPtr{tags};
}

// ============================================================================
// Pad probes
// ============================================================================
// ponytail: heap-allocates std::function; freed via GDestroyNotify when the
// probe is removed — same pattern as bus_add_watch above.

inline gulong pad_add_probe(Pad pad, PadProbeTypeFlags mask, std::function<PadProbeReturn(Pad, GstPadProbeInfo*)> callback) {
  auto* cb_ptr = new std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>(std::move(callback));
  return gst_pad_add_probe(
      pad.get(),
      static_cast<GstPadProbeType>(mask.value()),
      [](GstPad* probe_pad, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
        auto& cb = *static_cast<std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>*>(data);
        return static_cast<GstPadProbeReturn>(cb(Pad{probe_pad}, info));
      },
      cb_ptr,
      [](gpointer data) { delete static_cast<std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>*>(data); });
}

inline void pad_remove_probe(Pad pad, gulong probe_id) noexcept {
  gst_pad_remove_probe(pad.get(), probe_id);
}

// ============================================================================
// Query
// ============================================================================

inline QueryPtr query_new_latency() noexcept {
  return QueryPtr{gst_query_new_latency()};
}

struct LatencyInfo {
  bool live;
  GstClockTime min;
  GstClockTime max;
};

inline LatencyInfo query_parse_latency(Query query) noexcept {
  LatencyInfo info{};
  gboolean live = FALSE;
  gst_query_parse_latency(query.get(), &live, &info.min, &info.max);
  info.live = live != FALSE;
  return info;
}

// element_query_position / element_query_duration already exist above.

// ============================================================================
// Element factory introspection
// ============================================================================

inline std::string_view element_factory_get_metadata(ElementFactory factory, std::string_view key) {
  std::string key_str(key);
  const gchar* value = gst_element_factory_get_metadata(factory.get(), key_str.c_str());
  return value != nullptr ? std::string_view{value} : std::string_view{};
}

inline const GList* element_factory_get_static_pad_templates(ElementFactory factory) noexcept {
  return gst_element_factory_get_static_pad_templates(factory.get());
}

// element_factory_find already exists above.

// ============================================================================
// Plugin / registry
// ============================================================================

inline std::string_view plugin_get_name(Plugin plugin) noexcept {
  return std::string_view{gst_plugin_get_name(plugin.get())};
}

inline std::string_view plugin_feature_get_name(PluginFeature feature) noexcept {
  return std::string_view{gst_plugin_feature_get_name(feature.get())};
}

inline Registry registry_get() noexcept {
  return Registry{gst_registry_get()};
}

// Builds an owning vector from the GList so callers never touch
// gst_plugin_list_free themselves: each element keeps its ref inside a
// PluginPtr, and only the GList spine (not the plugin refs) is freed here.
inline std::vector<PluginPtr> registry_get_plugin_list(Registry registry) {
  GList* list = gst_registry_get_plugin_list(registry.get());
  std::vector<PluginPtr> plugins;
  for(GList* node = list; node != nullptr; node = node->next) {
    plugins.emplace_back(static_cast<GstPlugin*>(node->data));
  }
  g_list_free(list);
  return plugins;
}

inline std::vector<PluginFeaturePtr> registry_get_feature_list_by_plugin(Registry registry, std::string_view plugin_name) {
  std::string name_str(plugin_name);
  GList* list = gst_registry_get_feature_list_by_plugin(registry.get(), name_str.c_str());
  std::vector<PluginFeaturePtr> features;
  for(GList* node = list; node != nullptr; node = node->next) {
    features.emplace_back(static_cast<GstPluginFeature*>(node->data));
  }
  g_list_free(list);
  return features;
}

// ============================================================================
// Encoding profiles
// ============================================================================

inline nonstd::expected<EncodingContainerProfilePtr, std::string> encoding_container_profile_new(std::string_view name,
                                                                                                 std::string_view description,
                                                                                                 GstCaps* format,
                                                                                                 std::string_view preset = {}) {
  std::string name_str(name);
  std::string desc_str(description);
  std::string preset_str(preset);
  GstEncodingContainerProfile* profile = gst_encoding_container_profile_new(
      name_str.c_str(), desc_str.c_str(), format, preset.empty() ? nullptr : preset_str.c_str());
  if(profile == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create encoding container profile '{}'", name));
  }
  return EncodingContainerProfilePtr{profile};
}

// Transfers the video profile into the container (gst_encoding_container_profile_add_profile
// takes ownership of it on success — and on failure the C API still consumes the ref).
inline nonstd::expected<void, std::string> encoding_container_profile_add_profile(const EncodingContainerProfilePtr& container,
                                                                                  EncodingVideoProfilePtr video_profile) {
  if(!gst_encoding_container_profile_add_profile(container.get(), GST_ENCODING_PROFILE(video_profile.release()))) {
    return nonstd::make_unexpected(std::string("Failed to add video profile to container profile"));
  }
  return {};
}

inline nonstd::expected<EncodingVideoProfilePtr, std::string> encoding_video_profile_new(GstCaps* format,
                                                                                         std::string_view preset,
                                                                                         GstCaps* restriction,
                                                                                         guint presence) {
  std::string preset_str(preset);
  GstEncodingVideoProfile* profile = gst_encoding_video_profile_new(
      format, preset.empty() ? nullptr : preset_str.c_str(), restriction, presence);
  if(profile == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create encoding video profile"));
  }
  return EncodingVideoProfilePtr{profile};
}

// ============================================================================
// Clock / net sync
// ============================================================================

inline nonstd::expected<void, std::string> clock_wait_for_sync(Clock clock, GstClockTime timeout) {
  if(!gst_clock_wait_for_sync(clock.get(), timeout)) {
    return nonstd::make_unexpected(std::string("Clock did not sync within timeout"));
  }
  return {};
}

inline nonstd::expected<ClockPtr, std::string> net_client_clock_new(std::string_view name,
                                                                    std::string_view remote_address,
                                                                    gint port,
                                                                    GstClockTime base_time) {
  std::string name_str(name);
  std::string addr_str(remote_address);
  GstClock* clock = gst_net_client_clock_new(name_str.c_str(), addr_str.c_str(), port, base_time);
  if(clock == nullptr) {
    return nonstd::make_unexpected(fmt::format("Failed to create net client clock to '{}':{}", remote_address, port));
  }
  return ClockPtr{clock};
}

inline nonstd::expected<NetTimeProviderPtr, std::string> net_time_provider_new(Clock clock, std::string_view address, gint port) {
  std::string addr_str(address);
  GstNetTimeProvider* provider = gst_net_time_provider_new(clock.get(), address.empty() ? nullptr : addr_str.c_str(), port);
  if(provider == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to create net time provider"));
  }
  return NetTimeProviderPtr{provider};
}

inline void pipeline_use_clock(Pipeline pipeline, Clock clock) noexcept {
  gst_pipeline_use_clock(GST_PIPELINE(pipeline.get()), clock.get());
}

// ============================================================================
// Navigation events
// ============================================================================

inline GstNavigationEventType navigation_event_get_type(GstEvent* event) noexcept {
  return gst_navigation_event_get_type(event);
}

inline nonstd::expected<std::string, std::string> navigation_event_parse_key_event(GstEvent* event) {
  const gchar* key = nullptr;
  if(!gst_navigation_event_parse_key_event(event, &key) || key == nullptr) {
    return nonstd::make_unexpected(std::string("Not a navigation key event"));
  }
  return std::string{key};
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
