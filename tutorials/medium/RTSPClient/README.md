# RTSP Client

## Goal

Connect to a live RTSP stream, decode the incoming video, and display it on screen.

## Concepts

- `rtspsrc` — RTSP/RTP source element
- Two-stage dynamic pad linking: `rtspsrc` → `decodebin` → `videoconvert`
- `pad-added` signal — fired when a new pad becomes available
- Caps inspection — selecting only the video stream from a multiplexed source
- `latency` property — tuning network jitter buffer

## Pipeline

```text
rtspsrc (RTSP URL)
    │
    └─(pad-added: media=video)──▶ decodebin
                                      │
                                      └─(pad-added: video/x-raw)──▶ videoconvert
                                                                           ↓
                                                                     autovideosink
```

Dynamic pads appear at two points:

1. `rtspsrc` adds a pad per media track (audio, video, …). The callback selects `media=video`.
2. `decodebin` adds a pad after decoding. The callback selects `video/x-raw`.

## Variants

| Binary | Source | Approach |
|---|---|---|
| `RTSPClient` | `main.cpp` | Imperative GStreamer C API |
| `RTSPClientRAII` | `main_raii.cpp` | `gst::` RAII wrappers |

## Running

```bash
./build/tutorials/medium/RTSPClient rtsp://example.com/stream
./build/tutorials/medium/RTSPClientRAII rtsp://example.com/stream
```

You need a reachable RTSP server. For local testing, VLC or `gst-rtsp-server`
can publish a stream.

Press `Ctrl+C` to stop.

## Relation to VideoFilePlayer

VideoFilePlayer (easy) uses a single `decodebin` dynamic pad because `filesrc`
delivers a clean demuxed bytestream. RTSP adds a second layer: `rtspsrc` itself
uses dynamic pads to separate media tracks before `decodebin` even sees the data.
This tutorial shows how to chain two `pad-added` handlers together.
