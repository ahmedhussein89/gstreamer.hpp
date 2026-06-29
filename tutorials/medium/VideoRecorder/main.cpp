#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {
constexpr auto NumBuffers = 300;
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  const char* output_path = (argc > 1) ? argv[1] : "output.mp4";

  auto* pipeline = gst_pipeline_new("video-recorder");
  auto* source   = gst_element_factory_make("videotestsrc", "source");
  auto* convert  = gst_element_factory_make("videoconvert", "convert");
  auto* tee      = gst_element_factory_make("tee", "tee");
  auto* disp_q   = gst_element_factory_make("queue", "display-queue");
  auto* display  = gst_element_factory_make("autovideosink", "display");
  auto* rec_q    = gst_element_factory_make("queue", "record-queue");
  auto* encoder  = gst_element_factory_make("x264enc", "encoder");
  auto* muxer    = gst_element_factory_make("mp4mux", "muxer");
  auto* filesink = gst_element_factory_make("filesink", "filesink");

  if(!pipeline || !source || !convert || !tee || !disp_q || !display ||
     !rec_q || !encoder || !muxer || !filesink) {
    fmt::println(stderr, "Failed to create elements. Ensure gst-plugins-ugly is installed (x264enc).");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "num-buffers", NumBuffers, nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(filesink), "location", output_path, nullptr);

  gst_bin_add_many(GST_BIN(pipeline), source, convert, tee,
                   disp_q, display, rec_q, encoder, muxer, filesink, nullptr);

  if(TRUE != gst_element_link_many(source, convert, tee, nullptr)) {
    fmt::println(stderr, "Failed to link source chain.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // tee uses request pads; request one for each branch
  GstPad* tee_disp = gst_element_request_pad_simple(tee, "src_%u");
  GstPad* q_disp   = gst_element_get_static_pad(disp_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_disp, q_disp)) {
    fmt::println(stderr, "Failed to link tee to display queue.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  gst_object_unref(q_disp);

  GstPad* tee_rec = gst_element_request_pad_simple(tee, "src_%u");
  GstPad* q_rec   = gst_element_get_static_pad(rec_q, "sink");
  if(GST_PAD_LINK_OK != gst_pad_link(tee_rec, q_rec)) {
    fmt::println(stderr, "Failed to link tee to record queue.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  gst_object_unref(q_rec);

  if(TRUE != gst_element_link(disp_q, display)) {
    fmt::println(stderr, "Failed to link display branch.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link_many(rec_q, encoder, muxer, filesink, nullptr)) {
    fmt::println(stderr, "Failed to link record branch.");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to start pipeline.");
    gst_element_release_request_pad(tee, tee_disp);
    gst_element_release_request_pad(tee, tee_rec);
    gst_object_unref(tee_disp);
    gst_object_unref(tee_rec);
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Recording {} frames to '{}'.", NumBuffers, output_path);

  auto* bus = gst_element_get_bus(pipeline);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE,
      static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  if(nullptr != msg) {
    if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
      GError* err = nullptr;
      gst_message_parse_error(msg, &err, nullptr);
      fmt::println(stderr, "Error: {}", err->message);
      g_error_free(err);
    } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
      fmt::println(stdout, "Recording complete.");
    }
    gst_message_unref(msg);
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_element_release_request_pad(tee, tee_disp);
  gst_element_release_request_pad(tee, tee_rec);
  gst_object_unref(tee_disp);
  gst_object_unref(tee_rec);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
