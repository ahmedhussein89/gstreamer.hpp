#include <atomic>
#include <cstdlib>
#include <cstring>

#include <termios.h>
#include <unistd.h>

#include <fmt/core.h>
#include <gst/gst.h>

namespace {

constexpr int FrameWidth  = 640;
constexpr int FrameHeight = 480;

struct AppState {
  std::atomic<bool> capture{false};
  std::atomic<int>  count{0};
};

// RAII guard: switches stdin to raw (non-buffered) mode so Space is delivered
// immediately instead of waiting for Enter.
struct TerminalRaw {
  termios saved{};

  TerminalRaw() {
    tcgetattr(STDIN_FILENO, &saved);
    termios raw      = saved;
    raw.c_lflag     &= ~static_cast<tcflag_t>(ICANON | ECHO);
    raw.c_cc[VMIN]   = 1;
    raw.c_cc[VTIME]  = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }

  ~TerminalRaw() { tcsetattr(STDIN_FILENO, TCSANOW, &saved); }
};

// Encode raw RGB pixels as JPEG using a one-shot GStreamer mini-pipeline:
//   appsrc → jpegenc → filesink
// This demonstrates that jpegenc is just another element — push one buffer
// through it and the JPEG lands on disk.
bool save_jpeg(const char* path, const guint8* pixels, int w, int h) {
  const std::string desc = fmt::format(
      "appsrc name=src "
      "caps=\"video/x-raw,format=RGB,width={},height={},framerate=0/1\" "
      "! jpegenc ! filesink location={}",
      w, h, path);

  GError* gerr     = nullptr;
  auto*   enc_pipe = gst_parse_launch(desc.c_str(), &gerr);
  if(nullptr == enc_pipe) {
    fmt::print(stderr, "JPEG pipeline error: {}\n", gerr->message);
    g_error_free(gerr);
    return false;
  }

  auto* src = gst_bin_get_by_name(GST_BIN(enc_pipe), "src");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(src, "format", GST_FORMAT_TIME, nullptr);
  gst_element_set_state(enc_pipe, GST_STATE_PLAYING);

  const auto size = static_cast<gsize>(w * h * 3);
  auto*      buf  = gst_buffer_new_allocate(nullptr, size, nullptr);
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_WRITE);
  std::memcpy(map.data, pixels, size);
  gst_buffer_unmap(buf, &map);
  GST_BUFFER_PTS(buf)      = 0;
  GST_BUFFER_DURATION(buf) = GST_SECOND;

  GstFlowReturn flow;
  g_signal_emit_by_name(src, "push-buffer", buf, &flow);
  gst_buffer_unref(buf);
  g_signal_emit_by_name(src, "end-of-stream", &flow);

  // Block until filesink has flushed the JPEG; bail after 5 s.
  auto* bus = gst_element_get_bus(enc_pipe);
  auto* msg = gst_bus_timed_pop_filtered(
      bus, 5 * GST_SECOND,
      static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
  const bool ok = (nullptr != msg && GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg));
  if(nullptr != msg) { gst_message_unref(msg); }
  gst_object_unref(bus);

  gst_element_set_state(enc_pipe, GST_STATE_NULL);
  gst_object_unref(src);
  gst_object_unref(enc_pipe);
  return ok;
}

// Pad-probe callback — runs on the streaming thread once per buffer.
// When capture is requested it maps the current buffer and saves a JPEG.
// Note: save_jpeg blocks the streaming thread briefly while jpegenc runs;
// this is acceptable here because encoding is fast (< 5 ms) and the
// tutorial favours clarity over zero-latency design.
GstPadProbeReturn on_buffer(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer user_data) {
  auto* state = static_cast<AppState*>(user_data);
  if(!state->capture.exchange(false)) { return GST_PAD_PROBE_OK; }

  const int  n    = ++state->count;
  const auto path = fmt::format("snapshot_{:03d}.jpg", n);

  GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_READ);
  const bool ok = save_jpeg(path.c_str(), map.data, FrameWidth, FrameHeight);
  gst_buffer_unmap(buf, &map);

  if(ok) {
    fmt::print(stdout, "Saved {}\n", path);
  } else {
    fmt::print(stderr, "Failed to save {}\n", path);
  }
  return GST_PAD_PROBE_OK;
}

// GLib IO-watch callback: reads one key from stdin each time it becomes ready.
gboolean on_stdin(GIOChannel* ch, GIOCondition /*cond*/, gpointer data) {
  auto* ctx = static_cast<std::pair<AppState*, GMainLoop*>*>(data);
  gchar key  = 0;
  gsize read = 0;
  g_io_channel_read_chars(ch, &key, 1, &read, nullptr);
  if(0 == read) { return TRUE; }

  if(' ' == key) {
    ctx->first->capture = true;
    fmt::print(stdout, "Capturing...\n");
  } else if('q' == key || '\x1b' == key) {
    g_main_loop_quit(ctx->second);
  }
  return TRUE;
}

gboolean on_bus(GstBus* /*bus*/, GstMessage* msg, gpointer user_data) {
  auto* loop = static_cast<GMainLoop*>(user_data);
  if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
    GError* err = nullptr;
    gst_message_parse_error(msg, &err, nullptr);
    fmt::print(stderr, "Pipeline error: {}\n", err->message);
    g_error_free(err);
    g_main_loop_quit(loop);
  } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
    g_main_loop_quit(loop);
  }
  return TRUE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  // Pipeline: source → capsfilter (RGB 640×480) → convert → display
  // The capsfilter pins the format so the probe always sees plain RGB pixels.
  auto* pipeline   = gst_pipeline_new("image-capture");
  auto* source     = gst_element_factory_make("videotestsrc",  "source");
  auto* capsfilter = gst_element_factory_make("capsfilter",    "cap");
  auto* convert    = gst_element_factory_make("videoconvert",  "convert");
  auto* sink       = gst_element_factory_make("autovideosink", "display");

  if(nullptr == pipeline || nullptr == source || nullptr == capsfilter ||
     nullptr == convert  || nullptr == sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  GstCaps* caps = gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      "width",  G_TYPE_INT,    FrameWidth,
      "height", G_TYPE_INT,    FrameHeight,
      nullptr);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(capsfilter, "caps", caps, nullptr);
  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, convert, sink, nullptr);

  if(TRUE != gst_element_link_many(source, capsfilter, convert, sink, nullptr)) {
    fmt::print(stderr, "Failed to link pipeline elements.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  // Install a buffer probe on the capsfilter src pad.
  AppState state;
  auto*    probe_pad = gst_element_get_static_pad(capsfilter, "src");
  gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
      on_buffer, &state, nullptr);
  gst_object_unref(probe_pad);

  auto* loop = g_main_loop_new(nullptr, FALSE);

  auto* bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, on_bus, loop);
  gst_object_unref(bus);

  TerminalRaw term;
  auto*       stdin_chan = g_io_channel_unix_new(STDIN_FILENO);
  auto        ctx        = std::make_pair(&state, loop);
  g_io_add_watch(stdin_chan, G_IO_IN, on_stdin, &ctx);
  g_io_channel_unref(stdin_chan);

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to start pipeline.\n");
    g_main_loop_unref(loop);
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  fmt::print(stdout, "Live video running. Press SPACE to capture JPEG, 'q'/Esc to quit.\n");
  g_main_loop_run(loop);

  gst_element_set_state(pipeline, GST_STATE_NULL);
  g_main_loop_unref(loop);
  gst_object_unref(pipeline);
  return EXIT_SUCCESS;
}
