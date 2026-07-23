# Custom Plugin

## Problem

No built-in GStreamer element does the per-pixel transform you need (here, edge
detection), and you want it as a reusable, typed pipeline element rather than a one-off
callback. **Fix:** a `GstBaseTransform` subclass (`myedgedetector`) with its own caps
negotiation and an in-place `transform_ip`, registered at runtime with
`gst_element_register()`.

**Note:** this tutorial has only **2 tracks** (`CustomPlugin`, `CustomPluginView`), not the usual 3 —
`MyEdgeDetector` is a GObject/`GstBaseTransform` subclass, so there is no meaningful
RAII-vs-non-owning-view distinction to demonstrate; a `main_raii.cpp` would just be a copy of `main.cpp`.

## New concept
Writing a custom `GstElement` subclass (`GstBaseTransform`) from scratch: pad templates, caps
negotiation (`set_caps`), and an in-place buffer transform (`transform_ip`) — then registering it
at runtime with `gst_element_register()` and using it from both the plain C API and `gst::`.

`MyEdgeDetector` (`myedgedetector.h` / `myedgedetector.c`) accepts and produces `video/x-raw,format=GRAY8`
and replaces each pixel with a simple horizontal-difference edge indicator: white where the pixel
differs from its left neighbor by more than a threshold, black otherwise.

This tutorial deliberately skips the usual plugin packaging (`GST_PLUGIN_DEFINE`, a `.so`,
`GST_PLUGIN_PATH`) — see Exercises.

## Pipeline

    videotestsrc → videoconvert → myedgedetector → fakesink

`videoconvert` negotiates down to `GRAY8` because that is the only format `myedgedetector`'s
pad templates advertise.

## How to run
    ./CustomPlugin
    ./CustomPluginView

## Expected output
    Pipeline running with custom 'myedgedetector' element...
    End of stream reached.

## Exercises
1. Package `MyEdgeDetector` as a real loadable plugin with `GST_PLUGIN_DEFINE` and load it via
   `GST_PLUGIN_PATH` instead of calling `my_edge_detector_register()` from `main()`.
2. Add a `threshold` GObject property (installed in `class_init` via `g_object_class_install_property`)
   so the edge sensitivity can be set with `g_object_set`/`gst-launch-1.0`.
3. Extend the filter to a real 3x3 Sobel kernel instead of a single horizontal difference.
