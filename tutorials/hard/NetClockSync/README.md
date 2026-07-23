# Net Clock Sync

## Problem

Multiple pipelines running on separate machines (e.g. multi-room speakers, a video
wall) need to play back in lockstep, but each has its own independent clock. **Fix:**
a `GstNetTimeProvider` serves one machine's clock over TCP; every other machine
connects with a `GstNetClientClock`, adopts it via `gst_pipeline_use_clock`, and pins
its `base_time` to match.

## New concept
Multiple pipelines on different machines can share a single time reference over the network.
A **`GstNetTimeProvider`** wraps a clock (e.g. the system clock) and serves it over TCP.
A **`GstNetClientClock`** connects to that provider and tracks the server's clock, converging
toward it after a handful of round trips (`gst_clock_wait_for_sync`).
A pipeline adopts the net clock with `gst_pipeline_use_clock`, then pins its own `base_time`
with `gst_element_set_base_time` so its running time lines up with every other client using
the same clock — the basis for multi-room/multi-screen synchronized playback.

| Concept | API |
|---|---|
| Serve a clock over the network | `gst_net_time_provider_new` |
| Connect to a served clock | `gst_net_client_clock_new` |
| Wait until synced | `gst_clock_wait_for_sync` |
| Use it as the pipeline clock | `gst_pipeline_use_clock` |
| Align running time to it | `gst_element_set_base_time` |

`GstNetTimeProvider`/`GstNetClientClock` have no `gst::`/`gst::raii::` wrapper — all three
tracks call the raw GStreamer API for those two types directly (see the comment in each
`main*.cpp`); only the surrounding pipeline (`pipeline_new`, `element_factory_make`, `bin_add`,
`element_link`, `element_set_state`, `element_get_bus`, `bus_timed_pop_filtered`) goes through
`gst::`/`gst::raii::`.

## Pipeline

    [server] system clock --served over TCP:9998--> GstNetTimeProvider
                                                            |
    [client] GstNetClientClock <-------------------------- '
                   |
                   v (gst_pipeline_use_clock + gst_element_set_base_time)
    videotestsrc (num-buffers=60) → videoconvert → fakesink

## How to run

Default — both roles in one process against `127.0.0.1`, no second machine needed (CI-safe,
terminates on its own):

    ./NetClockSync

Two machines — start the server on one host, then the client on another, pointed at the
server's address:

    # machine A (server)
    ./NetClockSync --server

    # machine B (client), while the server above is still running
    ./NetClockSync --client <machine-A-ip>

(`NetClockSyncView` / `NetClockSyncRAII` take the same arguments.)

## Expected output

    Serving clock on 127.0.0.1:9998 (in-process loopback).
    Net clock synced with 127.0.0.1:9998. base_time=1234.567s offset-from-system-clock=0ns
    Client finished (60 frames synced to net clock).

The run takes ~2s, not milliseconds: the sink is configured `sync=TRUE` so rendering is
paced by the network clock. With the default `sync=FALSE` the pipeline would drain
instantly and the clock would have no observable effect at all.

## Exercises
1. Run `--server` on one machine and `--client <ip>` on another over a real network and compare
   the printed offset to the in-process loopback run.
2. Change `NumBuffers` and observe that the reported `base_time` stays anchored to when the
   client synced, not to when frames were produced.
3. Skip `gst_element_set_base_time` and see how the pipeline's own auto-assigned base time
   differs from the explicit one taken from the net clock.
