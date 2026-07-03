# CapsAndFilters

## New concept
Capability filters (`capsfilter`), caps negotiation, and inspecting negotiated caps on a pad.

GStreamer pads advertise what media formats they can produce or accept via `GstCaps`.
A `capsfilter` element constrains the flow to only matching caps, forcing the upstream
element to produce (or the downstream element to accept) a specific format.

## Pipeline

    videotestsrc → [video/x-raw,format=RGB,width=320,height=240] → videoconvert → autovideosink

## How to run
    ./CapsAndFilters

## Expected output
    Negotiated caps: video/x-raw, format=(string)RGB, width=(int)320, height=(int)240, ...
    End of stream reached.

## Exercises
1. Change the format to `BGR` — does it still work?
2. Remove `videoconvert` from after the capsfilter — what caps negotiation error do you get?
3. Place two capsfilters in series with conflicting constraints (e.g., width=320 and width=640).
