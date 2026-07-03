#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {
constexpr auto NumBuffers = 300;
}

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view output_path = (argc > 1) ? argv[1] : "output.mp4";

  auto pipeline = gst::pipeline_new("video-recorder");
  if(!pipeline) {
    fmt::print(stderr, "Failed to create pipeline: {}\n", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source   = gst::element_factory_make("videotestsrc", "source");
  auto convert  = gst::element_factory_make("videoconvert", "convert");
  auto tee      = gst::element_factory_make("tee", "tee");
  auto disp_q   = gst::element_factory_make("queue", "display-queue");
  auto display  = gst::element_factory_make("autovideosink", "display");
  auto rec_q    = gst::element_factory_make("queue", "record-queue");
  auto encoder  = gst::element_factory_make("x264enc", "encoder");
  auto muxer    = gst::element_factory_make("mp4mux", "muxer");
  auto filesink = gst::element_factory_make("filesink", "filesink");

  if(!source || !convert || !tee || !disp_q || !display || !rec_q || !encoder || !muxer || !filesink) {
    fmt::print(stderr, "Failed to create elements. Ensure gst-plugins-ugly is installed (x264enc).\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(filesink->get()), "location", output_path.data(), nullptr);

  auto raw_source   = gst::bin_add(*pipeline, std::move(*source));
  auto raw_convert  = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_tee      = gst::bin_add(*pipeline, std::move(*tee));
  auto raw_disp_q   = gst::bin_add(*pipeline, std::move(*disp_q));
  auto raw_display  = gst::bin_add(*pipeline, std::move(*display));
  auto raw_rec_q    = gst::bin_add(*pipeline, std::move(*rec_q));
  auto raw_encoder  = gst::bin_add(*pipeline, std::move(*encoder));
  auto raw_muxer    = gst::bin_add(*pipeline, std::move(*muxer));
  auto raw_filesink = gst::bin_add(*pipeline, std::move(*filesink));

  if(!raw_source || !raw_convert || !raw_tee || !raw_disp_q || !raw_display ||
     !raw_rec_q || !raw_encoder || !raw_muxer || !raw_filesink) {
    fmt::print(stderr, "Failed to add elements to pipeline.\n");
    return EXIT_FAILURE;
  }

  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::print(stderr, "Failed to link source to convert: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_tee); !link) {
    fmt::print(stderr, "Failed to link convert to tee: {}\n", link.error());
    return EXIT_FAILURE;
  }

  // tee request pads have no gst:: wrapper — use raw GStreamer API
  GstPad* tee_disp = gst_element_request_pad_simple(*raw_tee, "src_%u");
  GstPad* q_disp   = gst_element_get_static_pad(*raw_disp_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_disp, q_disp)) {
    fmt::print(stderr, "Failed to link tee to display queue.\n");
    return EXIT_FAILURE;
  }
  gst_object_unref(q_disp);

  GstPad* tee_rec = gst_element_request_pad_simple(*raw_tee, "src_%u");
  GstPad* q_rec   = gst_element_get_static_pad(*raw_rec_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_rec, q_rec)) {
    fmt::print(stderr, "Failed to link tee to record queue.\n");
    return EXIT_FAILURE;
  }
  gst_object_unref(q_rec);

  if(auto link = gst::element_link(*raw_disp_q, *raw_display); !link) {
    fmt::print(stderr, "Failed to link display branch: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_rec_q, *raw_encoder); !link) {
    fmt::print(stderr, "Failed to link record queue to encoder: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_encoder, *raw_muxer); !link) {
    fmt::print(stderr, "Failed to link encoder to muxer: {}\n", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_muxer, *raw_filesink); !link) {
    fmt::print(stderr, "Failed to link muxer to filesink: {}\n", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    gst_element_release_request_pad(*raw_tee, tee_disp);
    gst_element_release_request_pad(*raw_tee, tee_rec);
    gst_object_unref(tee_disp);
    gst_object_unref(tee_rec);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Recording {} frames to '{}'.\n", NumBuffers, output_path);

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
      fmt::print(stdout, "Recording complete.\n");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  gst_element_release_request_pad(*raw_tee, tee_disp);
  gst_element_release_request_pad(*raw_tee, tee_rec);
  gst_object_unref(tee_disp);
  gst_object_unref(tee_rec);

  return EXIT_SUCCESS;
}
