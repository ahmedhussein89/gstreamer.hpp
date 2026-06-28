# deepstream.hpp

A header-only, modern C++ wrapper for NVIDIA DeepStream — inspired by [vulkan.hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

DeepStream's raw GStreamer API is verbose, stringly-typed, and error-prone. **deepstream.hpp** wraps it with RAII resource management, strongly-typed enums, and `nonstd::expected`-based error handling — giving you explicit control over pipelines without the boilerplate.

## Status

Early development. The implemented surface today is the `gst` namespace in `include/gstreamer.hpp`. The broader `ds` namespace (typed DeepStream elements, pipeline builder, metadata views) is described in [`docs/description.md`](docs/description.md) and tracked in [`docs/roadmap.md`](docs/roadmap.md).

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

## What's implemented (`gst` namespace)

| Symbol                          | Description                                                      |
| ------------------------------- | ---------------------------------------------------------------- |
| `gst::Element`                  | Move-only RAII wrapper around `GstElement*`                      |
| `gst::ElementPtr`               | `unique_ptr<GstElement, GstElementDeleter>`                      |
| `gst::BusPtr`                   | `unique_ptr<GstBus, GstBusDeleter>`                              |
| `gst::ErrorPtr`                 | `unique_ptr<GError, GstErrorDeleter>`                            |
| `gst::MessagePtr`               | `unique_ptr<GstMessage, GstMessageDeleter>`                      |
| `gst::MessageType`              | Strongly-typed enum over `GstMessageType`; supports `\|` and `&` |
| `gst::parse_launch()`           | Returns `nonstd::expected<Element, ErrorPtr>`                    |
| `gst::message_parse_error()`    | Returns `nonstd::expected<pair<string,string>, string>`          |
| `gst::bus_timed_pop_filtered()` | Returns `nonstd::expected<MessagePtr, string>`                   |

All fallible functions return `nonstd::expected<T, E>` — no exceptions, no raw output parameters.

## Building

Requires CMake 3.20+, a C++20 compiler, GStreamer (with the Video component), and `gcovr` for coverage reports.

```bash
# Configure (builds tutorials + tests by default)
cmake -B build -S .

# Build
cmake --build build

# Run tests
cd build && ctest

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

Step-by-step tutorials live under `tutorials/`:

| Difficulty | Tutorial          | Description                                              |
| ---------- | ----------------- | -------------------------------------------------------- |
| Easy       | `HelloWorld`      | `gst_parse_launch` pipeline with `videotestsrc`          |
| Easy       | `VideoFilePlayer` | Manual element creation, bus polling, EOS/error handling |

## Dependencies

Fetched automatically via CMake FetchContent or `find_package`:

- [GStreamer](https://gstreamer.freedesktop.org/) (with Video component)
- [expected-lite](https://github.com/martinmoene/expected-lite) (`nonstd::expected`)
- [fmt](https://github.com/fmtlib/fmt)
- [spdlog](https://github.com/gabime/spdlog)
- [GoogleTest](https://github.com/google/googletest) (tests only)

## Code style

- C++20, 2-space indent, 130-column limit
- Google style base via `.clang-format`
- Most clang-tidy checks enabled (see `.clang-tidy`)
- Warnings as errors on GCC and Clang (`-Werror`)

```bash
clang-format -i include/gstreamer.hpp
clang-tidy include/gstreamer.hpp -- -I include
```

## DevContainer

`.devcontainer/` provides a Docker environment with GPU passthrough (`--gpus=all`), X11 forwarding, and VS Code extensions for clangd, CMake Tools, and GitLens.

## License

See [LICENSE](LICENSE).
