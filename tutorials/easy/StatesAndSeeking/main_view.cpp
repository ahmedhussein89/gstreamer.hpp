#include <algorithm>
#include <cstdlib>
#include <span>

#include <gst/video/navigation.h>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

// Create each factory (null-terminated list), add it to the pipeline, sync it to
// the running state, link the chain head-to-tail, then link `src_pad` into the
// head. Returns false on any failure.
bool build_branch(gst::Pipeline pipeline, GstPad* src_pad, std::span<const char* const> factories) {
  gst::Element head{nullptr};
  gst::Element prev{nullptr};
  for(const char* factory : factories) {
    auto element = gst::element_factory_make(factory);
    if(!element) {
      fmt::print(stderr, "Failed to create {}: {}\n", factory, element.error());
      return false;
    }
    auto added = gst::bin_add(pipeline, *element);
    if(!added) {
      return false;
    }
    std::ignore = gst::element_sync_state_with_parent(*added);
    if(!head.get()) {
      head = *added;
    }
    if(prev.get()) {
      if(auto link = gst::element_link(prev, *added); !link) {
        fmt::print(stderr, "Failed to link {} branch: {}\n", factories[0], link.error());
        return false;
      }
    }
    prev = *added;
  }

  auto sink_pad = gst::element_get_static_pad(head, "sink");
  if(!sink_pad) {
    return false;
  }
  return gst::pad_link(src_pad, *sink_pad).has_value();
}

// Intercept upstream navigation events from glimagesink and re-post key presses
// as application messages so the main thread (bus loop) handles them safely.
// Seeking/state changes from the streaming thread would deadlock.
GstPadProbeReturn on_nav_event(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer user_data) {
  auto* pipeline = static_cast<GstElement*>(user_data);
  auto* event = GST_PAD_PROBE_INFO_EVENT(info);
  if(GST_EVENT_NAVIGATION != GST_EVENT_TYPE(event)) {
    return GST_PAD_PROBE_OK;
  }
  if(GST_NAVIGATION_EVENT_KEY_PRESS != gst_navigation_event_get_type(event)) {
    return GST_PAD_PROBE_OK;
  }
  const char* key = nullptr;
  if(TRUE != gst_navigation_event_parse_key_event(event, &key) || nullptr == key) {
    return GST_PAD_PROBE_OK;
  }
  GstStructure* s = gst_structure_new("keypress", "key", G_TYPE_STRING, key, nullptr);
  gst_element_post_message(pipeline, gst_message_new_application(GST_OBJECT(pipeline), s));
  return GST_PAD_PROBE_OK;
}

// decodebin exposes its decoded output pads only after examining the stream, so
// we build the matching sink branch on demand here, routing audio and video to
// their own chains by caps.
void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  gst::Pipeline pipeline{static_cast<GstElement*>(user_data)};

  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    return;
  }
  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    return;
  }

  const auto name = gst::structure_get_name(*structure);
  if(name.starts_with("video/x-raw")) {
    const char* const branch[] = {"videoconvert", "glimagesink"};
    if(build_branch(pipeline, new_pad, branch)) {
      gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, on_nav_event, pipeline.get(), nullptr);
    }
  } else if(name.starts_with("audio/x-raw")) {
    const char* const branch[] = {"audioconvert", "audioresample", "autoaudiosink"};
    build_branch(pipeline, new_pad, branch);
  }
}

constexpr gint64 SeekStep = 5 * GST_SECOND;

// Relative seek: query position, clamp pos +/- SeekStep to [0, duration], flush-seek.
void seek_relative(gst::Element pipeline, gint64 delta) {
  auto pos = gst::element_query_position(pipeline, GST_FORMAT_TIME);
  if(!pos) {
    fmt::print(stderr, "Failed to query position: {}\n", pos.error());
    return;
  }

  auto dur = gst::element_query_duration(pipeline, GST_FORMAT_TIME);
  const gint64 duration = dur ? dur.value() : (pos.value() + delta);    // best effort if unavailable

  const gint64 target = std::clamp(pos.value() + delta, gint64{0}, duration);
  auto seek = gst::element_seek_simple(pipeline, GST_FORMAT_TIME, gst::SeekFlags::Flush | gst::SeekFlags::KeyUnit, target);
  if(seek) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(target) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
  }
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <video-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto pipeline = gst::pipeline_new("states-seeking");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::element_factory_make("filesrc", "source");
  auto decode = gst::element_factory_make("decodebin", "decoder");

  if(!source || !decode) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", argv[1], nullptr);

  auto raw_source = gst::bin_add(*pipeline, *source);
  auto raw_decode = gst::bin_add(*pipeline, *decode);

  if(!raw_source || !raw_decode) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  // filesrc and decodebin both have static pads, so they link now; decodebin
  // links to the sink branches later from the pad-added callback.
  if(auto link = gst::element_link(*raw_source, *raw_decode); !link) {
    fmt::print(stderr, "Failed to link source to decoder: {}\n", link.error());
    return EXIT_FAILURE;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), pipeline->get());
#pragma clang diagnostic pop

  // Cycle through states explicitly to show each transition.
  std::ignore = gst::element_set_state(*pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_PAUSED);
  std::ignore = gst::element_get_state(*pipeline);
  fmt::print(stdout, "State: READY → PAUSED\n");

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }
  std::ignore = gst::element_get_state(*pipeline);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  fmt::print(stdout, "←/→ seek 5s · space pause/resume · q quit\n");

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  bool playing = true;
  bool running = true;
  while(running) {
    auto msg_result = gst::bus_timed_pop_filtered(
      *bus, 100 * GST_MSECOND, gst::MessageType::Error | gst::MessageType::EOS | gst::MessageType::Application);
    if(!msg_result) {
      continue;
    }
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto error_result = gst::message_parse_error(msg.get());
      if(error_result) {
        fmt::print(stderr, "Error: {}\n", error_result.value().first);
      }
      running = false;
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
      running = false;
    } else if(gst::MessageType::Application == gst::message_type(msg)) {
      const GstStructure* s = gst_message_get_structure(msg.get());
      if(nullptr == s || !gst_structure_has_name(s, "keypress")) {
        continue;
      }
      const gchar* key = gst_structure_get_string(s, "key");
      if(nullptr == key) {
        continue;
      }
      if(0 == g_strcmp0(key, "Right")) {
        seek_relative(*pipeline, SeekStep);
      } else if(0 == g_strcmp0(key, "Left")) {
        seek_relative(*pipeline, -SeekStep);
      } else if(0 == g_strcmp0(key, "space")) {
        const GstState target = playing ? GST_STATE_PAUSED : GST_STATE_PLAYING;
        std::ignore = gst::element_set_state(*pipeline, target);
        std::ignore = gst::element_get_state(*pipeline);
        playing = !playing;
        fmt::print(stdout, "State: {} → {}\n", playing ? "PAUSED" : "PLAYING", playing ? "PLAYING" : "PAUSED");
      } else if(0 == g_strcmp0(key, "q")) {
        running = false;
      }
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);

  return EXIT_SUCCESS;
}
