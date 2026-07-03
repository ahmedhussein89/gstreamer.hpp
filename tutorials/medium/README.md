# Medium Tutorials

These tutorials assume familiarity with the easy tutorials and introduce more
advanced GStreamer patterns: frame-level access, tee branching, registry introspection,
and multi-protocol sources.

## Tutorials

| Tutorial | What you learn |
|---|---|
| [EventsAndQueries](EventsAndQueries/) | `gst_element_query` for position/duration, `gst_element_send_event` for seeks |
| [BuffersAndMemory](BuffersAndMemory/) | `GstBuffer`, `GstMemory`, timestamps, `gst_buffer_map`/`unmap` |
| [ClocksAndSync](ClocksAndSync/) | Pipeline clock, `gst_pipeline_get_clock`, sync vs. no-sync rendering |
| [CPUVideoProcessing](CPUVideoProcessing/) | `appsink`/`appsrc`, pulling frames into C++, pushing modified frames back |
| [ImageCapture](ImageCapture/) | Pad probes, intercepting a single frame, writing PPM |
| [DynamicPipeline](DynamicPipeline/) | Adding/removing branches while PLAYING, `BLOCK_DOWNSTREAM` probes |
| [PipelineInspector](PipelineInspector/) | GStreamer registry, element discovery, pad templates |
| [TagsAndMetadata](TagsAndMetadata/) | `GstTagList`, bus `TAG` messages, `GstDiscoverer` |
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
EventsAndQueries    — queries and events: synchronous pipeline interrogation
        ↓
BuffersAndMemory    — buffers: the data carrier, timestamps, memory access
        ↓
ClocksAndSync       — clocks: timing, sync mode, running time
        ↓
CPUVideoProcessing  — appsink/appsrc: per-frame C++ access
        ↓
ImageCapture        — pad probes: non-invasive frame interception
        ↓
DynamicPipeline     — live topology changes: add/remove branches while PLAYING
        ↓
PipelineInspector   — registry API: discover what elements are available
        ↓
TagsAndMetadata     — tags: media metadata via bus messages
        ↓
RTSPClient          — network source, chained dynamic pads
        ↓
VideoRecorder       — branching with tee, encoding, file writing
```
