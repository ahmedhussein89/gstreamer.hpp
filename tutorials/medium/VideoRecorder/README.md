# Video Recorder

## Goal

Split a video stream into two branches simultaneously: display it on screen and
encode it to an MP4 file, using a `tee` element and request pads.

## Concepts

- `tee` — fan-out element that duplicates a stream to N branches
- Request pads (`src_%u`) — pads that must be explicitly requested and released
- `queue` — decouples branches to avoid blocking between display and encoding
- `x264enc` — H.264 software encoder (requires `gst-plugins-ugly`)
- `mp4mux` — MP4 container muxer
- `filesink` — writes a bytestream to disk

## Pipeline

```text
videotestsrc (300 frames)
      ↓
 videoconvert
      ↓
    tee
   ↙    ↘
queue   queue
  ↓       ↓
display  x264enc
           ↓
         mp4mux
           ↓
         filesink (output.mp4)
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `VideoRecorder` | `main.cpp` | Imperative GStreamer C API (videotestsrc, 300 frames) |
| `VideoRecorderRAII` | `main_raii.cpp` | `gst::` RAII wrappers (videotestsrc, 300 frames) |
| `VideoRecorderWebcam` | `main_webcam.cpp` | `gst::` RAII wrappers + real webcam (v4l2src, runs until Ctrl+C) |

## Running

```bash
# Test source variants (300 frames ≈ 10 s at 30 fps)
./build/tutorials/medium/VideoRecorder                  # writes output.mp4
./build/tutorials/medium/VideoRecorder my_recording.mp4
./build/tutorials/medium/VideoRecorderRAII

# Webcam variant (press Ctrl+C to stop)
./build/tutorials/medium/VideoRecorderWebcam                          # /dev/video0 → output.mp4
./build/tutorials/medium/VideoRecorderWebcam /dev/video1              # different camera
./build/tutorials/medium/VideoRecorderWebcam /dev/video0 capture.mp4  # custom output path
```

## Prerequisites

`x264enc` ships in `gst-plugins-ugly`. Install it on Debian/Ubuntu:

```bash
sudo apt install gstreamer1.0-plugins-ugly
```

## Key Ideas

`tee` pads are *request pads*: you must call `gst_element_request_pad_simple(tee, "src_%u")`
to get one, and `gst_element_release_request_pad` to free it before unreffing the
pipeline. Forgetting to release request pads causes reference-count leaks.

`queue` elements between `tee` and each downstream branch are essential for
real-time pipelines — they decouple thread scheduling so a slow encoder cannot
block the display branch.
