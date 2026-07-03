#pragma once
#include <string_view>

#include <gst/gst.h>
#include <gst/gstformat.h>
#include <gst/gstpad.h>

namespace gst {

// ---------------------------------------------------------------------------
// gst::State — wraps GstState
// ---------------------------------------------------------------------------
enum class State : int {
  VoidPending = GST_STATE_VOID_PENDING,
  Null = GST_STATE_NULL,
  Ready = GST_STATE_READY,
  Paused = GST_STATE_PAUSED,
  Playing = GST_STATE_PLAYING,
};

[[nodiscard]] inline std::string_view to_string(State s) noexcept {
  return gst_element_state_get_name(static_cast<GstState>(s));
}

// ---------------------------------------------------------------------------
// gst::StateChangeReturn — wraps GstStateChangeReturn
// ---------------------------------------------------------------------------
enum class StateChangeReturn : int {
  Failure = GST_STATE_CHANGE_FAILURE,
  Success = GST_STATE_CHANGE_SUCCESS,
  Async = GST_STATE_CHANGE_ASYNC,
  NoPreroll = GST_STATE_CHANGE_NO_PREROLL,
};

[[nodiscard]] inline std::string_view to_string(StateChangeReturn r) noexcept {
  return gst_element_state_change_return_get_name(static_cast<GstStateChangeReturn>(r));
}

// ---------------------------------------------------------------------------
// gst::FlowReturn — wraps GstFlowReturn
// ---------------------------------------------------------------------------
enum class FlowReturn : int {
  CustomSuccess2 = GST_FLOW_CUSTOM_SUCCESS_2,
  CustomSuccess1 = GST_FLOW_CUSTOM_SUCCESS_1,
  CustomSuccess = GST_FLOW_CUSTOM_SUCCESS,
  Ok = GST_FLOW_OK,
  NotLinked = GST_FLOW_NOT_LINKED,
  Flushing = GST_FLOW_FLUSHING,
  Eos = GST_FLOW_EOS,
  NotNegotiated = GST_FLOW_NOT_NEGOTIATED,
  Error = GST_FLOW_ERROR,
  NotSupported = GST_FLOW_NOT_SUPPORTED,
  CustomError = GST_FLOW_CUSTOM_ERROR,
  CustomError1 = GST_FLOW_CUSTOM_ERROR_1,
  CustomError2 = GST_FLOW_CUSTOM_ERROR_2,
};

[[nodiscard]] inline std::string_view to_string(FlowReturn r) noexcept {
  return gst_flow_get_name(static_cast<GstFlowReturn>(r));
}

// ---------------------------------------------------------------------------
// gst::PadDirection — wraps GstPadDirection
// ---------------------------------------------------------------------------
enum class PadDirection : int {
  Unknown = GST_PAD_UNKNOWN,
  Src = GST_PAD_SRC,
  Sink = GST_PAD_SINK,
};

[[nodiscard]] inline constexpr std::string_view to_string(PadDirection d) noexcept {
  switch(d) {
    case PadDirection::Src:     return "src";
    case PadDirection::Sink:    return "sink";
    default:                    return "unknown";
  }
}

// ---------------------------------------------------------------------------
// gst::PadPresence — wraps GstPadPresence
// ---------------------------------------------------------------------------
enum class PadPresence : int {
  Always = GST_PAD_ALWAYS,
  Sometimes = GST_PAD_SOMETIMES,
  Request = GST_PAD_REQUEST,
};

[[nodiscard]] inline constexpr std::string_view to_string(PadPresence p) noexcept {
  switch(p) {
    case PadPresence::Always:    return "always";
    case PadPresence::Sometimes: return "sometimes";
    case PadPresence::Request:   return "request";
    default:                     return "unknown";
  }
}

// ---------------------------------------------------------------------------
// gst::Format — wraps GstFormat
// ---------------------------------------------------------------------------
enum class Format : int {
  Undefined = GST_FORMAT_UNDEFINED,
  Default = GST_FORMAT_DEFAULT,
  Bytes = GST_FORMAT_BYTES,
  Time = GST_FORMAT_TIME,
  Buffers = GST_FORMAT_BUFFERS,
  Percent = GST_FORMAT_PERCENT,
};

[[nodiscard]] inline std::string_view to_string(Format f) noexcept {
  return gst_format_get_name(static_cast<GstFormat>(f));
}

// ---------------------------------------------------------------------------
// gst::SeekType — wraps GstSeekType
// ---------------------------------------------------------------------------
enum class SeekType : int {
  None = GST_SEEK_TYPE_NONE,
  Set = GST_SEEK_TYPE_SET,
  End = GST_SEEK_TYPE_END,
};

[[nodiscard]] inline constexpr std::string_view to_string(SeekType t) noexcept {
  switch(t) {
    case SeekType::None: return "none";
    case SeekType::Set:  return "set";
    case SeekType::End:  return "end";
    default:             return "unknown";
  }
}

}    // namespace gst
