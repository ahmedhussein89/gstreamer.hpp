# Dynamic Pipeline

## New concept
Add and remove pipeline branches **while PLAYING** using pad probes for safe blocking.

The sequence for adding a branch:
1. Request a new `src_%u` pad from `tee`.
2. Install a `BLOCK_DOWNSTREAM` probe on that pad (streaming stops at that point).
3. Inside the probe (or after blocking): create new elements, add to bin, link, sync state.
4. Remove the probe → streaming resumes through the new branch.

Removing a branch is the reverse: block the tee pad, unlink and set elements to NULL,
remove from bin, release the tee request pad.

## Pipeline

    videotestsrc → videoconvert → tee ──→ queue-a → autovideosink   (always on)
                                      └──→ queue-b → fakesink         (added at t=1s, removed at t=3s)

## How to run
    ./DynamicPipeline

## Expected output
    Pipeline running...
    [t=1s] Adding second branch...
    [t=1s] Second branch active.
    [t=3s] Removing second branch...
    [t=3s] Second branch removed.
    End of stream reached.

## Exercises
1. Replace `fakesink` with `autovideosink` to see both sinks active simultaneously.
2. Add an `x264enc → mp4mux → filesink` recording branch dynamically (requires gst-plugins-ugly).
3. Remove the branch immediately after adding it — verify the pipeline stays stable.
