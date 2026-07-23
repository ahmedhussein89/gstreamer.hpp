# RTSP Server

## New concept

Serve a live stream over RTSP instead of consuming one. `GstRTSPServer` listens
for client connections; a `GstRTSPMediaFactory` builds a pipeline (from a
`gst_parse_launch`-style string) per client that connects to a mount point.
The factory's launch string must end in a payloader named `pay0` (here
`rtph264pay`) — that is how the server finds the RTP output to send.

The server is attached to the default `GMainContext` and driven by a
`GMainLoop`. Because this tutorial must terminate on its own in CI (no
display, no operator watching), a `g_timeout_add_seconds` callback quits the
loop after a fixed number of seconds instead of running forever.

This is the producing half of `tutorials/medium/RTSPClient/`: point that
tutorial's `rtspsrc` (or `gst-launch-1.0`) at the URL this server publishes.

## Pipeline

Server-side, per connected client, built internally by the media factory:

```text
videotestsrc
     ↓
  x264enc
     ↓
rtph264pay (pay0)  ──▶ RTP/UDP ──▶ client
```

Driver loop:

```text
gst_rtsp_server_new()
     │
     ├─ mount_points.add_factory("/test", factory)
     │
     └─ gst_rtsp_server_attach(NULL context)
              │
       g_main_loop_run(loop)
              │
     g_timeout_add_seconds(5s) ──▶ g_main_loop_quit
```

## Prerequisites

The media factory's launch string uses `x264enc`, which ships in **gst-plugins-ugly**. The
program checks for it at startup and exits non-zero if it is missing — a factory launch
string is not parsed until a client connects, so without that check a broken server would
run its full timeout and exit 0.

## How to run

```bash
./build/tutorials/hard/RTSPServer/RTSPServer
./build/tutorials/hard/RTSPServer/RTSPServerView
./build/tutorials/hard/RTSPServer/RTSPServerRAII
```

Each prints the mount URL, `rtsp://127.0.0.1:8554/test`, then serves for 5
seconds before quitting on its own. While it is running, point a client at
it:

```bash
gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/test ! decodebin ! autovideosink

# or the counterpart tutorial:
./build/tutorials/medium/RTSPClient/RTSPClient rtsp://127.0.0.1:8554/test
```

## Expected output

```text
Stream ready at rtsp://127.0.0.1:8554/test
Running for 5 seconds, then exiting on its own.
Timeout reached, stopping server.
```

## Exercises

1. Change the mount point from `/test` to `/camera` and update the launch
   string to use a real source (e.g. `v4l2src`) instead of `videotestsrc`.
2. Add a second `gst_rtsp_mount_points_add_factory` call for a second path
   (e.g. `/test2`) with a different launch string, and connect to both from
   two `RTSPClient` instances at once.
3. Set `gst_rtsp_media_factory_set_shared(factory, TRUE)` (already done here)
   to FALSE and observe that each connecting client now gets its own
   independent pipeline instance instead of sharing one.

## Wrapper gap

`GstRTSPServer` and `GstRTSPMediaFactory` have no `gst::`/`gst::raii::`
wrapper in this project — server, mount-points, and factory setup is raw
GStreamer C API in all three tracks (`main.cpp`, `main_view.cpp`,
`main_raii.cpp`). `main_view.cpp` and `main_raii.cpp` differ from `main.cpp`
only in using `gst::init` for initialization and wrapping the
`gst_rtsp_server_attach` call in a small `nonstd::expected`-returning helper,
matching the library's error-handling convention; there is no pipeline object
of our own to manage through `gst::raii::*`, since the media factory builds
and owns one internal pipeline per connecting client. Because of that, the
view and RAII tracks are near-identical to each other here — the meaningful
wrapper usage in this tutorial is in initialization and error propagation,
not pipeline construction.
