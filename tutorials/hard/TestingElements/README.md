# Testing Elements

**Note:** this tutorial has only **2 tracks** (`TestingElements`, `TestingElementsView`), not the usual 3 —
this is element unit-testing with `GstHarness`, not pipeline building, so there is no meaningful
RAII-vs-non-owning-view distinction to demonstrate.

## New concept

Unit-testing a `GstElement` in isolation with `GstHarness` (`gst/check/gstharness.h`), instead of
building a full pipeline: `gst_harness_new()` wraps `myedgedetector` (from the `CustomPlugin`
tutorial) with fake src/sink pads, `gst_harness_set_src_caps_str()` negotiates `GRAY8` caps,
`gst_harness_push()`/`gst_harness_pull()` drive one buffer through the element, and the test
asserts on the exact output bytes computed from the element's own algorithm (horizontal-difference
threshold, see `myedgedetector.c`). `gst::` has no wrapper for `GstHarness` — it is a check-library
concept, not part of the enhanced element/pipeline/bus layer the crate covers — so
`main_view.cpp` uses `gst::init()` and nothing else from `gst::` before falling back to the same
raw harness calls as `main.cpp`.

The second half covers observing an element from the outside instead of asserting on it:
`GST_DEBUG` category/level filtering and the `leaks`/`latency` tracers via `GST_TRACERS`.

## Pipeline

    GstHarness fake src pad --push--> [ myedgedetector ] --pull--> GstHarness fake sink pad

`GstHarness` stands in for upstream/downstream elements: it owns a fake src pad linked to the
element's sink, and a fake sink pad linked to the element's src, so a single element can be
driven with hand-built buffers without assembling a real pipeline.

## How to run

    ./build/tutorials/hard/TestingElements/TestingElements
    ./build/tutorials/hard/TestingElements/TestingElementsView
    ctest --test-dir build -R TestingElements

With tracers and debug logging enabled:

    GST_DEBUG=myedgedetector:5 GST_TRACERS="leaks,latency" ./build/tutorials/hard/TestingElements/TestingElements

## Expected output

    All assertions passed for 'myedgedetector'.

Both binaries exit `0` on success and abort (non-zero exit, via a failed `assert`) if the output
bytes ever stop matching the element's algorithm — that abort is what `ctest --test-dir build -R
TestingElements` treats as a failing test.

With `GST_TRACERS="leaks,latency"` set, expect additional lines on stderr around teardown and
per-buffer flow, similar to:

    0:00:00.012345000 12345 0x5555... TRACE GST_TRACER :0:: latency, src=(string)source, sink=(string)detector, time=(guint64)123456, ...
    0:00:00.045678000 12345 0x5555... TRACE GST_TRACER :0:: leaks, type=(string)GstHarness, count=(uint)0, ...

(exact addresses/timestamps vary per run; the `leaks` tracer reports `count=0` outstanding
objects when the harness and buffers are torn down cleanly, which they are here).

## Exercises

1. Add a second `GstHarness` test buffer with an all-equal row (no edges) and assert the output
   is all-zero.
2. Feed a buffer whose size does not match `width * height` and observe/assert on what
   `gst_buffer_map` + the element's in-place transform do with it.
3. Run with `GST_DEBUG=myedgedetector:5` and confirm `GST_DEBUG_FUNCPTR`-wrapped `set_caps`/
   `transform_ip` calls are visible in the trace.
