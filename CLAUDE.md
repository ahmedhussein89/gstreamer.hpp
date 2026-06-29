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

**RAII types**

| Symbol | Purpose |
|---|---|
| `gst::Element` | Move-only RAII wrapper around `GstElement*` |
| `gst::Pipeline` | Move-only RAII wrapper around a `GstPipeline` element |
| `gst::ElementPtr` | `unique_ptr<GstElement, GstElementDeleter>` |
| `gst::BusPtr` | `unique_ptr<GstBus, GstBusDeleter>` |
| `gst::ErrorPtr` | `unique_ptr<GError, GstErrorDeleter>` |
| `gst::MessagePtr` | `unique_ptr<GstMessage, GstMessageDeleter>` |
| `gst::PadPtr` | `unique_ptr<GstPad, GstPadDeleter>` |
| `gst::CapsPtr` | `unique_ptr<GstCaps, GstCapsDeleter>` |
| `gst::MessageType` | Strongly-typed enum wrapping `GstMessageType`; supports `\|` and `&` |
| `gst::StateChange` | POD struct holding `old_state`, `new_state`, `pending` |

**Free functions**

| Symbol | Returns |
|---|---|
| `gst::init(span<char*>)` | `void` — initialises GStreamer once (static guard) |
| `gst::parse_launch(string_view)` | `expected<Element, ErrorPtr>` |
| `gst::pipeline_new(string_view name={})` | `expected<Pipeline, string>` |
| `gst::element_factory_make(factory, name={})` | `expected<Element, string>` |
| `gst::bin_add(pipeline, Element)` | `expected<GstElement*, string>` — transfers ownership into bin |
| `gst::element_link(src, sink)` | `expected<void, string>` |
| `gst::element_get_bus(pipeline)` | `expected<BusPtr, string>` |
| `gst::element_set_state<T>(element, GstState)` | `expected<void, string>` |
| `gst::element_get_static_pad(element, name)` | `expected<PadPtr, string>` |
| `gst::pad_is_linked(PadPtr)` | `bool` |
| `gst::pad_link(src, sink)` | `expected<void, string>` |
| `gst::pad_get_current_caps(GstPad*)` | `expected<CapsPtr, string>` |
| `gst::caps_from_string(string_view)` | `expected<CapsPtr, string>` |
| `gst::caps_get_structure(CapsPtr, index=0)` | `expected<const GstStructure*, string>` |
| `gst::structure_get_name(GstStructure*)` | `string_view` |
| `gst::message_type(MessagePtr)` | `MessageType` |
| `gst::message_parse_error(GstMessage*)` | `expected<pair<string,string>, string>` |
| `gst::message_parse_state_changed(MessagePtr)` | `StateChange` |
| `gst::state_get_name(GstState)` | `string_view` |
| `gst::bus_timed_pop_filtered(BusPtr, timeout, MessageType)` | `expected<MessagePtr, string>` |

### `gst` namespace — `include/pipeline.hpp`

Declarative pipeline DSL built on top of `gstreamer.hpp`.

| Symbol | Purpose |
|---|---|
| `gst::PropertyValue` | `variant<bool, int32, uint32, int64, uint64, double, string>` — typed element property |
| `gst::Node` | Describes one element: factory name, optional instance name, and properties (set via `.prop(key, value)` chaining) |
| `gst::PipelineDesc` | Ordered list of `Node`s that form a linear pipeline |
| `gst::build(PipelineDesc)` | Creates, configures, and links all elements; returns `expected<Pipeline, string>` |

### `ds` namespace — `include/elements.hpp`, `include/builder.hpp`

Typed factory helpers for DeepStream pipeline nodes (sources, transforms, inference, tracking, sinks). `builder.hpp` provides the `ds::PipelineBuilder` that assembles a pipeline from `ds::*` node descriptors.

### `ds` namespace — `include/metadata/*.hpp`

Zero-cost views over NvDs metadata structures. Only compiled when DeepStream is found. Requires linking `ds::metadata`.

| Header | Contents |
|---|---|
| `metadata/batch_meta.hpp` | `NvDsBatchMeta` view |
| `metadata/frame_meta.hpp` | `NvDsFrameMeta` view |
| `metadata/object_meta.hpp` | `NvDsObjectMeta` view |
| `metadata/classifier_meta.hpp` | `NvDsClassifierMeta` view |
| `metadata/tensor_meta.hpp` | `NvDsInferTensorMeta` view |
| `metadata/user_meta.hpp` | `NvDsUserMeta` view |
| `metadata/meta_list_view.hpp` | Range adaptor over `NvDsMetaList` |

### `ds` namespace — `include/utils/`

- `utils/error.hpp` — `ds::ErrorKind` enum and `ds::Error` structured error type
- `utils/debug.hpp` — debug/logging helpers

Error handling pattern throughout: use `nonstd::expected` (from `expected-lite`) rather than exceptions. Functions return `nonstd::make_unexpected(...)` on failure.

### Dependencies (found via CMake `find_package`)

- `GStreamer` (with `Video` component) — via `cmake/Modules/FindGStreamer.cmake` ([docs](https://gstreamer.freedesktop.org/documentation/?gi-language=c))
- `DeepStream` — optional; enables `ds::metadata` target when found
- `expected-lite` (`nonstd::expected-lite`) — fetched via FetchContent into `build/expected-lite/`
- `fmt` — for formatted output
- `spdlog` — for logging in `ds::elements`
- `GTest` — for tests only

### Tutorials structure

Tutorials live under `tutorials/easy/`, `tutorials/medium/`, `tutorials/hard/`. Each is a standalone CMake subdirectory. Current easy tutorials: `HelloWorld` and `VideoFilePlayer`.

`VideoFilePlayer` has four variants:

| Binary | Source | Approach |
|---|---|---|
| `VideoFilePlayer` | `main.cpp` | Imperative GStreamer C API |
| `VideoFilePlayerRAII` | `main_raii.cpp` | `gst::` RAII wrappers |
| `VideoFilePlayerDeclarative` | `main_declarative.cpp` | `gst::PipelineDesc` / `gst::build()` |
| `VideoFilePlayerDynamic` | `main_dynamic.cpp` | Dynamic pad linking with `gst::` wrappers |

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
