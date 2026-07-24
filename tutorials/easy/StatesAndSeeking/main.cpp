#include <algorithm>
#include <cstdlib>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <fmt/core.h>

#include <gst/gst.h>

namespace {

// decodebin exposes its decoded output pads only after it has examined the
// stream, so we link decodebin -> videoconvert dynamically from this callback.
void on_decodebin_pad_added(GstElement* /*decodebin*/, GstPad* new_pad, gpointer user_data) {
  auto* convert = static_cast<GstElement*>(user_data);
  GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
  if(TRUE == gst_pad_is_linked(sink_pad)) {
    gst_object_unref(sink_pad);
    return;
  }

  GstCaps* caps = gst_pad_get_current_caps(new_pad);
  GstStructure* structure = gst_caps_get_structure(caps, 0);
  const char* name = gst_structure_get_name(structure);
  gst_caps_unref(caps);

  if(TRUE != g_str_has_prefix(name, "video/x-raw")) {
    gst_object_unref(sink_pad);
    return;
  }

  if(GST_PAD_LINK_OK != gst_pad_link(new_pad, sink_pad)) {
    fmt::print(stderr, "Failed to link decoded pad to videoconvert.\n");
  }
  gst_object_unref(sink_pad);
}

// ponytail: POSIX-only termios; tutorials target Linux/WSL.
struct RawTerminal {
  termios original{};
  bool restore = false;

  RawTerminal() {
    if(0 == tcgetattr(STDIN_FILENO, &original)) {
      termios raw = original;
      raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
      if(0 == tcsetattr(STDIN_FILENO, TCSANOW, &raw)) {
        restore = true;
      }
    }
  }

  ~RawTerminal() {
    if(restore) {
      tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
  }
};

constexpr gint64 SeekStep = 5 * GST_SECOND;

// Relative seek: query position, clamp pos +/- SeekStep to [0, duration], flush-seek.
void seek_relative(GstElement* pipeline, gint64 delta) {
  gint64 pos = 0;
  if(TRUE != gst_element_query_position(pipeline, GST_FORMAT_TIME, &pos)) {
    fmt::print(stderr, "Failed to query position.\n");
    return;
  }

  gint64 dur = 0;
  if(TRUE != gst_element_query_duration(pipeline, GST_FORMAT_TIME, &dur)) {
    dur = pos + delta;    // best effort if duration is unavailable
  }

  const gint64 target = std::clamp(pos + delta, gint64{0}, dur);
  if(gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
         static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), target)) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(target) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
  }
}

}    // namespace

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  if(argc < 2) {
    fmt::print(stderr, "Usage: {} <video-file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  auto* pipeline = gst_pipeline_new("states-seeking");
  if(nullptr == pipeline) {
    fmt::print(stderr, "Failed to create pipeline.\n");
    return EXIT_FAILURE;
  }

  auto* source = gst_element_factory_make("filesrc", "source");
  auto* decode = gst_element_factory_make("decodebin", "decoder");
  auto* convert = gst_element_factory_make("videoconvert", "convert");
  auto* sink = gst_element_factory_make("autovideosink", "sink");

  if(!source || !decode || !convert || !sink) {
    fmt::print(stderr, "Failed to create elements.\n");
    return EXIT_FAILURE;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  g_object_set(G_OBJECT(source), "location", argv[1], nullptr);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
  gst_bin_add_many(GST_BIN(pipeline), source, decode, convert, sink, nullptr);

  // filesrc and decodebin both have static pads, so they link now; decodebin
  // and videoconvert are linked later from the pad-added callback.
  if(TRUE != gst_element_link(source, decode)) {
    fmt::print(stderr, "Failed to link source to decoder.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  if(TRUE != gst_element_link(convert, sink)) {
    fmt::print(stderr, "Failed to link convert to sink.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }

  g_signal_connect(decode, "pad-added", G_CALLBACK(on_decodebin_pad_added), convert);

  // Cycle through states explicitly to show each transition.
  gst_element_set_state(pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  gst_element_set_state(pipeline, GST_STATE_PAUSED);
  // Block until PAUSED is reached so the clock is assigned.
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: READY → PAUSED\n");

  if(GST_STATE_CHANGE_FAILURE == gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::print(stderr, "Failed to change pipeline state to PLAYING.\n");
    gst_object_unref(pipeline);
    return EXIT_FAILURE;
  }
  gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  fmt::print(stdout, "←/→ seek 5s · space pause/resume · q quit\n");

  auto* bus = gst_element_get_bus(pipeline);
  if(nullptr == bus) {
    fmt::print(stderr, "Failed to get bus from pipeline.\n");
    return EXIT_FAILURE;
  }

  RawTerminal raw_terminal;
  bool playing = true;
  bool running = true;
  while(running) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval timeout{0, 30'000};    // ~30ms
    if(select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout) > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
      // ponytail: one read() of up to 3 bytes captures the whole arrow escape
      // sequence in practice; good enough for a tutorial.
      char buf[3] = {};
      const ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
      if(n == 3 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == 'C') {
        seek_relative(pipeline, SeekStep);
      } else if(n == 3 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == 'D') {
        seek_relative(pipeline, -SeekStep);
      } else if(n >= 1 && buf[0] == ' ') {
        const GstState target = playing ? GST_STATE_PAUSED : GST_STATE_PLAYING;
        gst_element_set_state(pipeline, target);
        gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        playing = !playing;
        fmt::print(stdout, "State: {} → {}\n", playing ? "PAUSED" : "PLAYING", playing ? "PLAYING" : "PAUSED");
      } else if(n >= 1 && buf[0] == 'q') {
        running = false;
      }
    }

    auto* msg =
        gst_bus_timed_pop_filtered(bus, 0, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if(nullptr != msg) {
      if(GST_MESSAGE_ERROR == GST_MESSAGE_TYPE(msg)) {
        GError* error = nullptr;
        gst_message_parse_error(msg, &error, nullptr);
        fmt::print(stderr, "Error: {}\n", error->message);
        g_error_free(error);
        running = false;
      } else if(GST_MESSAGE_EOS == GST_MESSAGE_TYPE(msg)) {
        fmt::print(stdout, "End of stream reached.\n");
        running = false;
      }
      gst_message_unref(msg);
    }
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(bus);
  gst_object_unref(pipeline);

  return EXIT_SUCCESS;
}
