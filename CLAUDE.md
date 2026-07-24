# CLAUDE.md

## Project Overview

**gstreamer.hpp** is a header-only, modern C++20 wrapper for GStreamer, inspired by vulkan.hpp. RAII, strong typing, `nonstd::expected` error handling, a declarative pipeline DSL. The `gst` namespace is implemented and covered by tests.

Detail lives in on-demand docs — read them when the task touches that area:

- `docs/api-reference.md` — full symbol tables: `gst::` handles, `gst::raii::` types, free functions, pipeline DSL
- `docs/dev-environment.md` — devcontainer internals, dependencies, sanitizers, coverage, tutorials structure
- `docs/roadmap.md` — phase status, what's implemented vs planned

## Development Environment (host)

**Build, test, and run directly on the host** — toolchain and GStreamer are installed system-wide; invoke `cmake`/`ninja`/`ctest` from the repo root, no container wrapper. The dev container (`.devcontainer/`) remains valid for a clean-room/CI-equivalent build but isn't required.

| | |
|---|---|
| Toolchain | `cmake` 4.x, `ninja`, `make`, `g++` 15 |
| GStreamer | 1.28.x — core, `video`, `base`, `net`, `check`, `pbutils` |
| Missing | `gstreamer-rtsp-server-1.0`, `clang++`, `clang-format`, `clang-tidy` |

- `gst-rtsp-server` absent → the `RTSPServer` tutorial self-skips at configure time (`-- Skipping RTSPServer tutorial: gst-rtsp-server not found`). Expected, not a failure; install `libgstrtspserver-1.0-dev` to build it.
- `clang-format`/`clang-tidy` absent → skip formatting/lint locally and let CI enforce them, or run them in the dev container.

Verify the environment before assuming a breakage is your code:

```bash
pkg-config --modversion gstreamer-1.0 gstreamer-base-1.0 gstreamer-check-1.0
gst-inspect-1.0 compositor >/dev/null && echo "plugins ok"
```

## Build & Test Commands

Run from the repo root.

```bash
cmake -B build -S .                      # configure (default: tutorials + tests on)
cmake --build build                      # build
ctest --test-dir build                   # run all tests
./build/tests/testGstreamer              # one binary of: testGstreamer testGstreamerRaii testPipeline testConcepts testCore
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

Configure options: `-DGST_BUILD_TUTORIALS=ON -DGST_BUILD_TESTS=ON`.
Tutorials pull in vendored Tracy (`third_party/tracy`, `TRACY_ENABLE` forced on) for profiling — enabled automatically when `GST_BUILD_TUTORIALS=ON`.
Sanitizers: `-DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address|memory|thread|undefined|none`.
Coverage: `-DENABLE_COVERAGE=ON`, then `cmake --build build -t coverage` (details: `docs/dev-environment.md`).

## Code Quality

`clang-format`/`clang-tidy` are enforced by CI (`.clang-tidy`: most checks except google/llvm/abseil/android/fuchsia) but not installed on the host — run in the dev container or rely on CI:

```bash
clang-format -i include/gstreamer.hpp include/gstreamer_raii.hpp include/core/*.hpp
clang-tidy include/gstreamer.hpp -- -I include
```

`-Werror` applies only to targets linking `gstreamer::warnings_strict` (`include/`, `tests/`). Tutorials link plain `gstreamer::warnings`, so warnings there are not fatal.

## Architecture

Two-layer design, vulkan.hpp-style:

- **Enhanced layer** (`gstreamer.hpp` + `include/core/`): non-owning, trivially-copyable typed handles (`gst::Element`, `gst::Bus`, …) plus free functions returning `nonstd::expected`. `gst::Element` does **not** own.
- **RAII layer** (`gstreamer_raii.hpp`): `gst::raii::*` move-only owning types, each implicitly convertible to its non-owning handle so enhanced-layer functions accept them unchanged.
- **Pipeline DSL**: `gst::Node`/`gst::PipelineDesc` in `gstreamer.hpp`, `gst::build()` in `gstreamer_raii.hpp`.

Full symbol tables (handle types, free-function signatures, DSL): `docs/api-reference.md`.

CMake target: `gstreamer_hpp` (alias `gstreamer::hpp`) — INTERFACE library over `gstreamer.hpp`, `gstreamer_raii.hpp`, `core/*.hpp`; always built.

## Code Conventions

- C++20, 2-space indentation, 130-column limit, left-aligned pointers (`.clang-format`)
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- New API functions return `nonstd::expected<T, E>` (from `expected-lite`) — not raw pointers or exceptions; fail via `nonstd::make_unexpected(...)`
- New owning GStreamer resource wrappers go in `gstreamer_raii.hpp` as move-only classes convertible to their `gst::` handle; the custom-deleter `unique_ptr` aliases in `gstreamer.hpp` are the implementation detail behind them, not the pattern to copy
- **Every template in `include/` must constrain its type parameters with a named C++20 concept or a `requires` clause.** Unconstrained `typename T` / `class T` parameters are rejected by `scripts/check-concepts.sh` (run in CI). Shared concept vocabulary: `include/core/concepts.hpp`; header-local concepts live in that header.
