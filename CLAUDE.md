# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**deepstream.hpp** is a header-only, modern C++ wrapper for the NVIDIA DeepStream SDK, inspired by vulkan.hpp. It wraps GStreamer/DeepStream APIs with RAII, strong typing, `nonstd::expected` error handling, and a builder-pattern pipeline API. The `gst` namespace (GStreamer primitives) is implemented; the `ds` namespace (DeepStream elements, metadata, pipeline builder) is actively being developed — see `docs/roadmap.md` for status.

## Build Commands

```bash
# Configure (default: builds tutorials + tests, no examples)
cmake -B build -S .

# Configure with all options explicit
cmake -B build -S . \
  -DDS_BUILD_EXAMPLES=OFF \
  -DDS_BUILD_TUTORIALS=ON \
  -DDS_BUILD_TESTS=ON

# Build
cmake --build build

# Run all tests
cd build && ctest

# Run a specific test binary directly
./build/tests/testGstreamer

# Run a single named test
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

## Sanitizers

```bash
cmake -B build -S . -DDS_ENABLE_SANITIZERS=ON -DDS_SANITIZER=address
cmake --build build
```

Valid sanitizer values: `address`, `memory`, `thread`, `undefined`, `none`.

## Code Coverage

Requires `gcovr` (`pip install gcovr`).

```bash
cmake -B build -S . -DENABLE_COVERAGE=ON
cmake --build build
./build/tests/testGstreamer
cmake --build build -t coverage          # generates text + HTML + XML reports
cmake --build build -t coverage-summary  # console summary only
```

Reports land in `build/coverage-reports/`.

## Code Quality

```bash
# Format all headers
clang-format -i include/gstreamer.hpp include/pipeline.hpp include/elements.hpp \
  include/builder.hpp include/metadata.hpp include/utils/*.hpp \
  include/elements/*.hpp include/metadata/*.hpp

# Lint (enforced via .clang-tidy — most checks enabled except google/llvm/abseil/android/fuchsia)
clang-tidy include/gstreamer.hpp -- -I include
```

Warnings are treated as errors (`-Werror`) across GCC and Clang.

## Architecture

### CMake targets

| Target | Alias | Header(s) | Notes |
|---|---|---|---|
| `gstreamer_hpp` | `gstreamer::hpp` | `gstreamer.hpp` | GStreamer primitives; always built |
| `pipeline_hpp` | `pipeline::hpp` | `pipeline.hpp` | Builder-pattern pipeline DSL; depends on `gstreamer::hpp` |
| `deepstream_elements` | `ds::elements` | `elements.hpp`, `builder.hpp`, `elements/*.hpp`, `utils/*.hpp` | DeepStream element wrappers; always built |
| `deepstream_metadata` | `ds::metadata` | `metadata.hpp`, `metadata/*.hpp` | NvDs metadata views; only built when `DeepStream_FOUND` |

### `gst` namespace — `include/gstreamer.hpp`

| Symbol | Purpose |
|---|---|
| `gst::Element` | RAII wrapper around `GstElement*` (move-only) |
| `gst::ElementPtr` | `unique_ptr<GstElement, GstElementDeleter>` |
| `gst::BusPtr` | `unique_ptr<GstBus, GstBusDeleter>` |
| `gst::ErrorPtr` | `unique_ptr<GError, GstErrorDeleter>` |
| `gst::MessagePtr` | `unique_ptr<GstMessage, GstMessageDeleter>` |
| `gst::MessageType` | Strongly-typed enum wrapping `GstMessageType`; supports `\|` and `&` |
| `gst::parse_launch()` | Returns `nonstd::expected<Element, ErrorPtr>` |
| `gst::message_parse_error()` | Returns `nonstd::expected<pair<string,string>, string>` |
| `gst::bus_timed_pop_filtered()` | Returns `nonstd::expected<MessagePtr, string>` |
| `gst::Node` / `gst::Graph` | Pipeline DSL node and graph types (`pipeline.hpp`) |

### `ds` namespace — `include/elements.hpp`, `include/builder.hpp`

Typed factory helpers for DeepStream pipeline nodes (sources, transforms, inference, tracking, sinks). `builder.hpp` provides the `ds::PipelineBuilder` that assembles a `gst::Graph` from `ds::*` node descriptors.

### `ds` namespace — `include/metadata/*.hpp`

Zero-cost views over NvDs metadata structures (`NvDsBatchMeta`, `NvDsFrameMeta`, `NvDsObjectMeta`, etc.). Only compiled when DeepStream is found. Requires linking `ds::metadata`.

### `ds` namespace — `include/utils/`

- `utils/error.hpp` — `ds::ErrorKind` enum and `ds::Error` structured error type
- `utils/debug.hpp` — debug/logging helpers

Error handling pattern throughout: use `nonstd::expected` (from `expected-lite`) rather than exceptions. Functions return `nonstd::make_unexpected(...)` on failure.

### Dependencies (found via CMake `find_package`)

- `GStreamer` (with `Video` component) — via `cmake/Modules/FindGStreamer.cmake`
- `DeepStream` — optional; enables `ds::metadata` target when found
- `expected-lite` (`nonstd::expected-lite`) — fetched via FetchContent into `build/expected-lite/`
- `fmt` — for formatted output
- `spdlog` — for logging in `ds::elements`
- `GTest` — for tests only

### Tutorials structure

Tutorials live under `tutorials/easy/`, `tutorials/medium/`, `tutorials/hard/`. Each is a standalone CMake subdirectory. Current easy tutorials: `HelloWorld` and `VideoFilePlayer` (includes both imperative and declarative `pipeline.hpp`-based variants).

### DevContainer

`.devcontainer/` provides a Docker-based dev environment with GPU passthrough (`--gpus=all`), X11 forwarding for GUI elements, and VS Code extensions for clangd, CMake Tools, and GitLens.

## Code Conventions

- C++20 standard (`Standard: c++20` in `.clang-format`)
- 2-space indentation, 130-column limit
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- Left-aligned pointer declarations (`PointerAlignment: Left`)
- All new GStreamer resource wrappers follow the custom-deleter `unique_ptr` pattern already established in `gstreamer.hpp`
- New API functions should return `nonstd::expected<T, E>` — not raw pointers or exceptions
- `gst` namespace: GStreamer primitives and pipeline DSL; `ds` namespace: DeepStream elements, metadata, and builder abstractions
- Metadata views in `include/metadata/` are `#ifdef`-guarded on DeepStream availability; don't add unconditional NvDs includes outside that tree
