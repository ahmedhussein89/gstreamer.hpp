#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>
#include <gst/pbutils/encoding-profile.h>

namespace {
constexpr auto NumBuffers = 150;
}

namespace {
// Build a container profile (Ogg) holding a single Theora video stream.
// There is no gst:: wrapper for GstEncodingProfile — build it with the raw pbutils API.
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
  gst_init(&argc, &argv);

  const char* output_path = (argc > 1) ? argv[1] : "output.ogv";

  auto* pipeline  = gst_pipeline_new("encode-profiles");
  auto* source    = gst_element_factory_make("videotestsrc", "source");
  auto* convert   = gst_element_factory_make("videoconvert", "convert");
  auto* encodebin = gst_element_factory_make("encodebin", "encodebin");
  auto* filesink  = gst_element_factory_make("filesink", "filesink");

  if(!pipeline || !source || !convert || !encodebin || !filesink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-base/good ship theoraenc/oggmux.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(filesink), "location", output_path, nullptr);

  auto* profile = build_video_profile();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(encodebin), "profile", profile, nullptr);
  g_object_unref(profile);    // what gst_encoding_profile_unref expands to, minus its C-style cast

  gst_bin_add_many(GST_BIN(pipeline), source, convert, encodebin, filesink, nullptr);

  if(TRUE != gst_element_link(source, convert)) {
    fmt::print(stderr, "Failed to link source to convert.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // encodebin's sink pads ("video_%u"/"audio_%u") are request pads, and it will only hand one
  // out if the requester's caps match a stream profile. gst_element_link does exactly that
  // negotiation for us — requesting "video_%u" by name with no caps is refused outright.
  if(TRUE != gst_element_link(convert, encodebin)) {
    fmt::print(stderr, "Failed to link convert to encodebin.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(encodebin, filesink)) {
    fmt::print(stderr, "Failed to link encodebin to filesink.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Encoding {} frames to '{}' using an Ogg/Theora GstEncodingProfile.\n", NumBuffers, output_path);

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::print(stderr, "Error: {}\n", err->message);
      g_error_free(err);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::print(stdout, "Encoding complete.\n");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
