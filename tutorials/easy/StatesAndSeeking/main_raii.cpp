#include <algorithm>
#include <cstdlib>
#include <span>

#include <fmt/printf.h>

#include <gst/video/navigation.h>

#include "gstreamer_raii.hpp"

namespace {

// Create each factory (null-terminated list), transfer ownership into the
// pipeline bin, sync it to the running state, link the chain head-to-tail, then
// link `src_pad` into the head. Returns false on any failure.
bool build_branch(const gst::raii::Pipeline& pipeline, GstPad* src_pad, std::span<const char* const> factories) {
  gst::Element head{nullptr};
  gst::Element prev{nullptr};
  for(const char* factory : factories) {
    auto element = gst::raii::element_factory_make(factory);
    if(!element) {
      fmt::print(stderr, "Failed to create {}: {}\n", factory, element.error());
      return false;
    }
    auto added = gst::raii::bin_add(pipeline, std::move(*element));
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

// glimagesink sends key presses upstream as GST_EVENT_NAVIGATION; post them as
// application messages so the main thread can act on them without deadlocking.
gst::PadProbeReturn on_nav_event(GstElement* pipeline, gst::Pad /*pad*/, GstPadProbeInfo* info) {
  auto* event = GST_PAD_PROBE_INFO_EVENT(info);
  if(GST_EVENT_NAVIGATION != GST_EVENT_TYPE(event)) {
    return gst::PadProbeReturn::Ok;
  }
  if(GST_NAVIGATION_EVENT_KEY_PRESS != gst::navigation_event_get_type(event)) {
    return gst::PadProbeReturn::Ok;
  }
  auto key_result = gst::navigation_event_parse_key_event(event);
  if(!key_result) {
    return gst::PadProbeReturn::Ok;
  }
  const std::string& key = key_result.value();
  // Hand off to the main thread — a flushing seek / state change from the
  // streaming thread deadlocks. Post the key as an application message.
  auto s = gst::structure_new("keypress", "key", G_TYPE_STRING, key.c_str());
  if(!s) {
    return gst::PadProbeReturn::Ok;
  }
  auto app_msg = gst::message_new_application(GST_OBJECT(pipeline), std::move(*s));
  std::ignore = gst::element_post_message(gst::Element{pipeline}, app_msg.release());
  return gst::PadProbeReturn::Ok;
}

// decodebin exposes its decoded output pads only after examining the stream, so
// we build the matching sink branch on demand here, routing audio and video to
// their own chains by caps.
void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* pipeline = static_cast<gst::raii::Pipeline*>(user_data);

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
    if(build_branch(*pipeline, new_pad, branch)) {
      // Install upstream nav-event probe on the decodebin src pad so key events
      // from glimagesink bubble up and reach our handler.
      GstElement* raw_pipeline = pipeline->get();
      std::ignore = gst::pad_add_probe(gst::Pad{new_pad}, gst::PadProbeType::EventUpstream,
                                        [raw_pipeline](gst::Pad pad, GstPadProbeInfo* info) {
                                          return on_nav_event(raw_pipeline, pad, info);
                                        });
    }
  } else if(name.starts_with("audio/x-raw")) {
    const char* const branch[] = {"audioconvert", "audioresample", "autoaudiosink"};
    build_branch(*pipeline, new_pad, branch);
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

  // gst::raii::pipeline_new — returns an owning Pipeline (freed on scope exit)
  auto pipeline = gst::raii::pipeline_new("states-seeking");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::raii::element_factory_make("filesrc", "source");
  auto decode = gst::raii::element_factory_make("decodebin", "decoder");

  if(!source || !decode) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", argv[1], nullptr);

  // raii::bin_add transfers ownership of each element into the pipeline bin and
  // returns a non-owning gst::Element handle for linking (bin now owns it).
  auto raw_source = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_decode = gst::raii::bin_add(*pipeline, std::move(*decode));

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

  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), &*pipeline);

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

  // raii::element_get_bus returns an owning Bus (freed on scope exit)
  auto bus = gst::raii::element_get_bus(*pipeline);
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
      const GstStructure* s = gst::message_get_structure(gst::Message{msg.get()});
      if(nullptr == s || !gst::structure_has_name(s, "keypress")) {
        continue;
      }
      const char* key = gst_structure_get_string(s, "key");
      if(nullptr == key) {
        continue;
      }
      const std::string_view key_view{key};
      if("Right" == key_view) {
        seek_relative(*pipeline, SeekStep);
      } else if("Left" == key_view) {
        seek_relative(*pipeline, -SeekStep);
      } else if("Space" == key_view) {
        const GstState target = playing ? GST_STATE_PAUSED : GST_STATE_PLAYING;
        std::ignore = gst::element_set_state(*pipeline, target);
        std::ignore = gst::element_get_state(*pipeline);
        playing = !playing;
        fmt::print(stdout, "State: {} → {}\n", playing ? "PAUSED" : "PLAYING", playing ? "PLAYING" : "PAUSED");
      } else if("Q" == key_view) {
        running = false;
      }
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline goes out of scope here — gst::raii::Pipeline destructor
  // calls gst_object_unref, which in turn unrefs all contained elements.

  return EXIT_SUCCESS;
}
