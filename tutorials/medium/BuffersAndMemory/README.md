# Buffers and Memory

## New concept
`GstBuffer` is the fundamental data carrier in GStreamer. Each buffer has:
- **Memory blocks** (`GstMemory`): one or more contiguous memory segments.
- **Timestamps**: `PTS` (presentation), `DTS` (decode), `duration`.
- **Flags**: `DISCONT`, `DELTA_UNIT`, `DROPPABLE`, etc.

`gst_buffer_map` locks the buffer for CPU access. Always pair with `gst_buffer_unmap`.

## Pipeline

    videotestsrc → videoconvert → capsfilter(RGB 320×240) → autovideosink
                                                  ↑ pad probe inspects each buffer

## How to run
    ./BuffersAndMemory

## Expected output
    Buffer 0: size=230400 pts=0.000s dur=0.033s flags=0x00000010
    Buffer 1: size=230400 pts=0.033s dur=0.033s flags=0x00000000
    ...
    End of stream reached.

## Exercises
1. Print the number of memory blocks in each buffer with `gst_buffer_n_memory`.
2. Modify pixel data inside the probe by requesting `GST_MAP_READWRITE`.
3. Compare PTS values — what is the time step between consecutive frames?
