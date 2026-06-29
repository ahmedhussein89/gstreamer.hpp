# Pipeline Builder

## Goal

Learn how to build and run a GStreamer pipeline from a string at runtime,
and see the difference between the raw C API and the `gst::` RAII wrappers.

## Concepts

- `gst_parse_launch` / `gst::parse_launch`
- Runtime pipeline strings
- Error handling via `GError*` vs `nonstd::expected`
- Bus message loop (Error / EOS)

## Pipeline

```text
videotestsrc ! autovideosink   (default)
```

Any valid `gst-launch-1.0` pipeline string can be passed as a command-line argument:

```bash
./PipelineBuilder "videotestsrc pattern=ball ! autovideosink"
./PipelineBuilder "audiotestsrc ! autoaudiosink"
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `PipelineBuilder` | `main.cpp` | Imperative GStreamer C API |
| `PipelineBuilderRAII` | `main_raii.cpp` | `gst::parse_launch` + `gst::` RAII wrappers |

## Running

```bash
./build/tutorials/easy/PipelineBuilder
./build/tutorials/easy/PipelineBuilderRAII "videotestsrc pattern=smpte ! autovideosink"
```

Press `Ctrl+C` to stop.

## Key Difference from HelloWorld

HelloWorld hard-codes a fixed pipeline string. This tutorial shows that the same
`gst_parse_launch` call works for any pipeline — exactly like `gst-launch-1.0` on
the command line. The RAII variant additionally shows how `gst::parse_launch` returns
`expected<Element, ErrorPtr>` instead of a raw pointer that must be freed manually.
