#include <fmt/printf.h>

#include "gstreamer_raii.hpp"

namespace {

// decodebin exposes its decoded output pads only after it has examined the
// stream, so we link decodebin -> videoconvert dynamically from this callback.
void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* convert = static_cast<GstElement*>(user_data);
  auto sink_pad = gst::element_get_static_pad(convert, "sink");
  if(!sink_pad) {
    return;
  }
  if(gst::pad_is_linked(*sink_pad)) {
    return;
  }

  auto caps = gst::pad_get_current_caps(new_pad);
  if(!caps) {
    return;
  }
  auto structure = gst::caps_get_structure(*caps);
  if(!structure) {
    return;
  }

  if(!gst::structure_get_name(*structure).starts_with("video/x-raw")) {
    return;
  }

  if(auto link = gst::pad_link(new_pad, *sink_pad); !link) {
    fmt::print(stderr, "Failed to link decoded pad: {}\n", link.error());
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
  auto pipeline = gst::raii::pipeline_new("video-player");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source = gst::raii::element_factory_make("filesrc", "source");
  auto decode = gst::raii::element_factory_make("decodebin", "decoder");
  auto convert = gst::raii::element_factory_make("videoconvert", "convert");
  auto sink = gst::raii::element_factory_make("autovideosink", "sink");

  if(!source || !decode || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "location", argv[1], nullptr);

  // raii::bin_add transfers ownership of each element into the pipeline bin and
  // returns a non-owning gst::Element handle for linking (bin now owns it).
  auto raw_source = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_decode = gst::raii::bin_add(*pipeline, std::move(*decode));
  auto raw_convert = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_sink = gst::raii::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_decode || !raw_convert || !raw_sink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  // filesrc and decodebin both have static pads, so they link now; decodebin
  // and videoconvert are linked later from the pad-added callback.
  if(auto link = gst::element_link(*raw_source, *raw_decode); !link) {
    fmt::print(stderr, "Failed to link source to decoder: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_convert, *raw_sink); !link) {
    fmt::print(stderr, "Failed to link convert to sink: {}\n", link.error());
    return EXIT_FAILURE;
  }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
  g_signal_connect(*raw_decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), *raw_convert);
#pragma clang diagnostic pop

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  // raii::element_get_bus returns an owning Bus (freed on scope exit)
  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto error_result = gst::message_parse_error(msg.get());
      if(error_result) {
        fmt::print(stderr, "Error: {}\n", error_result.value().first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "End of stream reached.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline goes out of scope here — gst::raii::Pipeline destructor
  // calls gst_object_unref, which in turn unrefs all contained elements.

  return EXIT_SUCCESS;
}
