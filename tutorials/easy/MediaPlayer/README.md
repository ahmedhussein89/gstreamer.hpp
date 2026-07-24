# Media Player

## Problem

You have a media file (MP4/MKV/…) carrying both an audio and a video stream and
want to play both. The container and codecs aren't known ahead of time, and
`decodebin` only exposes its decoded output pads *after* it inspects the stream.
**Fix:** one `pad-added` callback that routes each decoded pad to its own chain
by caps — video to `videoconvert → autovideosink`, audio to
`audioconvert → audioresample → autoaudiosink`.

## Goal

Learn dynamic pad linking for more than one stream: decoding a real file and
demuxing its audio and video branches from a single callback. Extends
`VideoFilePlayer` (which keeps only video) by keeping both streams.

## Concepts

- filesrc / decodebin
- Dynamic pad linking (audio **and** video), routed by caps
- videoconvert / audioconvert / audioresample
- autovideosink / autoaudiosink
- Bus messages, EOS handling

## Flow

```
Media File (MP4/MKV/…)
      ↓
  decodebin ──(pad-added, video/x-raw)──▶ videoconvert ──▶ autovideosink
      │
      └────────(pad-added, audio/x-raw)──▶ audioconvert ──▶ audioresample ──▶ autoaudiosink
```

## Running

Takes a media file (with audio and/or video) on the command line:

```bash
./MediaPlayer /path/to/media.mp4
```
