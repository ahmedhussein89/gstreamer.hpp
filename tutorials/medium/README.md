# Medium Tutorials

These tutorials assume familiarity with the easy tutorials and introduce more
advanced GStreamer patterns: frame-level access, tee branching, registry introspection,
and multi-protocol sources.

## Tutorials

| Tutorial | What you learn |
|---|---|
| [CPUVideoProcessing](CPUVideoProcessing/) | `appsink`/`appsrc`, pulling frames into C++, pushing modified frames back |
| [ImageCapture](ImageCapture/) | Pad probes, intercepting a single frame, writing PPM |
| [PipelineInspector](PipelineInspector/) | GStreamer registry, element discovery, pad templates |
| [RTSPClient](RTSPClient/) | `rtspsrc`, two-stage dynamic pad linking, network buffering |
| [VideoRecorder](VideoRecorder/) | `tee`, request pads, H.264 encoding, MP4 muxing |

## Variants

Every tutorial ships two source files:

| Suffix | Approach |
|---|---|
| `main.cpp` | Imperative GStreamer C API |
| `main_raii.cpp` | `gst::` RAII wrappers (`gstreamer.hpp`) |

## Building

```bash
cmake -B build -S . -DDS_BUILD_TUTORIALS=ON
cmake --build build
```

Binaries land in `build/tutorials/medium/`.

## Prerequisites

- [VideoRecorder] requires `x264enc` from `gst-plugins-ugly`.
- [RTSPClient] requires a reachable RTSP endpoint.
- All other tutorials work with a standard GStreamer installation.

## Progression

```text
CPUVideoProcessing  — appsink/appsrc: per-frame C++ access
        ↓
ImageCapture        — pad probes: non-invasive frame interception
        ↓
PipelineInspector   — registry API: discover what elements are available
        ↓
RTSPClient          — network source, chained dynamic pads
        ↓
VideoRecorder       — branching with tee, encoding, file writing
```
