# Image Capture

## Goal

Display a live video stream and save a JPEG snapshot whenever the user presses
Space, without stopping or modifying the stream.

## Concepts

- **Pad probes** — `gst_pad_add_probe` / `GST_PAD_PROBE_TYPE_BUFFER` intercept
  every buffer on a pad without restructuring the pipeline.
- **Snapshot** — an `atomic<bool>` flag set on keypress; the probe checks and
  clears it, then copies the current frame.
- **JPEG encoder** — a one-shot mini-pipeline `appsrc → jpegenc → filesink`
  encodes and writes the JPEG, showing that `jpegenc` is just another element.
- **GLib main loop** — `g_main_loop_run` drives the pipeline while a
  `GIOChannel` watch on stdin delivers keypresses without blocking the stream.
- **Raw terminal mode** — `termios` switches stdin to non-buffered mode so
  Space is delivered immediately.

## Pipeline

```text
videotestsrc
     ↓
capsfilter (RGB 640×480)  ← pad probe: on Space, copy buffer → save_jpeg()
     ↓
videoconvert
     ↓
autovideosink

save_jpeg() mini-pipeline (one-shot):
  appsrc → jpegenc → filesink
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `ImageCapture` | `main.cpp` | Imperative GStreamer C API |
| `ImageCaptureRAII` | `main_raii.cpp` | `gst::` RAII wrappers |

## Running

```bash
./build/tutorials/medium/ImageCapture
./build/tutorials/medium/ImageCaptureRAII
```

Press **Space** to save `snapshot_001.jpg`, `snapshot_002.jpg`, … in the
current directory.  Press **q** or **Esc** to quit.

## Key Ideas

The pad probe fires once per buffer and returns `GST_PAD_PROBE_OK`, so the
buffer flows downstream unchanged.  Only when the atomic flag is set does the
probe copy the pixel data and block the streaming thread briefly to run the
JPEG encoder.  Blocking the streaming thread is acceptable here because `jpegenc`
is fast (< 5 ms); for production use, offload encoding to a worker thread.

The `appsrc → jpegenc → filesink` mini-pipeline is independent of the display
pipeline and does not cause deadlocks.  It demonstrates that any GStreamer
element can be composed on demand — you do not need a monolithic pipeline
designed upfront.
