# gstreamer.hpp

A header-only, modern C++ wrapper for GStreamer — inspired by [vulkan.hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

GStreamer's raw C API is verbose, stringly-typed, and error-prone. **gstreamer.hpp** wraps it with RAII resource management, strongly-typed enums, C++20 concept constraints, and `nonstd::expected`-based error handling — giving you explicit control over pipelines without the boilerplate.

## Status

Active development. The `gst` namespace (GStreamer primitives, RAII layer, pipeline DSL) is implemented and covered by tests.

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
| C++ wrapper | `gst` | `include/gstreamer.hpp`, `include/core/*.hpp` | Non-owning typed handles, type-safe enums/flags, C++20 concepts, free functions, declarative pipeline DSL (`gst::Node`/`gst::PipelineDesc`) |
| RAII | `gst::raii` | `include/gstreamer_raii.hpp` | Owning wrappers that implicitly convert to the `gst` layer — mirrors `vulkan_raii.hpp`; `gst::build()` (pipeline DSL builder) lives here |

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

**Pipeline DSL** — `include/gstreamer.hpp` + `include/gstreamer_raii.hpp`

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

| Option                  | Default   | Description                                                   |
| ----------------------- | --------- | ------------------------------------------------------------- |
| `GST_BUILD_TUTORIALS`   | `ON`      | Build tutorial programs                                       |
| `GST_BUILD_TESTS`       | `ON`      | Build GTest suite                                             |
| `GST_ENABLE_SANITIZERS` | `OFF`     | Enable a sanitizer build                                      |
| `GST_SANITIZER`         | `address` | Sanitizer to use (`address`, `memory`, `thread`, `undefined`) |
| `ENABLE_COVERAGE`       | `OFF`     | Enable code coverage instrumentation                          |

### Sanitizers

```bash
cmake -B build -S . -DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address
cmake --build build
LSAN_OPTIONS=suppressions=.lsan-suppressions.txt ./build/tutorials/medium/CPUVideoProcessing/CPUVideoProcessing
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

### Package managers

Both recipes build with `GST_USE_SYSTEM_DEPS=ON`, so `fmt` and `expected-lite` come from the
package manager rather than the vendored submodules.

**vcpkg** — the port lives in `ports/gstreamer-hpp`. Use it as an overlay until it is upstreamed:

```bash
vcpkg install gstreamer-hpp --overlay-ports=ports
```

**Conan 2** — `conanfile.py` at the repo root:

```bash
conan create . --build=missing
```

Then `find_package(gstreamer-hpp REQUIRED)` and link `gstreamer::hpp`.

## Tutorials

Step-by-step tutorials live under `tutorials/`. Each topic ships three source files and matching binaries:

| Suffix | Source | Approach |
| ------ | ------ | -------- |
| *(none)* | `main.cpp` | Raw GStreamer C API |
| `RAII` | `main_raii.cpp` | `gst::` RAII wrappers |
| `View` | `main_view.cpp` | `gst::` free functions + declarative DSL |

### Easy

| Tutorial | Problem | Fix |
| -------- | ------- | --- |
| `HelloWorld` | Verify the GStreamer + C++ setup works, with no external hardware/media | `gst_parse_launch` pipeline with `videotestsrc` |
| `VideoFilePlayer` | Play a file and shut down cleanly on EOS or error | Manual element creation, bus polling, EOS/error handling |
| `WebcamViewer` | Display live video instead of a synthetic pattern | Live capture from a V4L2 webcam |
| `AudioPlayer` | Play an audio file regardless of its source format/rate | Audio file playback with `playbin` |
| `CapsAndFilters` | Force an exact format between two elements | Caps negotiation and `capsfilter` |
| `ElementByHand` | Need element handles/branching a launch string can't give | Creating and linking elements manually |
| `StatesAndSeeking` | Pause/resume/seek and know when it's actually done | State machine, seeking, and position queries |
| `PipelineBuilder` | Avoid repeating create/add/link boilerplate per element | Declarative pipeline with `gst::PipelineDesc` / `gst::build()` |

### Medium

| Tutorial | Problem | Fix |
| -------- | ------- | --- |
| `DynamicPipeline` | Add/remove an output branch while already `PLAYING` | Dynamic pad linking with `pad-added` signal |
| `EventsAndQueries` | Read position/duration or push seek/EOS into a running pipeline | Sending events and position/duration queries |
| `BuffersAndMemory` | Inspect raw buffer bytes/timestamps flowing through a pipeline | `appsrc` / `appsink`, buffer access and mapping |
| `ClocksAndSync` | Stop playback drifting too fast/slow or out of A/V sync | Pipeline clock, base time, and A/V sync |
| `CPUVideoProcessing` | Edit raw video frames in app code (overlay, OpenCV, filters) | Per-frame CPU processing via `appsink`/`appsrc` |
| `ImageCapture` | Grab a snapshot from a live stream without stopping it | Snapshot from a live pipeline to PNG |
| `PipelineInspector` | Discover which elements/plugins/caps are available on this machine | Introspecting element pads and caps at runtime |
| `RTSPClient` | Consume a live network camera/stream | Consuming an RTSP stream with `rtspsrc` |
| `TagsAndMetadata` | Read title/artist/codec metadata without playing the whole file | Reading stream tags and metadata |
| `VideoRecorder` | Preview on screen while recording the same stream to disk | Encoding and muxing video to a file |

### Hard

| Tutorial | Problem | Fix |
| -------- | ------- | --- |
| `CustomPlugin` | No built-in element does the per-pixel transform you need | `GstBaseTransform` subclass registered at runtime |
| `CustomSourceSink` | Need a source/sink with behavior no plugin provides | `GstBaseSrc`/`GstBaseSink` subclasses registered at runtime |
| `EncodeProfiles` | Wiring `encoder ! muxer` by hand per codec/container is repetitive | `encodebin` driven by a `GstEncodingProfile` |
| `MultiCameraViewer` | Combine several camera feeds into one output frame | `compositor` with per-source request pads |
| `NetClockSync` | Keep pipelines on separate machines playing back in lockstep | Shared `GstNetTimeProvider`/`GstNetClientClock` + `base_time` alignment |
| `RTSPServer` | Publish a live stream to arbitrary RTSP clients | `GstRTSPServer` + `GstRTSPMediaFactory` |
| `TestingElements` | Unit-test a custom element without a full pipeline | `GstHarness` push/pull with byte-exact assertions |

## Dependencies

Fetched automatically via CMake `find_package` or `FetchContent`:

- [GStreamer](https://gstreamer.freedesktop.org/) (with Video component)
- [expected-lite](https://github.com/martinmoene/expected-lite) (`nonstd::expected`)
- [fmt](https://github.com/fmtlib/fmt)
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

`.devcontainer/` provides a Docker environment with GPU passthrough (`--gpus=all`), X11 forwarding, and VS Code extensions for clangd, CMake Tools, and GitLens. It's currently based on the NVIDIA DeepStream image (`nvcr.io/nvidia/deepstream:9.0-samples-multiarch`), which ships GStreamer and the full toolchain; swapping to a plain GStreamer base image is a follow-up.

**All builds, tests, and binary runs must happen inside the container** — GStreamer, `expected-lite`, and the toolchain only exist inside the image.

```bash
# Find the running container
CID=$(docker ps --format '{{.ID}} {{.Image}}' | grep -i deepstream | awk '{print $1}' | head -1)

# Run any build command inside it
docker exec -u 1000 "$CID" bash -c 'cd /workspace && cmake -B build -S . && cmake --build build'
```

## License

See [LICENSE](LICENSE).
