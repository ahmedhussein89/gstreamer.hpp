# Events and Queries

## New concept
**Queries** ask the pipeline for information synchronously: position, duration, latency.
**Events** push control information (seek, flush, EOS) upstream or downstream.

| Mechanism | Direction | Examples |
|---|---|---|
| Query | Request/response | position, duration, latency, caps |
| Event | Upstream or downstream | seek, flush-start/stop, EOS, tag |

Queries travel against the data flow (upstream) via `gst_element_query`.
Events travel with or against the flow via `gst_element_send_event`.

## Pipeline

    videotestsrc → videoconvert → autovideosink

## How to run
    ./EventsAndQueries

## Expected output
    Position: 0.000s
    Duration: unknown (videotestsrc has no fixed duration)
    Latency: live=no min=0.000ms
    End of stream reached.

## Exercises
1. Query the pipeline in PAUSED state — does position advance?
2. Use `gst_query_new_caps` to query the accepted caps on the autovideosink sink pad.
3. Send a custom upstream event with `gst_event_new_custom` and log it with a pad probe.
