# CPU Video Processing

## Goal

Pull raw video frames from a GStreamer pipeline into C++, manipulate pixel data on
the CPU, then push the processed frames back into a second pipeline for display.

## Concepts

- `appsink` — pull buffers from a pipeline into application code
- `appsrc` — push application-generated or modified buffers into a pipeline
- `GstMapInfo` — memory-map a buffer for read/write access
- Buffer copy — modifying a frame without corrupting in-flight data
- Caps negotiation — constraining format to `video/x-raw,format=RGB`

## Pipeline

```text
videotestsrc (90 frames)
      ↓
 videoconvert
      ↓
   appsink  ──(new-sample signal)──▶  [C++: draw red border]
                                              ↓
                                           appsrc
                                              ↓
                                        videoconvert
                                              ↓
                                        autovideosink
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `CPUVideoProcessing` | `main.cpp` | Imperative GStreamer C API |
| `CPUVideoProcessingRAII` | `main_raii.cpp` | `gst::` RAII wrappers |

## Running

```bash
./build/tutorials/medium/CPUVideoProcessing
./build/tutorials/medium/CPUVideoProcessingRAII
```

Processes 90 frames (≈3 seconds at 30 fps), displays a red border overlay on each
frame, then exits when EOS is received.

## Key Ideas

The `new-sample` signal fires on `appsink` for every decoded frame. The callback:

1. Pulls the sample with `pull-sample`.
2. Copies the buffer (never mutate a buffer you don't own).
3. Maps the copy for `READWRITE` access via `GstMapInfo`.
4. Writes RGB pixel data directly.
5. Unmaps and pushes the result to `appsrc` via `push-buffer`.

This pattern is the basis for any CPU-side image processing step: blending overlays,
running OpenCV, writing custom detectors, etc.
