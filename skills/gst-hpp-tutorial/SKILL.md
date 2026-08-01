---
name: gst-hpp-tutorial
description: Add or modify a tutorial in the gstreamer.hpp repo. Use when asked to create a new tutorial or example program, add a variant (main.cpp / main_view.cpp / main_raii.cpp) to an existing one, wire a tutorial into CMake, or write a tutorial README. Covers the three-variant convention, the CMake target pattern, and the doc tables that must be updated.
owner: ahmedhussein89/gstreamer.hpp
service: gstreamer-hpp
version: 1.0.0
reviewed: 2026-08-01
license: same-as-repo
---

# Adding a Tutorial

Tutorials are the library's documentation-by-example and its integration test surface. A tutorial
that only shows one style teaches half the point — the value is in seeing the **same pipeline**
expressed three ways.

## The three-variant convention

| File | Target suffix | Shows |
|---|---|---|
| `main.cpp` | *(none)* — `Foo` | Imperative, close to the raw GStreamer C API |
| `main_view.cpp` | `View` — `FooView` | Enhanced layer: non-owning `gst::` handles + `expected` free functions |
| `main_raii.cpp` | `RAII` — `FooRAII` | `gst::raii::` owning types, scope-based cleanup |

Every variant builds the **same pipeline** and prints the **same output**. The diff between them is
the lesson. Not every existing tutorial has all three — new ones should.

> **Doc drift to fix, not copy:** `tutorials/easy/README.md` still describes
> `main_declarative.cpp` / `main_dynamic.cpp` variants and a `-DDS_BUILD_TUTORIALS=ON` flag. Neither
> exists — the option is `GST_BUILD_TUTORIALS` and the variants on disk are
> `main.cpp` / `main_view.cpp` / `main_raii.cpp`. Correct the table when you touch it.

## Scaffold

```bash
python3 <skill-path>/scripts/new_tutorial.py --tier easy --name BufferTiming \
  --pipeline "videotestsrc num-buffers=100 ! videoconvert ! fakesink" \
  --concept "Buffer PTS/DTS and running time"
```

Creates `tutorials/<tier>/<Name>/` with all three sources, a `CMakeLists.txt` with the three
targets, and a `README.md` in the house format; then appends the `add_subdirectory(<Name>)` line to
the tier `CMakeLists.txt`. Pass `--dry-run` to preview, `--force` to overwrite.

The generated sources compile and run as-is — they are a working starting point, not a stub. Replace
the body with the actual lesson.

## Rules

1. **Tutorials link `gstreamer::warnings`, not `gstreamer::warnings_strict`.** Warnings are not
   fatal here. Never weaken a header in `include/` to make a tutorial build clean.
2. **Three targets per tutorial**, each with its own `add_executable` / `target_sources` /
   `target_link_libraries` block. Link `fmt::fmt`, `nonstd::expected-lite`, `gstreamer::hpp`,
   `GStreamer::Video`, `${SELECTED_SANITIZER}` — then `gstreamer::warnings` in a separate call.
   Copy `tutorials/easy/HelloWorld/CMakeLists.txt`.
3. **Extra libraries are the tutorial's own responsibility.** `GStreamer::Check` for anything using
   `gstreamer_harness.hpp`; `GStreamer::RtspServer` for `gstreamer_rtsp.hpp`. `gstreamer_hpp`
   deliberately does not carry them.
4. **Self-skip on a missing optional dependency**, as `tutorials/hard/RTSPServer/CMakeLists.txt`
   does — guard with `if(NOT TARGET GStreamer::RtspServer) ... return() endif()` and a
   `message(STATUS "Skipping ...")`. Never fail the whole configure.
5. **Only `-plugins-base` / `-plugins-good` elements** if the tutorial is expected to run in CI.
   Prefer `fakesink` over `autovideosink` for anything that must run headless.
6. **Drive the pipeline to `State::Null` before exit** and check every `expected`. Tutorials are
   read as the canonical example of how to use the API — sloppy error handling here propagates.
7. **Tracy is on** for tutorials (`third_party/tracy`, `TRACY_ENABLE` forced when
   `GST_BUILD_TUTORIALS=ON`). Use Tracy zones rather than timing `printf`s if the tutorial is about
   performance.

## Update the doc tables

A new tutorial touches four places. The scaffolder does the first two:

1. `tutorials/<tier>/CMakeLists.txt` — `add_subdirectory(<Name>)` *(automatic)*
2. `tutorials/<tier>/<Name>/README.md` *(automatic)*
3. `tutorials/<tier>/README.md` — the tutorial table **and** the progression diagram *(manual)*
4. `tutorials/README.md` — the top-level summary table *(manual)*

`docs/tutorials.md` also carries a list — check whether it needs the entry.

## Tutorial README format

Fixed section order, taken from the existing ones:

```markdown
# <Name>

## Problem
<the concrete situation this solves, and the one-line fix>

## New concept
<the single idea introduced, then 2-4 sentences of explanation>

## Pipeline
>    element → element → element

## How to run
    ./<Name>

## Expected output
    <literal expected stdout>
```

Details and a worked example: [references/tutorial-anatomy.md](references/tutorial-anatomy.md).

## Verify

```bash
cmake -B build -S . && cmake --build build
./build/tutorials/<tier>/<Name>/<Name>
./build/tutorials/<tier>/<Name>/<Name>View
./build/tutorials/<tier>/<Name>/<Name>RAII
```

All three must produce identical output. If they don't, one of them is wrong.
