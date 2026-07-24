#include <algorithm>
#include <cstdlib>
#include <span>

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

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
void seek_relative(gst::Element pipeline, gint64 delta) {
  auto pos = gst::element_query_position(pipeline, GST_FORMAT_TIME);
  if(!pos) {
    fmt::print(stderr, "Failed to query position: {}\n", pos.error());
    return;
  }

  auto dur = gst::element_query_duration(pipeline, GST_FORMAT_TIME);
  const gint64 duration = dur ? dur.value() : (pos.value() + delta);    // best effort if unavailable

  const gint64 target = std::clamp(pos.value() + delta, gint64{0}, duration);
  auto seek = gst::element_seek_simple(pipeline, GST_FORMAT_TIME, gst::SeekFlags::Flush | gst::SeekFlags::KeyUnit, target);
  if(seek) {
    fmt::print(stdout, "Seeked to {:.3f}s\n", static_cast<double>(target) / GST_SECOND);
  } else {
    fmt::print(stdout, "Seek not supported by this source.\n");
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
  auto pipeline = gst::raii::pipeline_new("states-seeking");
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

  // Cycle through states explicitly to show each transition.
  std::ignore = gst::element_set_state(*pipeline, GST_STATE_READY);
  fmt::print(stdout, "State: NULL → READY\n");

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_PAUSED);
  std::ignore = gst::element_get_state(*pipeline);
  fmt::print(stdout, "State: READY → PAUSED\n");

  if(auto state = gst::element_set_state(*pipeline, GST_STATE_PLAYING); !state) {
    fmt::print(stderr, "Failed to start pipeline: {}\n", state.error());
    return EXIT_FAILURE;
  }
  std::ignore = gst::element_get_state(*pipeline);
  fmt::print(stdout, "State: PAUSED → PLAYING\n");

  fmt::print(stdout, "←/→ seek 5s · space pause/resume · q quit\n");

  // raii::element_get_bus returns an owning Bus (freed on scope exit)
  auto bus = gst::raii::element_get_bus(*pipeline);
  if(!bus) {
    fmt::print(stderr, "Failed to get bus: {}\n", bus.error());
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
        seek_relative(*pipeline, SeekStep);
      } else if(n == 3 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == 'D') {
        seek_relative(*pipeline, -SeekStep);
      } else if(n >= 1 && buf[0] == ' ') {
        const GstState target = playing ? GST_STATE_PAUSED : GST_STATE_PLAYING;
        std::ignore = gst::element_set_state(*pipeline, target);
        std::ignore = gst::element_get_state(*pipeline);
        playing = !playing;
        fmt::print(stdout, "State: {} → {}\n", playing ? "PAUSED" : "PLAYING", playing ? "PLAYING" : "PAUSED");
      } else if(n >= 1 && buf[0] == 'q') {
        running = false;
      }
    }

    auto msg_result = gst::bus_timed_pop_filtered(*bus, 0, gst::MessageType::Error | gst::MessageType::EOS);
    if(msg_result) {
      const auto& msg = msg_result.value();
      if(gst::MessageType::Error == gst::message_type(msg)) {
        auto error_result = gst::message_parse_error(msg.get());
        if(error_result) {
          fmt::print(stderr, "Error: {}\n", error_result.value().first);
        }
        running = false;
      } else if(gst::MessageType::EOS == gst::message_type(msg)) {
        fmt::print(stdout, "End of stream reached.\n");
        running = false;
      }
    }
  }

  std::ignore = gst::element_set_state(*pipeline, GST_STATE_NULL);
  // *pipeline goes out of scope here — gst::raii::Pipeline destructor
  // calls gst_object_unref, which in turn unrefs all contained elements.

  return EXIT_SUCCESS;
}
