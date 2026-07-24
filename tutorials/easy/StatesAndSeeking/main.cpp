#include <algorithm>
#include <cstdlib>
#include <ranges>
#include <string>

#include <fmt/core.h>

#include <gst/gst.h>
#include <gst/video/navigation.h>

namespace {

// Create each factory in `factories` (null-terminated), add it to the pipeline,
// sync it to the running state, and link the chain head-to-tail; finally link
// `src_pad` into the chain's head. Returns false on any failure.
bool build_branch(GstElement* pipeline, GstPad* src_pad, const char* const* factories) {
  GstElement* head = nullptr;
  GstElement* prev = nullptr;
  for(const char* const* factory = factories; nullptr != *factory; ++factory) {
    GstElement* element = gst_element_factory_make(*factory, nullptr);
    if(nullptr == element) {
      fmt::print(stderr, "Failed to create {}.\n", *factory);
      return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
    gst_bin_add(GST_BIN(pipeline), element);
    gst_element_sync_state_with_parent(element);
    if(nullptr == head) {
      head = element;
    }
    if(nullptr != prev && TRUE != gst_element_link(prev, element)) {
      fmt::print(stderr, "Failed to link {} branch.\n", factories[0]);
      return false;
    }
    prev = element;
  }

  GstPad* sink_pad = gst_element_get_static_pad(head, "sink");
  const GstPadLinkReturn link = gst_pad_link(src_pad, sink_pad);
  gst_object_unref(sink_pad);
  return GST_PAD_LINK_OK == link;
}

// glimagesink sends key presses upstream as navigation events. Hand off to the
// main thread via an application message — a flushing seek / state change from
// the streaming thread deadlocks.
GstPadProbeReturn on_nav_event(GstPad*, GstPadProbeInfo* info, gpointer user_data) {
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
  auto* pipeline = static_cast<GstElement*>(user_data);

  GstCaps* caps = gst_pad_get_current_caps(new_pad);
  if(nullptr == caps) {
    return;
  }
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  const char* name = gst_structure_get_name(structure);

  if(TRUE == g_str_has_prefix(name, "video/x-raw")) {
    const char* const branch[] = {"videoconvert", "glimagesink", nullptr};
    if(build_branch(pipeline, new_pad, branch)) {
      gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, on_nav_event, pipeline, nullptr);
    }
  } else if(TRUE == g_str_has_prefix(name, "audio/x-raw")) {
    const char* const branch[] = {"audioconvert", "audioresample", "autoaudiosink", nullptr};
    build_branch(pipeline, new_pad, branch);
  }

  gst_caps_unref(caps);
}

constexpr gint64 SeekStep = 5 * GST_SECOND;

// Relative seek: query position, clamp pos +/- SeekStep to [0, duration], flush-seek.
void seek_relative(GstElement* pipeline, gint64 delta) {
  gint64 pos = 0;
  if(TRUE != gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)) {
    fmt::print(stderr, "Failed to query position.\n");
    return;
  }

  gint64 dur = 0;
  if(TRUE != gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur)) {
    dur = pos + delta;    // best effort if duration is unavailable
  }

  const gint64 target = std::clamp(pos + delta, gint64{0}, dur);
  if(gst_element_seek_simple(
         pipeline, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), target)) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(target) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
  }
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <video-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("states-seeking");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("filesrc", "source");
  auto* decode = gst_element_factory_make("decodebin", "decoder");

  if(nullptr == source || nullptr == decode) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "location", argv[1], nullptr);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  gst_bin_add_many(GST_BIN(pipeline), source, decode, nullptr);

  // filesrc and decodebin both have static pads, so they link now; decodebin
  // links to the sink branches later from the pad-added callback.
  if(TRUE != gst_element_link(source, decode)) {
    fmt::print(stderr, "Failed to link source to decoder.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  g_signal_connect(decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), pipeline);

  // Cycle through states explicitly to show each transition.
  gst_element_set_state(pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  gst_element_set_state(pipeline, GST_STATE_PAUSED);
  // Block until PAUSED is reached so the clock is assigned.
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: READY → PAUSED\n");

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  fmt::print(stdout, "←/→ seek 5s · space pause/resume · q quit\n");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    return EXIT_FAILURE;
  }

  bool playing = true;
  bool running = true;
  while(running) {
    auto* msg = gst_bus_timed_pop_filtered(
        bus, 100 * GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_APPLICATION));
    if(nullptr != msg) {
      if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
        GError* error = nullptr;
        gst_message_parse_error(msg, &error, nullptr);
        fmt::print(stderr, "Error: {}\n", error->message);
        g_error_free(error);
        running = false;
      } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
        fmt::print(stdout, "End of stream reached.\n");
        running = false;
      } else if(GST_MESSAGE_APPLICATION == GST_MESSAGE_TYPE(msg)) {
        const GstStructure* structure = gst_message_get_structure(msg);
        const char* key = (nullptr != structure && (gst_structure_has_name(structure, "keypress") != 0)) ?
            gst_structure_get_string(structure, "key") :
            nullptr;
        if(nullptr == key) {
          continue;
        }
        const std::string_view key_view{key};
        if("Right" == key_view) {
          seek_relative(pipeline, SeekStep);
        } else if("Left" == key_view) {
          seek_relative(pipeline, -SeekStep);
        } else if("Space" == key_view) {
          const GstState target = playing ? GST_STATE_PAUSED : GST_STATE_PLAYING;
          gst_element_set_state(pipeline, target);
          gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
          playing = !playing;
          fmt::print(stdout, "State: {} → {}\n", playing ? "PAUSED" : "PLAYING", playing ? "PLAYING" : "PAUSED");
        } else if("Q" == key_view) {
          running = false;
        }
      }
      gst_message_unref(msg);
    }
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
