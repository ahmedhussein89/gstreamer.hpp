# ElementByHand

## New concept
Building a pipeline manually: factory, floating references, and `bin_add` ownership transfer.
`gst_element_factory_make` returns a floating reference. `gst_bin_add` sinks it (the pipeline
takes ownership). Never call `gst_object_unref` on an element after a successful `bin_add`.

## Pipeline

    videotestsrc → videoconvert → autovideosink

## How to run
    ./ElementByHand

## Expected output
    End of stream reached.

## Exercises
1. Remove `videoconvert` and observe the caps negotiation error.
2. Try calling `gst_element_link` in the wrong order — what happens?
3. Add a second sink and branch with a `tee` element (see VideoRecorder for guidance).
