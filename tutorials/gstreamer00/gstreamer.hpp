#pragma once
#include <cstdint>
#include <memory>
#include <string_view>

#include <gst/gst.h>
#include <gst/gstmessage.h>

#include <nonstd/expected.hpp>

namespace gst {

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
  Element = GST_MESSAGE_ELEMENT,
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
  Any = GST_MESSAGE_ANY
};

// Bitwise operators for MessageType enum class
constexpr MessageType operator|(MessageType lhs, MessageType rhs) {
  return static_cast<MessageType>(static_cast<std::int32_t>(lhs) | static_cast<std::int32_t>(rhs));
}

constexpr MessageType operator&(MessageType lhs, MessageType rhs) {
  return static_cast<MessageType>(static_cast<std::int32_t>(lhs) & static_cast<std::int32_t>(rhs));
}

struct GstElementDeleter {
  void operator()(GstElement* element) const {
    if(nullptr != element) {
      gst_object_unref(element);
    }
  }
};

using ElementPtr = std::unique_ptr<GstElement, GstElementDeleter>;

struct GstBusDeleter {
  void operator()(GstBus* bus) const {
    if(nullptr != bus) {
      gst_object_unref(bus);
    }
  }
};
using BusPtr = std::unique_ptr<GstBus, GstBusDeleter>;

struct GstErrorDeleter {
  void operator()(GError* error) const {
    if(nullptr != error) {
      g_error_free(error);
    }
  }
};
using ErrorPtr = std::unique_ptr<GError, GstErrorDeleter>;

struct GstMessageDeleter {
  void operator()(GstMessage* message) const {
    if(nullptr != message) {
      gst_message_unref(message);
    }
  }
};
using MessagePtr = std::unique_ptr<GstMessage, GstMessageDeleter>;

MessageType message_type(const MessagePtr& msg) {
  return static_cast<MessageType>(GST_MESSAGE_TYPE(msg.get()));
}

struct Element final {
  explicit Element(GstElement* element) : mElement(element) {}

  ~Element() = default;

  Element& operator=(const Element&) = delete;
  Element(const Element&) = delete;

  Element& operator=(Element&&) = default;
  Element(Element&&) = default;

  [[nodiscard]] GstElement* get() const {
    return mElement.get();
  }

  operator bool() const {
    return nullptr != mElement;
  }

private:
  ElementPtr mElement;
};

nonstd::expected<Element, ErrorPtr> parse_launch(std::string_view pipeline_description) {
  GError* error = nullptr;
  std::string pipeline_str(pipeline_description);

  GstElement* element = gst_parse_launch(pipeline_str.c_str(), &error);
  if(nullptr != error) {
    return nonstd::make_unexpected(ErrorPtr(error));
  }

  return Element(element);
}

nonstd::expected<std::pair<std::string, std::string>, std::string> message_parse_error(GstMessage* message) {
  GError* error = nullptr;
  gchar* debug_info = nullptr;

  gst_message_parse_error(message, &error, &debug_info);
  if(nullptr == error) {
    return nonstd::make_unexpected(std::string("No error found in message"));
  }

  auto result = std::make_pair(std::string{error->message}, std::string{GST_STR_NULL(debug_info)});
  g_error_free(error);
  g_free(debug_info);
  return result;
}

nonstd::expected<MessagePtr, std::string> bus_timed_pop_filtered(const BusPtr& bus, GstClockTime timeout, MessageType types) {
  GstMessage* msg = gst_bus_timed_pop_filtered(bus.get(), timeout, static_cast<GstMessageType>(types));
  if(nullptr == msg) {
    return nonstd::make_unexpected(std::string("No message received from bus"));
  }
  return MessagePtr(msg);
}

}    // namespace gst
