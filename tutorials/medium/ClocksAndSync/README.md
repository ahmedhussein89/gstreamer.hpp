# Clocks and Sync

## New concept
Every pipeline has a **clock** that provides a common time reference for all elements.
A **sink** element holds a buffer until its PTS matches the clock — this is *sync*.
Setting `sync=false` on a sink disables waiting and renders as fast as possible.

| Concept | API |
|---|---|
| Get pipeline clock | `gst_pipeline_get_clock` |
| Query current time | `gst_clock_get_time` |
| Query running time | `gst_element_get_current_clock_time` |
| Disable sync | `g_object_set(sink, "sync", FALSE, nullptr)` |

## Pipeline

    videotestsrc (num-buffers=120) → videoconvert → autovideosink

## How to run
    ./ClocksAndSync         # with sync (real-time)
    ./ClocksAndSync nosync  # with sync=false (as fast as possible)

## Expected output (sync mode)
    Clock type: GstSystemClock
    Clock time at start: 1234.567s
    End of stream reached.
    Elapsed wall time: ~4.000s (120 frames at 30 fps)

## Exercises
1. Run with and without `nosync` — how does elapsed time differ?
2. Force a specific clock with `gst_pipeline_use_clock(GST_PIPELINE(pipeline), nullptr)` (disable clock) — what happens?
3. Add `g_usleep` between frames via a pad probe and observe how sync=true compensates.
