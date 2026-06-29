#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {
void pad_added_handler(GstElement* /*src*/, GstPad* new_pad, GstElement* sink) {
  auto sink_pad = gst::element_get_static_pad(sink, "sink");
  if(!sink_pad) {
    fmt::println(stderr, "Failed to get sink pad: {}", sink_pad.error());
    return;
  }

  if(gst::pad_is_linked(*sink_pad)) {
    fmt::println(stdout, "Pad already linked, ignoring.");
    return;
  }

  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    fmt::println(stderr, "No caps on new pad: {}", caps.error());
    return;
  }

  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    fmt::println(stderr, "No structure in caps: {}", structure.error());
    return;
  }

  const auto pad_type = gst::structure_get_name(*structure);
  if(!pad_type.starts_with("video/x-raw")) {
    fmt::println(stdout, "Pad type '{}' is not raw video, ignoring.", pad_type);
    return;
  }

  if(auto link = gst::pad_link(new_pad, *sink_pad); !link) {
    fmt::println(stderr, "Failed to link pad of type '{}': {}", pad_type, link.error());
  } else {
    fmt::println(stdout, "Linked pad of type '{}'.", pad_type);
  }
}
}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline = gst::pipeline_new("video-player");
  if(!pipeline) {
    fmt::println(stderr, "Failed to create pipeline: {}", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::element_factory_make("filesrc", "file-source");
  if(!source) {
    fmt::println(stderr, "{}", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", "big-buck-bunny-480p-30sec.mp4", nullptr);

  auto decode = gst::element_factory_make("decodebin", "decoder");
  if(!decode) {
    fmt::println(stderr, "{}", decode.error());
    return EXIT_FAILURE;
  }

  auto sink = gst::element_factory_make("autovideosink", "video-output");
  if(!sink) {
    fmt::println(stderr, "{}", sink.error());
    return EXIT_FAILURE;
  }

  auto raw_source = gst::bin_add(*pipeline, std::move(*source));
  auto raw_decode = gst::bin_add(*pipeline, std::move(*decode));
  auto raw_sink = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_decode || !raw_sink) {
    fmt::println(stderr, "Failed to add elements to pipeline.");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_decode); !link) {
    fmt::println(stderr, "Failed to link source to decoder: {}", link.error());
    return EXIT_FAILURE;
  }

  // decodebin exposes pads dynamically; link them to the sink on pad-added.
  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(pad_added_handler), *raw_sink);

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::println(stderr, "Failed to start pipeline: {}", state.error());
    return EXIT_FAILURE;
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::println(stderr, "Failed to get bus: {}", bus.error());
    return EXIT_FAILURE;
  }

  auto terminate = false;
  do {
    auto msg_result = gst::bus_timed_pop_filtered(
        *bus, GST_CLOCK_TIME_NONE, gst::MessageType::StateChanged | gst::MessageType::Error | gst::MessageType::EOS);

    if(!msg_result) {
      continue;
    }

    const auto& msg = msg_result.value();
    const auto msg_type = gst::message_type(msg);

    if(gst::MessageType::Error == msg_type) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::println(stderr, "Error: {}\nDebug: {}", parsed->first, parsed->second);
      }
      terminate = true;
    } else if(gst::MessageType::EOS == msg_type) {
      fmt::println(stdout, "End of stream reached.");
      terminate = true;
    } else if(gst::MessageType::StateChanged == msg_type) {
      if(GST_MESSAGE_SRC(msg.get()) == GST_OBJECT(pipeline->get())) {
        const auto [old_state, new_state, pending] = gst::message_parse_state_changed(msg);
        fmt::println(stdout, "Pipeline state: {} -> {}.", gst::state_get_name(old_state), gst::state_get_name(new_state));
      }
    }
  } while(!terminate);

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);

  return EXIT_SUCCESS;
}
