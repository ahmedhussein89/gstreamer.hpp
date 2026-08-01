---
name: gst-hpp-pipeline
description: Build, translate, validate, and debug GStreamer pipelines using gstreamer.hpp. Use when asked to write a pipeline in C++, convert a gst-launch-1.0 string into gst::PipelineDesc / parse_launch / manual element construction, choose between the DSL and parse_launch, diagnose a pipeline that will not link or negotiate caps, or handle dynamic pads, tees, and request pads.
owner: ahmedhussein89/gstreamer.hpp
service: gstreamer-hpp
version: 1.0.0
reviewed: 2026-08-01
license: same-as-repo
---

# Pipeline Authoring with gstreamer.hpp

## Step 1 — Pick the construction style

There are three, and the choice is forced by the pipeline's shape, not by taste.

| Style | API | Use when | Cannot do |
|---|---|---|---|
| **DSL** | `gst::PipelineDesc` + `gst::build()` | Linear chain, static properties, compile-time known | tees, request pads, dynamic pads, inline caps |
| **parse_launch** | `gst::parse_launch(sv)` | Pipeline comes from config/CLI, or is non-linear, or you want the shortest path | Type-safe properties; errors are runtime strings |
| **Manual** | `element_factory_make` + `bin_add` + `element_link` / `pad_link` | You need handles to individual elements — probes, signals, dynamic reconfiguration | — (most verbose) |

**`gst::build()` is linear only.** It links element *i* to *i+1* in order. A `tee`, a
`compositor`, a `decodebin`, or an inline caps filter cannot go through it. This is by design; see
`docs/roadmap.md` before proposing a change.

## Step 2 — Validate and translate

```bash
python3 <skill-path>/scripts/validate_pipeline.py "<pipeline>" --format summary
python3 <skill-path>/scripts/validate_pipeline.py "<pipeline>" --emit-cpp dsl      # or parse, manual
python3 <skill-path>/scripts/validate_pipeline.py "<pipeline>" --emit-cpp manual --raii
```

The script runs four checks — syntax, element existence, property names, DSL fit — and reports
*why* a pipeline is not DSL-expressible. It emits ready-to-paste code in the repo's idioms
(`expected` checks, `fmt::print(stderr, ...)`, correct ownership at `bin_add`).

Element and property checks need `gst-inspect-1.0` on PATH; without it they are skipped and the
script says so. Treat emitted code as a starting point — always confirm property types with
`gst-inspect-1.0 <element>` before committing, since `g_object_set` is varargs and fails silently
on a wrong name or type.

**Always pass `--format summary`** in conversation. The default `text` mode is verbose and `json` is
for programmatic callers.

## Step 3 — Write it in repo idiom

Every fallible call is checked. Never `.value()` unchecked.

```cpp
gst::PipelineDesc desc{
    gst::Node{"videotestsrc"}.prop("num-buffers", std::int32_t{100}).prop("pattern", "snow"),
    gst::Node{"videoconvert"},
    gst::Node{"autovideosink", "sink"},
};

auto pipeline = gst::build(desc);
if(!pipeline) {
  fmt::print(stderr, "build failed: {}\n", pipeline.error());
  return EXIT_FAILURE;
}
```

Watch the error accessor: `parse_launch` fails with an `ErrorPtr` (`.error()->message`); everything
else fails with a `std::string` (`.error()`). Mixing them up is the most common compile error here.

`PropertyValue` is `variant<bool, int32_t, uint32_t, int64_t, uint64_t, double, string>`. An integer
literal is `int` and will pick `int32_t` — be explicit (`std::int64_t{...}`) when the property is
64-bit, and remember GStreamer enum properties are settable by their **nickname string**
("snow", "smpte"), which maps to the `string` alternative.

## Step 4 — Debug

```bash
GST_DEBUG=3 ./build/tutorials/easy/HelloWorld/HelloWorld
GST_DEBUG=GST_CAPS:5,videoconvert:5 ./...
GST_DEBUG_DUMP_DOT_DIR=/tmp/dots ./... && dot -Tpng /tmp/dots/*PLAYING*.dot -o pipeline.png
```

The dot dump is the fastest diagnostic for a link or negotiation failure — the failing link is a
missing edge and every pad carries its negotiated caps.

Details: [references/pipeline-patterns.md](references/pipeline-patterns.md).

## Critical Rules

1. **Do not silently extend the DSL.** If the request needs branching, use `parse_launch` or manual
   construction and say why. Turning `PipelineDesc` from a vector into a graph is a roadmap
   decision.
2. **Only `-plugins-base` / `-plugins-good` elements are available in CI.** Safe: `fakesrc`,
   `fakesink`, `videotestsrc`, `audiotestsrc`, `videoconvert`, `videoscale`, `decodebin`,
   `autovideosink`, `capsfilter`, `queue`, `tee`, `identity`, `compositor`. Anything else will pass
   locally and fail CI.
3. **Dynamic pads need a `pad-added` handler.** `decodebin`, `uridecodebin`, `rtspsrc`, and every
   demuxer expose pads only after they see data — they cannot be linked at construction time.
4. **Request pads must be released.** `element_request_pad_simple` pairs with
   `element_release_request_pad`. Leaking a request pad on `tee`/`compositor` keeps the whole branch
   alive.
5. **Never leave a pipeline `PLAYING` at exit** in a test or tutorial — set `State::Null` first, or
   the sanitizer build reports live objects.
6. **Verify property names with `gst-inspect-1.0`.** `g_object_set` cannot type-check.

## Reference

| Document | Read when |
|---|---|
| [references/pipeline-patterns.md](references/pipeline-patterns.md) | Dynamic pads, tee branches, request pads, caps filters, bus loops, seeking |
