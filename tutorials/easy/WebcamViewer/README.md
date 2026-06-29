# Webcam Viewer

## Goal

Capture live video from a V4L2 webcam and display it on screen.

## Concepts

- `v4l2src` — Video4Linux2 camera source
- `videoconvert` — pixel-format negotiation
- `autovideosink` — automatic video display
- Live (non-finite) pipeline: no EOS, runs until error or Ctrl+C

## Pipeline

```text
v4l2src (/dev/video0)
        ↓
  videoconvert
        ↓
  autovideosink
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `WebcamViewer` | `main.cpp` | Imperative GStreamer C API |
| `WebcamViewerRAII` | `main_raii.cpp` | `gst::` RAII wrappers |

## Running

```bash
./build/tutorials/easy/WebcamViewer
./build/tutorials/easy/WebcamViewerRAII
```

The default device is `/dev/video0`. Edit the source to use a different device node
if your camera appears elsewhere (e.g. `/dev/video2`).

Press `Ctrl+C` to stop.

## What This Adds Over HelloWorld

HelloWorld uses `videotestsrc`, which is synthetic and always available. This tutorial
replaces the source with a real camera via `v4l2src`. A `videoconvert` element is
inserted between source and sink to handle any pixel-format mismatch between what the
camera outputs and what the sink expects — a common requirement with live capture.
