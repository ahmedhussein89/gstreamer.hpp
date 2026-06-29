#include <csignal>
#include <cstdlib>
#include <string_view>

#include <fmt/core.h>

#include "gstreamer.hpp"

// Pipeline:
//
//   v4l2src → videoconvert → tee ─→ queue → autovideosink
//                                └→ queue → x264enc → mp4mux → filesink
//
// Concepts: tee (fan-out), request pads, queue (branch decoupling),
//           videoconvert, x264enc, mp4mux, filesink, signal handling.

namespace {
volatile std::sig_atomic_t g_stop = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void on_sigint(int /*sig*/) {
  g_stop = 1;
}
}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  const std::string_view device      = (argc > 1) ? argv[1] : "/dev/video0";
  const std::string_view output_path = (argc > 2) ? argv[2] : "output.mp4";

  std::signal(SIGINT, on_sigint);

  auto pipeline = gst::pipeline_new("webcam-recorder");
  if(!pipeline) {
    fmt::println(stderr, "Failed to create pipeline: {}", pipeline.error());
    return EXIT_FAILURE;
  }

  auto source   = gst::element_factory_make("v4l2src", "source");
  auto convert  = gst::element_factory_make("videoconvert", "convert");
  auto tee      = gst::element_factory_make("tee", "tee");
  auto disp_q   = gst::element_factory_make("queue", "display-queue");
  auto display  = gst::element_factory_make("autovideosink", "display");
  auto rec_q    = gst::element_factory_make("queue", "record-queue");
  auto encoder  = gst::element_factory_make("x264enc", "encoder");
  auto muxer    = gst::element_factory_make("mp4mux", "muxer");
  auto filesink = gst::element_factory_make("filesink", "filesink");

  if(!source || !convert || !tee || !disp_q || !display || !rec_q || !encoder || !muxer || !filesink) {
    fmt::println(stderr, "Failed to create elements. Ensure gst-plugins-ugly (x264enc) and v4l2 are installed.");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source->get()), "device", device.data(), nullptr);
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
    fmt::println(stderr, "Failed to add elements to pipeline.");
    return EXIT_FAILURE;
  }

  // Linear chain: source → convert → tee
  if(auto link = gst::element_link(*raw_source, *raw_convert); !link) {
    fmt::println(stderr, "Failed to link source to convert: {}", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_convert, *raw_tee); !link) {
    fmt::println(stderr, "Failed to link convert to tee: {}", link.error());
    return EXIT_FAILURE;
  }

  // tee has request pads — must be explicitly requested and released.
  GstPad* tee_disp = gst_element_request_pad_simple(*raw_tee, "src_%u");
  GstPad* q_disp   = gst_element_get_static_pad(*raw_disp_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_disp, q_disp)) {
    fmt::println(stderr, "Failed to link tee to display queue.");
    return EXIT_FAILURE;
  }
  gst_object_unref(q_disp);

  GstPad* tee_rec = gst_element_request_pad_simple(*raw_tee, "src_%u");
  GstPad* q_rec   = gst_element_get_static_pad(*raw_rec_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_rec, q_rec)) {
    fmt::println(stderr, "Failed to link tee to record queue.");
    gst_object_unref(q_rec);
    return EXIT_FAILURE;
  }
  gst_object_unref(q_rec);

  // Display branch: disp_q → autovideosink
  if(auto link = gst::element_link(*raw_disp_q, *raw_display); !link) {
    fmt::println(stderr, "Failed to link display branch: {}", link.error());
    return EXIT_FAILURE;
  }

  // Record branch: rec_q → x264enc → mp4mux → filesink
  if(auto link = gst::element_link(*raw_rec_q, *raw_encoder); !link) {
    fmt::println(stderr, "Failed to link record queue to encoder: {}", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_encoder, *raw_muxer); !link) {
    fmt::println(stderr, "Failed to link encoder to muxer: {}", link.error());
    return EXIT_FAILURE;
  }
  if(auto link = gst::element_link(*raw_muxer, *raw_filesink); !link) {
    fmt::println(stderr, "Failed to link muxer to filesink: {}", link.error());
    return EXIT_FAILURE;
  }

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::println(stderr, "Failed to start pipeline: {}", state.error());
    gst_element_release_request_pad(*raw_tee, tee_disp);
    gst_element_release_request_pad(*raw_tee, tee_rec);
    gst_object_unref(tee_disp);
    gst_object_unref(tee_rec);
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Recording from '{}' to '{}'. Press Ctrl+C to stop.", device, output_path);

  auto bus = gst::element_get_bus(*pipeline);
  if(!bus) {
    fmt::println(stderr, "Failed to get bus: {}", bus.error());
    return EXIT_FAILURE;
  }

  // Poll the bus so SIGINT can break the loop.
  constexpr GstClockTime kPollInterval = 100 * GST_MSECOND;
  auto terminate = false;
  while(!terminate && !g_stop) {
    auto msg_result = gst::bus_timed_pop_filtered(
        *bus, kPollInterval, gst::MessageType::Error | gst::MessageType::EOS);

    if(!msg_result) {
      continue;
    }

    const auto& msg = msg_result.value();
    if(gst::MessageType::Error == gst::message_type(msg)) {
      auto parsed = gst::message_parse_error(msg.get());
      if(parsed) {
        fmt::println(stderr, "Error: {}\nDebug: {}", parsed->first, parsed->second);
      }
      terminate = true;
    } else if(gst::MessageType::EOS == gst::message_type(msg)) {
      fmt::println(stdout, "End of stream.");
      terminate = true;
    }
  }

  if(g_stop) {
    fmt::println(stdout, "Interrupted — sending EOS to finalize file.");
    gst_element_send_event(pipeline->get(), gst_event_new_eos());

    // Wait for EOS to propagate so mp4mux can write the moov atom.
    auto eos_result = gst::bus_timed_pop_filtered(*bus, 5 * GST_SECOND, gst::MessageType::EOS);
    if(!eos_result) {
      fmt::println(stderr, "Warning: EOS not received within 5 s; file may be incomplete.");
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  gst_element_release_request_pad(*raw_tee, tee_disp);
  gst_element_release_request_pad(*raw_tee, tee_rec);
  gst_object_unref(tee_disp);
  gst_object_unref(tee_rec);

  fmt::println(stdout, "Recording saved to '{}'.", output_path);
  return EXIT_SUCCESS;
}
