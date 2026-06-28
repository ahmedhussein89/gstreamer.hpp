# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**deepstream.hpp** is a header-only, modern C++ wrapper for the NVIDIA DeepStream SDK, inspired by vulkan.hpp. It wraps GStreamer/DeepStream APIs with RAII, strong typing, `nonstd::expected` error handling, and a builder-pattern pipeline API. The project is in early development — currently the primary implemented header is `include/gstreamer.hpp` (the `gst` namespace), with the broader DeepStream wrapper (`ds` namespace) described in `docs/description.md` as a roadmap.

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
# Format (enforced via .clang-format, Google style base, 130-col limit, C++20)
clang-format -i include/gstreamer.hpp

# Lint (enforced via .clang-tidy — most checks enabled except google/llvm/abseil/android/fuchsia)
clang-tidy include/gstreamer.hpp -- -I include
```

Warnings are treated as errors (`-Werror`) across GCC and Clang.

## Architecture

### Header-only library target

`include/gstreamer.hpp` is the only implemented header. It is exposed as a CMake interface library target `gstreamer::hpp` (alias for `gstreamer_hpp`). Downstream targets link against it with `target_link_libraries(... gstreamer::hpp)`.

### `gst` namespace — what's implemented

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

Error handling pattern throughout: use `nonstd::expected` (from `expected-lite`) rather than exceptions. Functions return `nonstd::make_unexpected(...)` on failure.

### Dependencies (found via CMake `find_package`)

- `GStreamer` (with `Video` component) — via `cmake/Modules/FindGStreamer.cmake`
- `expected-lite` (`nonstd::expected-lite`) — fetched via FetchContent into `build/expected-lite/`
- `fmt` — for formatted output
- `spdlog` — available for logging
- `GTest` — for tests only

### Tutorials structure

Tutorials live under `tutorials/easy/`, `tutorials/medium/`, `tutorials/hard/`. Each is a standalone CMake subdirectory. Current easy tutorials: `HelloWorld` and `VideoFilePlayer`.

### DevContainer

`.devcontainer/` provides a Docker-based dev environment with GPU passthrough (`--gpus=all`), X11 forwarding for GUI elements, and VS Code extensions for clangd, CMake Tools, and GitLens.

## Code Conventions

- C++20 standard (`Standard: c++20` in `.clang-format`)
- 2-space indentation, 130-column limit
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- Left-aligned pointer declarations (`PointerAlignment: Left`)
- All new GStreamer resource wrappers follow the custom-deleter `unique_ptr` pattern already established in `gstreamer.hpp`
- New API functions should return `nonstd::expected<T, E>` — not raw pointers or exceptions
