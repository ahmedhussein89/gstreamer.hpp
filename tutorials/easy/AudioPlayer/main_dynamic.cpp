#include <cstdlib>
#include <span>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {
void pad_added_handler(GstElement* /*src*/, GstPad* new_pad, GstElement* sink) {
  auto sink_pad = gst::element_get_static_pad(sink, "sink");
  if(!sink_pad) {
    fmt::print(stderr, "Failed to get sink pad: {}\n", sink_pad.error());
    return;
  }

  if(gst::pad_is_linked(*sink_pad)) {
    fmt::print(stdout, "Pad already linked, ignoring.\n");
    return;
  }

  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    fmt::print(stderr, "No caps on new pad: {}\n", caps.error());
    return;
  }

  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    fmt::print(stderr, "No structure in caps: {}\n", structure.error());
    return;
  }

  const auto pad_type = gst::structure_get_name(*structure);
  if(!pad_type.starts_with("audio/x-raw")) {
    fmt::print(stdout, "Pad type '{}' is not raw audio, ignoring.\n", pad_type);
    return;
  }

  if(auto link = gst::pad_link(new_pad, *sink_pad); !link) {
    fmt::print(stderr, "Failed to link pad of type '{}': {}\n", pad_type, link.error());
  } else {
    fmt::print(stdout, "Linked pad of type '{}'.\n", pad_type);
  }
}
}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <audio-file>\n", argv[0]);
    fmt::print(stderr, "Example: {} sample.ogg\n", argv[0]);
    return EXIT_FAILURE;
  }
  const char* filepath = argv[1];

  auto pipeline = gst::pipeline_new("audio-player");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::element_factory_make("filesrc", "file-source");
  if(!source) {
    fmt::print(stderr, "{}\n", source.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", filepath, nullptr);

  auto decode = gst::element_factory_make("decodebin", "decoder");
  if(!decode) {
    fmt::print(stderr, "{}\n", decode.error());
    return EXIT_FAILURE;
  }

  auto convert = gst::element_factory_make("audioconvert", "converter");
  if(!convert) {
    fmt::print(stderr, "{}\n", convert.error());
    return EXIT_FAILURE;
  }

  auto resample = gst::element_factory_make("audioresample", "resampler");
  if(!resample) {
    fmt::print(stderr, "{}\n", resample.error());
    return EXIT_FAILURE;
  }

  auto sink = gst::element_factory_make("autoaudiosink", "audio-output");
  if(!sink) {
    fmt::print(stderr, "{}\n", sink.error());
    return EXIT_FAILURE;
  }

  auto raw_source   = gst::bin_add(*pipeline, std::move(*source));
  auto raw_decode   = gst::bin_add(*pipeline, std::move(*decode));
  auto raw_convert  = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_resample = gst::bin_add(*pipeline, std::move(*resample));
  auto raw_sink     = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_decode || !raw_convert || !raw_resample || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_decode); !link) {
    fmt::print(stderr, "Failed to link source to decoder: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_convert, *raw_resample); !link) {
    fmt::print(stderr, "Failed to link converter to resampler: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_resample, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link resampler to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  // decodebin exposes pads dynamically; link the audio pad to the converter on pad-added.
  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(pad_added_handler), *raw_convert);

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    auto bus = gst::element_get_bus(*pipeline);
    if(bus) {
      auto err_msg = gst::bus_timed_pop_filtered(*bus, 0, gst::MessageType::Error);
      if(err_msg) {
        auto parsed = gst::message_parse_error(err_msg->get());
        if(parsed) {
          fmt::print(stderr, "Failed to start pipeline: {}\n", parsed->first);
          return EXIT_FAILURE;
        }
      }
    }
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto terminate = false;
  do {
    auto msg_result = gst::bus_timed_pop_filtered(
        *bus, GST_CLOCK_TIME_NONE, gst::MessageType::StateChanged | gst::MessageType::Error | gst::MessageType::EOS);

    if(!msg_result) {
      continue;
    }

    const auto& msg      = msg_result.value();
    const auto  msg_type = gst::message_type(msg);

    if(gst::MessageType::Error == msg_type) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::print(stderr, "Error: {}\nDebug: {}\n", parsed->first, parsed->second);
      }
      terminate = true;
    } else if(gst::MessageType::EOS == msg_type) {
      fmt::print(stdout, "End of stream reached.\n");
      terminate = true;
    } else if(gst::MessageType::StateChanged == msg_type) {
      if(GST_MESSAGE_SRC(msg.get()) == GST_OBJECT(pipeline->get())) {
        const auto [old_state, new_state, pending] = gst::message_parse_state_changed(msg);
        fmt::print(stdout, "Pipeline state: {} -> {}.\n", gst::state_get_name(old_state), gst::state_get_name(new_state));
      }
    }
  } while(!terminate);

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);

  return EXIT_SUCCESS;
}
