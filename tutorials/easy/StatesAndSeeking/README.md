# States and Seeking

## Problem

You need to pause/resume playback and jump to an arbitrary position in a real
video file, and know when those transitions have actually completed instead of
racing ahead. **Fix:** the NULL→READY→PAUSED→PLAYING state machine plus
`gst_element_seek_simple` with `GST_SEEK_FLAG_FLUSH`, syncing on
`ASYNC_DONE`/`gst_element_get_state`, driven interactively from the keyboard.

## New concept

The GStreamer state machine and seeking.

## States (in order)

    NULL → READY → PAUSED → PLAYING

Each transition is asynchronous for live elements. `gst_element_set_state` returns immediately;
the bus delivers `ASYNC_DONE` when the transition completes. `gst_element_get_state` blocks
until a requested state (or timeout) is reached.

## Seeking

`gst_element_seek_simple(element, format, flags, position)` sends a seek event upstream.
`GST_SEEK_FLAG_FLUSH` discards queued data so the seek takes effect immediately.
Seeks in this tutorial are relative (±5 s) and clamped to `[0, duration]` so they
never go past the start or end of the file.

## Pipeline

    filesrc → decodebin → videoconvert → autovideosink

`decodebin` links its output pad dynamically once it detects the stream format.

## Controls

| Key | Action |
|-----|--------|
| `←` | Seek −5 s |
| `→` | Seek +5 s |
| `Space` | Toggle PLAYING ⇄ PAUSED |
| `q` | Quit |

## How to run

    ./StatesAndSeeking <video-file>

## Expected output

Startup always prints the three state transitions:

    State: NULL → READY
    State: READY → PAUSED
    State: PAUSED → PLAYING
    ←/→ seek 5s · space pause/resume · q quit

Then, depending on what keys you press:

    Seeked to 7.000s
    State: PLAYING → PAUSED
    State: PAUSED → PLAYING
    Seeked to 2.000s
    End of stream reached.

## Exercises

1. Change the seek step from 5 s to 10 s — find the constant in the source and update it.
2. Print the current position before and after each PAUSED/PLAYING toggle using `gst_element_query_position`.
3. Replace `GST_SEEK_FLAG_FLUSH` with `GST_SEEK_FLAG_ACCURATE` and seek to a non-keyframe
   position — observe whether the seek lands on the exact frame or snaps to the nearest keyframe.
