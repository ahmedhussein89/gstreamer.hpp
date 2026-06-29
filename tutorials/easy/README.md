# Easy Tutorials

These tutorials introduce GStreamer and the `gst::` C++ wrappers step by step.
Each concept builds on the previous one, so reading them in order is recommended.

## Tutorials

| Tutorial | What you learn |
|---|---|
| [HelloWorld](HelloWorld/) | `gst_parse_launch`, pipeline states, main loop |
| [PipelineBuilder](PipelineBuilder/) | Runtime pipeline strings, `gst::parse_launch` |
| [VideoFilePlayer](VideoFilePlayer/) | `filesrc`, `decodebin`, dynamic pad linking, EOS |
| [AudioPlayer](AudioPlayer/) | `audiotestsrc`, `audioconvert`, `audioresample`, `autoaudiosink` |
| [WebcamViewer](WebcamViewer/) | `v4l2src`, live capture, `videoconvert` |

## Variants

Every tutorial ships at least two source files:

| Suffix | Approach |
|---|---|
| `main.cpp` | Imperative GStreamer C API |
| `main_raii.cpp` | `gst::` RAII wrappers (`gstreamer.hpp`) |
| `main_declarative.cpp` | `gst::PipelineDesc` / `gst::build()` DSL |
| `main_dynamic.cpp` | Dynamic pad handling with `gst::` wrappers |

Not every tutorial has all four variants — check each subdirectory.

## Building

From the repository root:

```bash
cmake -B build -S . -DDS_BUILD_TUTORIALS=ON
cmake --build build
```

Binaries land in `build/tutorials/easy/`.

## Progression

```text
HelloWorld        — one-liner pipeline, no state management
     ↓
PipelineBuilder   — same idea, runtime string, RAII wrappers
     ↓
VideoFilePlayer   — file input, decoding, dynamic pads, EOS
     ↓
AudioPlayer       — audio path, format conversion, resampling
     ↓
WebcamViewer      — live source, continuous stream
```

After completing these, move on to the [medium tutorials](../medium/).
