# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**deepstream.hpp** is a header-only, modern C++ wrapper for the NVIDIA DeepStream SDK, inspired by vulkan.hpp. It wraps GStreamer/DeepStream APIs with RAII, strong typing, `nonstd::expected` error handling, and a builder-pattern pipeline API. The `gst` namespace (GStreamer primitives) is implemented; the `ds` namespace (DeepStream elements, metadata, pipeline builder) is actively being developed — see `docs/roadmap.md` for status.

## Development Environment (Docker — REQUIRED)

**All builds, tests, and binary runs MUST happen inside the DeepStream dev container.**
Never invoke `cmake`, `make`, `ninja`, `ctest`, `clang-format`, `clang-tidy`, or any
compiled binary directly on the host — the GStreamer/DeepStream SDK, `expected-lite`,
`tracy`, and the toolchain only exist inside the container image
(`nvcr.io/nvidia/deepstream:9.0-samples-multiarch`, defined in
`.devcontainer/Dockerfile`). Running on the host produces wrong paths, missing
dependencies, and cache mismatches.

The container runs as a non-root `developer` user with `USER_UID=1000`, so always
exec as UID 1000. Find the running container and wrap every command:

```bash
# Discover the container spun up from .devcontainer (name is auto-generated)
CID=$(docker ps --format '{{.ID}} {{.Image}}' | grep -i deepstream | awk '{print $1}' | head -1)

# Run any build/test command inside it, as the developer user
docker exec -u 1000 "$CID" bash -c 'cd /workspace && <command>'
```

If no container is running, start it via VS Code "Reopen in Container" (see
`.devcontainer/`) before building.

## Build Commands

Every command below assumes it is wrapped with
`docker exec -u 1000 "$CID" bash -c 'cd /workspace && ...'`.

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
ctest --test-dir build

# Run a specific test binary directly
./build/tests/testGstreamer

# Run a single named test
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

Full one-liner example (configure + build + test from the host):

```bash
CID=$(docker ps --format '{{.ID}} {{.Image}}' | grep -i deepstream | awk '{print $1}' | head -1)
docker exec -u 1000 "$CID" bash -c 'cd /workspace && cmake -B build -S . && cmake --build build && ctest --test-dir build'
```

## Sanitizers

Run inside the container (see above):

```bash
cmake -B build -S . -DDS_ENABLE_SANITIZERS=ON -DDS_SANITIZER=address
cmake --build build
```

Valid sanitizer values: `address`, `memory`, `thread`, `undefined`, `none`.

## Code Coverage

Run inside the container. Requires `gcovr` (`pip install gcovr`).

```bash
cmake -B build -S . -DENABLE_COVERAGE=ON
cmake --build build
./build/tests/testGstreamer
cmake --build build -t coverage          # generates text + HTML + XML reports
cmake --build build -t coverage-summary  # console summary only
```

Reports land in `build/coverage-reports/`.

## Code Quality

Run inside the container (see Development Environment above).

```bash
# Format all headers
clang-format -i include/gstreamer.hpp include/gstreamer_raii.hpp include/pipeline.hpp \
  include/elements.hpp include/builder.hpp include/metadata.hpp \
  include/core/*.hpp include/utils/*.hpp include/elements/*.hpp include/metadata/*.hpp

# Lint (enforced via .clang-tidy — most checks enabled except google/llvm/abseil/android/fuchsia)
clang-tidy include/gstreamer.hpp -- -I include
```

Warnings are treated as errors (`-Werror`) across GCC and Clang.

## Architecture

### CMake targets

| Target | Alias | Header(s) | Notes |
|---|---|---|---|
| `gstreamer_hpp` | `gstreamer::hpp` | `gstreamer.hpp`, `gstreamer_raii.hpp`, `core/*.hpp` | GStreamer primitives + RAII owning layer; always built |
| `pipeline_hpp` | `pipeline::hpp` | `pipeline.hpp` | Builder-pattern pipeline DSL; depends on `gstreamer::hpp` |
| `deepstream_elements` | `ds::elements` | `elements.hpp`, `builder.hpp`, `elements/*.hpp`, `utils/*.hpp` | DeepStream element wrappers; always built |
| `deepstream_metadata` | `ds::metadata` | `metadata.hpp`, `metadata/*.hpp` | NvDs metadata views; only built when `DeepStream_FOUND` |

### `gst` namespace — `include/core/`

vulkan.hpp-style foundation layer, included transitively by `gstreamer.hpp`:

| Header | Contents |
|---|---|
| `core/core.hpp` | Umbrella that pulls in the rest of `core/` |
| `core/handle.hpp` | Non-owning typed handle wrappers over raw `Gst*` pointers |
| `core/flags.hpp` | Type-safe bit-flag template (bitwise operators on strong enums) |
| `core/enums.hpp` | Strongly-typed enum definitions |
| `core/array_proxy.hpp` | Lightweight `span`-like view for passing arrays into the API |

### `gst` namespace — `include/gstreamer_raii.hpp`

`gst::raii::*` owning layer, mirroring `vulkan_raii.hpp`: each type owns its resource
and releases it in the destructor, and implicitly converts to the matching non-owning
`gst::` handle so every enhanced-layer free function works on a RAII object unchanged.

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

All of these are preinstalled in the dev container image — do not install them on the host.

- `GStreamer` (with `Video` component) — via `cmake/Modules/FindGStreamer.cmake` ([docs](https://gstreamer.freedesktop.org/documentation/?gi-language=c))
- `DeepStream` — bundled in the container (SDK 9.0); enables `ds::metadata` target when found
- `expected-lite` (`nonstd::expected-lite`) — installed from source (v0.10.0) in the image
- `tracy` — profiler (v0.13.0), installed from source in the image
- `fmt` — for formatted output
- `spdlog` — for logging in `ds::elements`
- `GTest` — for tests only

### Tutorials structure

Tutorials live under `tutorials/easy/`, `tutorials/medium/`, `tutorials/hard/`. Each is a standalone CMake subdirectory (build/run them inside the container like everything else).

- **easy**: `HelloWorld`, `VideoFilePlayer`, `WebcamViewer`, `AudioPlayer`, `CapsAndFilters`, `ElementByHand`, `StatesAndSeeking`, `PipelineBuilder`
- **medium**: `DynamicPipeline`, `EventsAndQueries`, `BuffersAndMemory`, `ClocksAndSync`, `CPUVideoProcessing`, `ImageCapture`, `PipelineInspector`, `RTSPClient`, `TagsAndMetadata`, `VideoRecorder`
- **hard**: (none yet)

`VideoFilePlayer` has four variants:

| Binary | Source | Approach |
|---|---|---|
| `VideoFilePlayer` | `main.cpp` | Imperative GStreamer C API |
| `VideoFilePlayerRAII` | `main_raii.cpp` | `gst::` RAII wrappers |
| `VideoFilePlayerDeclarative` | `main_declarative.cpp` | `gst::PipelineDesc` / `gst::build()` |
| `VideoFilePlayerDynamic` | `main_dynamic.cpp` | Dynamic pad linking with `gst::` wrappers |

### DevContainer

`.devcontainer/` provides the Docker-based dev environment that **all** builds and
tests must run in (see "Development Environment" above):

- Base image `nvcr.io/nvidia/deepstream:9.0-samples-multiarch` with the full DeepStream SDK.
- GPU passthrough (`--gpus=all`), `--network=host`, `--ipc=host`, and X11 forwarding
  (`/tmp/.X11-unix` bind mount + `DISPLAY`) for GUI sink windows.
- Extra tooling baked in via `Dockerfile`: `ninja-build`, `libgtest-dev`, `libspdlog-dev`,
  `expected-lite` v0.10.0 and `tracy` v0.13.0 (both from source).
- Runs as a non-root `developer` user (`USER_UID=1000`); `setup_user.sh` provisions it,
  which is why host commands must exec with `-u 1000`.
- DeepStream samples are copied to `/workspace/deepstream-samples`; the repo mounts at `/workspace`.
- VS Code extensions: cpptools, clangd, CMake Tools, Python, GitLens, Git Graph.

## Code Conventions

- C++20 standard (`Standard: c++20` in `.clang-format`)
- 2-space indentation, 130-column limit
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- Left-aligned pointer declarations (`PointerAlignment: Left`)
- All new GStreamer resource wrappers follow the custom-deleter `unique_ptr` pattern already established in `gstreamer.hpp`
- New API functions should return `nonstd::expected<T, E>` — not raw pointers or exceptions
- `gst` namespace: GStreamer primitives and pipeline DSL; `ds` namespace: DeepStream elements, metadata, and builder abstractions
- Metadata views in `include/metadata/` are `#ifdef`-guarded on DeepStream availability; don't add unconditional NvDs includes outside that tree
- **Every template in `include/` must constrain its type parameters with a named C++20 concept or a `requires` clause.** Unconstrained `typename T` / `class T` template parameters are rejected by `scripts/check-concepts.sh` (run in CI). Shared concept vocabulary lives in `include/core/concepts.hpp`; local concepts (specific to one header) are defined in that header. See `docs/concepts-roadmap.md` for the full concept catalogue.
