# deepstream.hpp

A header-only, modern C++ wrapper for NVIDIA DeepStream — inspired by [vulkan.hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

DeepStream's raw GStreamer API is verbose, stringly-typed, and error-prone. **deepstream.hpp** wraps it with RAII resource management, strongly-typed enums, C++20 concept constraints, and `nonstd::expected`-based error handling — giving you explicit control over pipelines without the boilerplate.

## Status

Active development. The `gst` namespace (GStreamer primitives, RAII layer, pipeline DSL) is implemented and covered by tests. The `ds` namespace (typed DeepStream elements, pipeline builder, metadata views) is partially implemented — see [`docs/description.md`](docs/description.md) and [`docs/roadmap.md`](docs/roadmap.md) for details.

## Quick example

```cpp
#include <gstreamer.hpp>

gst_init(&argc, &argv);

auto result = gst::parse_launch("videotestsrc ! autovideosink");
if (!result) {
    // result.error() is a gst::ErrorPtr (unique_ptr<GError, ...>)
    fmt::println(stderr, "Pipeline error: {}", result.error()->message);
    return EXIT_FAILURE;
}

gst::Element& pipeline = result.value();
gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);
```

## Architecture

The library mirrors the two-layer design of `vulkan.hpp`:

| Layer | Namespace | Header(s) | Description |
| ----- | --------- | --------- | ----------- |
| C++ wrapper | `gst` | `include/gstreamer.hpp`, `include/pipeline.hpp`, `include/core/*.hpp` | Non-owning typed handles, type-safe enums/flags, C++20 concepts, free functions, declarative pipeline DSL |
| RAII | `gst::raii` | `include/gstreamer_raii.hpp` | Owning wrappers that implicitly convert to the `gst` layer — mirrors `vulkan_raii.hpp` |

The `ds` namespace (`include/elements.hpp`, `include/builder.hpp`, `include/metadata/*.hpp`) extends the wrapper layer with typed DeepStream element factories, a pipeline builder, and zero-cost NvDs metadata views.

## What's implemented (`gst` namespace)

**RAII types** — `include/gstreamer.hpp`

| Symbol | Description |
| ------ | ----------- |
| `gst::Element` | Move-only RAII wrapper around `GstElement*` |
| `gst::Pipeline` | Move-only RAII wrapper around a `GstPipeline` element |
| `gst::ElementPtr` | `unique_ptr<GstElement, GstElementDeleter>` |
| `gst::BusPtr` | `unique_ptr<GstBus, GstBusDeleter>` |
| `gst::ErrorPtr` | `unique_ptr<GError, GstErrorDeleter>` |
| `gst::MessagePtr` | `unique_ptr<GstMessage, GstMessageDeleter>` |
| `gst::PadPtr` | `unique_ptr<GstPad, GstPadDeleter>` |
| `gst::CapsPtr` | `unique_ptr<GstCaps, GstCapsDeleter>` |
| `gst::MessageType` | Strongly-typed enum over `GstMessageType`; supports `\|` and `&` |
| `gst::StateChange` | POD struct holding `old_state`, `new_state`, `pending` |

**Free functions** — all return `nonstd::expected<T, E>`, no exceptions

| Symbol | Returns |
| ------ | ------- |
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

**Pipeline DSL** — `include/pipeline.hpp`

| Symbol | Description |
| ------ | ----------- |
| `gst::PropertyValue` | `variant<bool, int32, uint32, int64, uint64, double, string>` — typed element property |
| `gst::Node` | Describes one element: factory name, optional instance name, and properties (chained via `.prop(key, value)`) |
| `gst::PipelineDesc` | Ordered list of `Node`s that form a linear pipeline |
| `gst::build(PipelineDesc)` | Creates, configures, and links all elements; returns `expected<Pipeline, string>` |

**RAII layer** — `include/gstreamer_raii.hpp`

`gst::raii::*` owning wrappers mirroring `vulkan_raii.hpp`. Each type owns its resource and releases it on destruction, and implicitly converts to the matching non-owning `gst::` handle so every free function works unchanged on a RAII object.

## Building

Requires CMake 3.20+, a C++20 compiler, GStreamer (with the Video component), and `gcovr` for coverage reports. **All builds must run inside the dev container** — see [DevContainer](#devcontainer).

```bash
# Configure (builds tutorials + tests by default)
cmake -B build -S .

# Build
cmake --build build

# Run tests
ctest --test-dir build

# Run a specific test
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

### CMake options

| Option                 | Default   | Description                                                   |
| ---------------------- | --------- | ------------------------------------------------------------- |
| `DS_BUILD_TUTORIALS`   | `ON`      | Build tutorial programs                                       |
| `DS_BUILD_TESTS`       | `ON`      | Build GTest suite                                             |
| `DS_BUILD_EXAMPLES`    | `OFF`     | Build reference examples                                      |
| `DS_ENABLE_SANITIZERS` | `OFF`     | Enable a sanitizer build                                      |
| `DS_SANITIZER`         | `address` | Sanitizer to use (`address`, `memory`, `thread`, `undefined`) |
| `ENABLE_COVERAGE`      | `OFF`     | Enable code coverage instrumentation                          |

### Sanitizers

```bash
cmake -B build -S . -DDS_ENABLE_SANITIZERS=ON -DDS_SANITIZER=address
cmake --build build
```

### Code coverage

```bash
cmake -B build -S . -DENABLE_COVERAGE=ON
cmake --build build
./build/tests/testGstreamer
cmake --build build -t coverage          # text + HTML + XML reports in build/coverage-reports/
cmake --build build -t coverage-summary  # console summary only
```

## Consuming the library

The CMake interface target is `gstreamer::hpp`:

```cmake
find_package(GStreamer REQUIRED)
target_link_libraries(my_target PRIVATE gstreamer::hpp)
```

Include the header:

```cpp
#include <gstreamer.hpp>
```

## Tutorials

Step-by-step tutorials live under `tutorials/`. Each topic ships three source files and matching binaries:

| Suffix | Source | Approach |
| ------ | ------ | -------- |
| *(none)* | `main.cpp` | Raw GStreamer C API |
| `RAII` | `main_raii.cpp` | `gst::` RAII wrappers |
| `View` | `main_view.cpp` | `gst::` free functions + declarative DSL |

### Easy

| Tutorial | Description |
| -------- | ----------- |
| `HelloWorld` | `gst_parse_launch` pipeline with `videotestsrc` |
| `VideoFilePlayer` | Manual element creation, bus polling, EOS/error handling |
| `WebcamViewer` | Live capture from a V4L2 webcam |
| `AudioPlayer` | Audio file playback with `playbin` |
| `CapsAndFilters` | Caps negotiation and `capsfilter` |
| `ElementByHand` | Creating and linking elements manually |
| `StatesAndSeeking` | State machine, seeking, and position queries |
| `PipelineBuilder` | Declarative pipeline with `gst::PipelineDesc` / `gst::build()` |

### Medium

| Tutorial | Description |
| -------- | ----------- |
| `DynamicPipeline` | Dynamic pad linking with `pad-added` signal |
| `EventsAndQueries` | Sending events and position/duration queries |
| `BuffersAndMemory` | `appsrc` / `appsink`, buffer access and mapping |
| `ClocksAndSync` | Pipeline clock, base time, and A/V sync |
| `CPUVideoProcessing` | Per-frame CPU processing via `appsink`/`appsrc` |
| `ImageCapture` | Snapshot from a live pipeline to PNG |
| `PipelineInspector` | Introspecting element pads and caps at runtime |
| `RTSPClient` | Consuming an RTSP stream with `rtspsrc` |
| `TagsAndMetadata` | Reading stream tags and metadata |
| `VideoRecorder` | Encoding and muxing video to a file |

## Dependencies

Fetched automatically via CMake `find_package` or `FetchContent`:

- [GStreamer](https://gstreamer.freedesktop.org/) (with Video component)
- [expected-lite](https://github.com/martinmoene/expected-lite) (`nonstd::expected`)
- [fmt](https://github.com/fmtlib/fmt)
- [spdlog](https://github.com/gabime/spdlog)
- [tracy](https://github.com/wolfpld/tracy) (profiler, v0.13.0)
- [GoogleTest](https://github.com/google/googletest) (tests only)

## Code style

- C++20, 2-space indent, 130-column limit
- Google style base via `.clang-format`
- Most clang-tidy checks enabled (see `.clang-tidy`)
- Warnings as errors on GCC and Clang (`-Werror`)
- Every template parameter must be constrained by a named C++20 concept or `requires` clause (enforced by `scripts/check-concepts.sh`)

```bash
clang-format -i include/gstreamer.hpp
clang-tidy include/gstreamer.hpp -- -I include
```

## DevContainer

`.devcontainer/` provides a Docker environment with GPU passthrough (`--gpus=all`), X11 forwarding, and VS Code extensions for clangd, CMake Tools, and GitLens.

**All builds, tests, and binary runs must happen inside the container** — GStreamer, DeepStream SDK, `expected-lite`, `tracy`, and the toolchain only exist inside the image (`nvcr.io/nvidia/deepstream:9.0-samples-multiarch`).

```bash
# Find the running container
CID=$(docker ps --format '{{.ID}} {{.Image}}' | grep -i deepstream | awk '{print $1}' | head -1)

# Run any build command inside it
docker exec -u 1000 "$CID" bash -c 'cd /workspace && cmake -B build -S . && cmake --build build'
```

## License

See [LICENSE](LICENSE).
