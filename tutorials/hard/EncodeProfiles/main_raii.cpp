#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include <gst/pbutils/encoding-profile.h>

#include "gstreamer_raii.hpp"

namespace {
constexpr auto NumBuffers = 150;
}

namespace {
// Build a container profile (Ogg) holding a single Theora video stream.
// GstEncodingProfile has no gst:: wrapper — build it with the raw pbutils API in all tracks.
GstEncodingProfile* build_video_profile() {
  auto* container_caps = gst_caps_from_string("application/ogg");
  auto* container = gst_encoding_container_profile_new("ogg-theora", "Ogg/Theora profile", container_caps, nullptr);
  gst_caps_unref(container_caps);

  auto* video_caps = gst_caps_from_string("video/x-theora");
  auto* video_profile = gst_encoding_video_profile_new(video_caps, nullptr, nullptr, 1);
  gst_caps_unref(video_caps);

  gst_encoding_container_profile_add_profile(container, GST_ENCODING_PROFILE(video_profile));

  return GST_ENCODING_PROFILE(container);
}

}  // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view output_path = (argc > 1) ? argv[1] : "output.ogv";

  auto pipeline = gst::raii::pipeline_new("encode-profiles");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source    = gst::raii::element_factory_make("videotestsrc", "source");
  auto convert   = gst::raii::element_factory_make("videoconvert", "convert");
  auto encodebin = gst::raii::element_factory_make("encodebin", "encodebin");
  auto filesink  = gst::raii::element_factory_make("filesink", "filesink");

  if(!source || !convert || !encodebin || !filesink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-base/good ship theoraenc/oggmux.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(filesink->get()), "location", output_path.data(), nullptr);

  auto* profile = build_video_profile();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(encodebin->get()), "profile", profile, nullptr);
  gst_encoding_profile_unref(profile);

  auto raw_source    = gst::raii::bin_add(*pipeline, std::move(*source));
  auto raw_convert   = gst::raii::bin_add(*pipeline, std::move(*convert));
  auto raw_encodebin = gst::raii::bin_add(*pipeline, std::move(*encodebin));
  auto raw_filesink  = gst::raii::bin_add(*pipeline, std::move(*filesink));

  if(!raw_source || !raw_convert || !raw_encodebin || !raw_filesink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link source to convert: {}\n", link.error());
    return EXIT_FAILURE;
  }

  // encodebin's sink pads ("video_%u"/"audio_%u") are request pads, and it will only hand one
  // out if the requester's caps match a stream profile. gst::element_link performs that
  // negotiation for us — requesting "video_%u" by name with no caps is refused outright.
  if(auto link = gst::element_link(*raw_convert, *raw_encodebin); !link) {
    fmt::print(stderr, "Failed to link convert to encodebin: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_encodebin, *raw_filesink); !link) {
    fmt::print(stderr, "Failed to link encodebin to filesink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Encoding {} frames to '{}' using an Ogg/Theora GstEncodingProfile.\n", NumBuffers, output_path);

  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
    return EXIT_FAILURE;
  }

  auto msg_result = gst::bus_timed_pop_filtered(*bus, GST_CLOCK_TIME_NONE, gst::MessageType::Error | gst::MessageType::EOS);
  if(msg_result) {
    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::print(stderr, "Error: {}\n", parsed->first);
      }
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::print(stdout, "Encoding complete.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline destructor calls gst_object_unref

  return EXIT_SUCCESS;
}
