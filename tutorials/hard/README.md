# Hard Tutorials

These tutorials assume familiarity with the easy and medium tutorials and cover
advanced, less-common GStreamer scenarios: multi-stream compositing, custom
GObject elements, unit testing pipelines, RTSP serving, and network clock
synchronization.

## Tutorials

| Tutorial | What you learn |
|---|---|
| [MultiCameraViewer](MultiCameraViewer/) | `compositor`, multiple sources muxed into one output, per-pad geometry |
| [EncodeProfiles](EncodeProfiles/) | `encodebin`, `GstEncodingProfile`, container/codec selection at runtime |
| [CustomPlugin](CustomPlugin/) | Writing a custom `GstElement` subclass, registering it, using it in a pipeline |
| [CustomSourceSink](CustomSourceSink/) | Custom `GstBaseSrc`/`GstBaseSink` subclasses for app-defined I/O |
| [TestingElements](TestingElements/) | `gstcheck` harness, unit-testing elements and pipelines |
| [RTSPServer](RTSPServer/) | `gst-rtsp-server`, serving a live pipeline over RTSP |
| [NetClockSync](NetClockSync/) | `GstNetTimeProvider`/`gst_net_client_clock`, synchronizing clocks across processes |

## Variants

Most tutorials ship two source files:

| Suffix | Approach |
|---|---|
| `main.cpp` | Imperative GStreamer C API |
| `main_raii.cpp` | `gst::` RAII wrappers (`gstreamer.hpp`) |

`CustomPlugin`, `CustomSourceSink`, and `TestingElements` ship only two tracks,
`main.cpp` and `main_view.cpp`, instead: a `GstElement`/`GstBaseSrc`/`GstBaseSink`
subclass is a GObject type registered with the GStreamer plugin system, and has
no meaningful RAII-vs-view distinction to wrap.

## Building

```bash
cmake -B build -S . -DGST_BUILD_TUTORIALS=ON
cmake --build build
```

Binaries land in `build/tutorials/hard/`.

## Prerequisites

- [RTSPServer] requires `gst-rtsp-server` (`GStreamer::RtspServer`).
- [TestingElements] requires `gstcheck` (`GStreamer::Check`).
- [NetClockSync] requires `gstnet` (`GStreamer::Net`).
- [CustomPlugin], [CustomSourceSink] require `gstbase` (`GStreamer::Base`).
- [EncodeProfiles] requires `pbutils` from `gst-plugins-base` (`GStreamer::Pbutils`).
- These components are optional at configure time; a container image missing
  one of them skips the affected tutorial rather than failing configure.

## Progression

```text
MultiCameraViewer   — compositor: combining multiple live sources
        ↓
EncodeProfiles      — encodebin: runtime-selectable encode/mux profiles
        ↓
CustomPlugin        — writing your own GstElement
        ↓
CustomSourceSink    — writing your own GstBaseSrc/GstBaseSink
        ↓
TestingElements      — unit-testing elements with gstcheck
        ↓
RTSPServer          — serving a pipeline over the network
        ↓
NetClockSync        — synchronizing clocks across processes
```
