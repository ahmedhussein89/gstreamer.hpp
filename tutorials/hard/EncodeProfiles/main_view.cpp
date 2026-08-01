#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include <gst/pbutils/encoding-profile.h>

#include "gstreamer.hpp"

namespace {
constexpr auto NumBuffers = 150;
}

namespace {
// Build a container profile (Ogg) holding a single Theora video stream.
nonstd::expected<gst::EncodingContainerProfilePtr, std::string> build_video_profile() {
  auto container_caps = gst::caps_from_string("application/ogg");
  if(!container_caps) {
    return nonstd::make_unexpected(container_caps.error());
  }

  auto container = gst::encoding_container_profile_new("ogg-theora", "Ogg/Theora profile", container_caps->get());
  if(!container) {
    return nonstd::make_unexpected(container.error());
  }

  auto video_caps = gst::caps_from_string("video/x-theora");
  if(!video_caps) {
    return nonstd::make_unexpected(video_caps.error());
  }

  auto video_profile = gst::encoding_video_profile_new(video_caps->get(), {}, nullptr, 1);
  if(!video_profile) {
    return nonstd::make_unexpected(video_profile.error());
  }

  // encoding_container_profile_add_profile takes ownership of the video profile on success
  // (and on failure — the C API still consumes the ref either way).
  if(auto added = gst::encoding_container_profile_add_profile(*container, std::move(*video_profile)); !added) {
    return nonstd::make_unexpected(added.error());
  }

  return std::move(*container);
}

}  // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view output_path = (argc > 1) ? argv[1] : "output.ogv";

  auto pipeline = gst::pipeline_new("encode-profiles");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source    = gst::element_factory_make("videotestsrc", "source");
  auto convert   = gst::element_factory_make("videoconvert", "convert");
  auto encodebin = gst::element_factory_make("encodebin", "encodebin");
  auto filesink  = gst::element_factory_make("filesink", "filesink");

  if(!source || !convert || !encodebin || !filesink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-base/good ship theoraenc/oggmux.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(filesink->get()), "location", output_path.data(), nullptr);

  auto profile = build_video_profile();
  if(!profile) {
    fmt::print(stderr, "Failed to build encoding profile: {}\n", profile.error());
    return EXIT_FAILURE;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(encodebin->get()), "profile", profile->get(), nullptr);
  // *profile destructor calls gst_encoding_profile_unref; encodebin's "profile" setter takes its own ref.

  auto raw_source    = gst::bin_add(*pipeline, std::move(*source));
  auto raw_convert   = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_encodebin = gst::bin_add(*pipeline, std::move(*encodebin));
  auto raw_filesink  = gst::bin_add(*pipeline, std::move(*filesink));

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

  auto bus = gst::element_get_bus(*pipeline);
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

  return EXIT_SUCCESS;
}
