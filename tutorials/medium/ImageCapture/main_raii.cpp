#include <atomic>
#include <cstdlib>
#include <cstring>

#include <termios.h>
#include <unistd.h>

#include <fmt/core.h>

#include "gstreamer.hpp"

namespace {

constexpr int FrameWidth  = 640;
constexpr int FrameHeight = 480;

struct AppState {
  std::atomic<bool> capture{false};
  std::atomic<int>  count{0};
};

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

bool save_jpeg(const char* path, const guint8* pixels, int w, int h) {
  const std::string desc = fmt::format(
      "appsrc name=src "
      "caps=\"video/x-raw,format=RGB,width={},height={},framerate=0/1\" "
      "! jpegenc ! filesink location={}",
      w, h, path);

  auto enc_pipe = gst::parse_launch(desc);
  if(!enc_pipe) {
    fmt::println(stderr, "JPEG pipeline error.");
    return false;
  }

  auto* src = gst_bin_get_by_name(GST_BIN(enc_pipe->get()), "src");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(src, "format", GST_FORMAT_TIME, nullptr);
  std::ignore = gst::element_set_state(*enc_pipe, GST_STATE_PLAYING);

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

  bool ok = false;
  if(auto bus = gst::element_get_bus(*enc_pipe)) {
    auto msg = gst::bus_timed_pop_filtered(
        *bus, 5 * GST_SECOND,
        gst::MessageType::EOS | gst::MessageType::Error);
    ok = msg.has_value() && gst::MessageType::EOS == gst::message_type(*msg);
  }

  std::ignore = gst::element_set_state(*enc_pipe, GST_STATE_NULL);
  gst_object_unref(src);
  return ok;
}

GstPadProbeReturn on_buffer(GstPad* /*pad*/, GstPadProbeInfo* info, AppState* state) {
  if(!state->capture.exchange(false)) { return GST_PAD_PROBE_OK; }

  const int  n    = ++state->count;
  const auto path = fmt::format("snapshot_{:03d}.jpg", n);

  GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_READ);
  const bool ok = save_jpeg(path.c_str(), map.data, FrameWidth, FrameHeight);
  gst_buffer_unmap(buf, &map);

  if(ok) {
    fmt::println(stdout, "Saved {}", path);
  } else {
    fmt::println(stderr, "Failed to save {}", path);
  }
  return GST_PAD_PROBE_OK;
}

gboolean on_stdin(GIOChannel* ch, GIOCondition /*cond*/, gpointer data) {
  auto* ctx = static_cast<std::pair<AppState*, GMainLoop*>*>(data);
  gchar key  = 0;
  gsize read = 0;
  g_io_channel_read_chars(ch, &key, 1, &read, nullptr);
  if(0 == read) { return TRUE; }

  if(' ' == key) {
    ctx->first->capture = true;
    fmt::println(stdout, "Capturing...");
  } else if('q' == key || '\x1b' == key) {
    g_main_loop_quit(ctx->second);
  }
  return TRUE;
}

gboolean on_bus(GstBus* /*bus*/, GstMessage* msg, GMainLoop* loop) {
  if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
    auto parsed = gst::message_parse_error(msg);
    if(parsed) { fmt::println(stderr, "Pipeline error: {}", parsed->first); }
    g_main_loop_quit(loop);
  } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
    g_main_loop_quit(loop);
  }
  return TRUE;
}

}    // namespace

int main(int argc, char* argv[]) {
  gst::init(std::span(argv, static_cast<size_t>(argc)));

  auto pipeline   = gst::pipeline_new("image-capture");
  auto source     = gst::element_factory_make("videotestsrc",  "source");
  auto capsfilter = gst::element_factory_make("capsfilter",    "cap");
  auto convert    = gst::element_factory_make("videoconvert",  "convert");
  auto sink       = gst::element_factory_make("autovideosink", "display");

  if(!pipeline || !source || !capsfilter || !convert || !sink) {
    fmt::println(stderr, "Failed to create elements.");
    return EXIT_FAILURE;
  }

  auto caps = gst::caps_from_string(
      fmt::format("video/x-raw,format=RGB,width={},height={}", FrameWidth, FrameHeight));
  if(!caps) {
    fmt::println(stderr, "Failed to create caps: {}", caps.error());
    return EXIT_FAILURE;
  }

  auto raw_source     = gst::bin_add(*pipeline, std::move(*source));
  auto raw_capsfilter = gst::bin_add(*pipeline, std::move(*capsfilter));
  auto raw_convert    = gst::bin_add(*pipeline, std::move(*convert));
  auto raw_sink       = gst::bin_add(*pipeline, std::move(*sink));

  if(!raw_source || !raw_capsfilter || !raw_convert || !raw_sink) {
    fmt::println(stderr, "Failed to add elements to pipeline.");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(*raw_capsfilter, "caps", caps->get(), nullptr);

  if(auto r = gst::element_link(*raw_source, *raw_capsfilter); !r) {
    fmt::println(stderr, "Link source→cap: {}", r.error());
    return EXIT_FAILURE;
  }
  if(auto r = gst::element_link(*raw_capsfilter, *raw_convert); !r) {
    fmt::println(stderr, "Link cap→convert: {}", r.error());
    return EXIT_FAILURE;
  }
  if(auto r = gst::element_link(*raw_convert, *raw_sink); !r) {
    fmt::println(stderr, "Link convert→sink: {}", r.error());
    return EXIT_FAILURE;
  }

  AppState state;
  auto probe_pad = gst::element_get_static_pad(*raw_capsfilter, "src");
  if(!probe_pad) {
    fmt::println(stderr, "Failed to get probe pad: {}", probe_pad.error());
    return EXIT_FAILURE;
  }
  gst_pad_add_probe(probe_pad->get(), GST_PAD_PROBE_TYPE_BUFFER,
      reinterpret_cast<GstPadProbeCallback>(on_buffer), &state, nullptr);

  auto* loop = g_main_loop_new(nullptr, FALSE);

  if(auto bus = gst::element_get_bus(*pipeline)) {
    gst_bus_add_watch(bus->get(), reinterpret_cast<GstBusFunc>(on_bus), loop);
  }

  TerminalRaw term;
  auto*       stdin_chan = g_io_channel_unix_new(STDIN_FILENO);
  auto        ctx        = std::make_pair(&state, loop);
  g_io_add_watch(stdin_chan, G_IO_IN, on_stdin, &ctx);
  g_io_channel_unref(stdin_chan);

  if(auto r = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !r) {
    fmt::println(stderr, "Failed to start pipeline: {}", r.error());
    g_main_loop_unref(loop);
    return EXIT_FAILURE;
  }

  fmt::println(stdout, "Live video running. Press SPACE to capture JPEG, 'q'/Esc to quit.");
  g_main_loop_run(loop);

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  g_main_loop_unref(loop);
  return EXIT_SUCCESS;
}
