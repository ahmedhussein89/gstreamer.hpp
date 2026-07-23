# Video File Player

## Problem

Play back a video file with the right decode chain and shut down cleanly whether it
finishes normally or hits a decode error. **Fix:** manual source → decoder → sink
pipeline with a bus-polling loop that reacts to `EOS`/`ERROR` messages explicitly.

## Goal

Learn sources, decoders, sinks.

## Concepts

- filesrc
- decodebin
- autovideosink
- Bus messages
- EOS handling

## Example Flow

```
MP4 File
   ↓
decodebin
   ↓
autovideosink
```
