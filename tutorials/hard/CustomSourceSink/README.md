# Custom Source & Sink

**Only 2 tracks (C / `gst::` view), not the usual 3** — these elements are `GObject`
subclasses (`GstBaseSrc`/`GstBaseSink`), registered once at process start; there is no
owning-vs-non-owning distinction to demonstrate with a separate RAII track, so no
`main_raii.cpp` is provided.

## New concept
Writing your own `GstBaseSrc` and `GstBaseSink` subclasses in plain C, and registering
them at runtime with `gst_element_register()` — no plugin `.so`, no `GST_PLUGIN_PATH`,
just a function call before the elements are used.

- `mysrc` (`GstBaseSrc`): negotiates GRAY8 video caps (`fixate`/`set_caps`), then emits a
  bounded number of buffers (`fill`) each filled with a single repeating byte equal to
  `buffers_sent % 256` — a trivial counting pattern — before returning `GST_FLOW_EOS`.
  `num-buffers` defaults to 100; `-1` means "run forever", matching `videotestsrc`.
- **Timestamping is the subclass's job.** `GstBaseSrc` does not set PTS for you unless
  `do-timestamp` is enabled, so `fill` derives `GST_BUFFER_PTS`/`GST_BUFFER_DURATION` from
  the negotiated framerate. Drop those two lines and the run finishes instantly instead of
  taking `num-buffers / framerate` seconds — `GstBaseSink` syncs on the clock by default,
  and untimestamped buffers are rendered as fast as they arrive.
- `mysink` (`GstBaseSink`): in `render`, sums every byte of every buffer into a running
  additive checksum and counts buffers; both are printed in `stop`.

## Pipeline

    mysrc (num-buffers=50) → mysink

## How to run
    ./CustomSourceSink
    ./CustomSourceSinkView

## Expected output
    Pipeline running (50 buffers)...
    mysink: received 50 buffers, checksum=...
    End of stream reached.

(the checksum value depends on the negotiated frame size, which is fixed by `mysrc`'s
`width`/`height` properties — default 64x64 — so it is deterministic across runs)

## Exercises
1. Change `mysrc`'s `width`/`height` properties (via `g_object_set`) and confirm the
   checksum changes accordingly.
2. Make the counting pattern a gradient instead of a flat fill (e.g. `column % 256`
   per row) inside `my_src_fill`.
3. Add a `num-buffers` cap to `mysink` too, and have it post an `EOS`-like custom bus
   message once it is reached, instead of relying solely on the source's EOS.
