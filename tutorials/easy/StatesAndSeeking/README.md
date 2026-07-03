# States and Seeking

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

## Pipeline

    videotestsrc → videoconvert → autovideosink

## How to run
    ./StatesAndSeeking

## Expected output
    State: NULL → READY
    State: READY → PAUSED
    State: PAUSED → PLAYING
    Position: 0.000s
    Duration: unknown (videotestsrc has no fixed duration)
    Seeked to 2.000s
    End of stream reached.

## Exercises
1. Seek to 5 seconds — does videotestsrc reset its frame counter?
2. Pause the pipeline (PAUSED), sleep 500 ms, then resume (PLAYING). Print position before and after.
3. Replace `GST_SEEK_FLAG_FLUSH` with `GST_SEEK_FLAG_ACCURATE` — any visible difference?
