# Multi-Camera Viewer

## New concept

`compositor` mixes N live video streams into a single output frame. Each input
connects to a **request pad** (`sink_%u`) that must be requested explicitly
(and released afterward), and each request pad exposes `xpos`/`ypos`/`width`/
`height` properties controlling where that source is placed in the composite.
This is the same request-pad idea as `tee` in `VideoRecorder`, but here the
requested pads live on the *sink* side of a mixer instead of the *src* side of
a fan-out element, and they carry per-pad geometry properties.

Three `videotestsrc` elements (different `pattern` values) stand in for three
cameras so the tutorial runs headless with no real hardware.

## Pipeline

```text
videotestsrc(pattern=0) → videoconvert ─┐
videotestsrc(pattern=1) → videoconvert ─┼→ compositor → videoconvert → fakesink
videotestsrc(pattern=2) → videoconvert ─┘
```

Each `videoconvert → compositor` link lands on a request pad
(`gst_element_request_pad_simple(compositor, "sink_%u")`) with its
`xpos`/`ypos`/`width`/`height` set to place the source in a 2x2-ish grid
(640x480 output, 320x240 cells):

```text
+-----------+-----------+
| camera 0  | camera 1  |
| (smpte)   | (snow)    |
+-----------+-----------+
| camera 2  |           |
| (black)   |           |
+-----------+-----------+
```

## How to run

```bash
./build/tutorials/hard/MultiCameraViewer
./build/tutorials/hard/MultiCameraViewerView
./build/tutorials/hard/MultiCameraViewerRAII
```

Each source emits 150 buffers; the pipeline runs to EOS and exits on its own.

## Expected output

```text
Compositing 3 cameras into one output (150 frames each).
Composite stream finished.
```

## Exercises

1. Add a fourth `videotestsrc` and give it the bottom-right cell — request a
   fourth compositor sink pad and set its `xpos`/`ypos`.
2. Swap `fakesink` for `autovideosink` (drop `sync=false` implications by
   testing locally, not in CI) and watch the composited grid render live.
3. Replace one `videotestsrc` with a `v4l2src` from a real webcam and compare
   how `compositor` handles mismatched frame rates between a live camera and
   the synthetic sources.
