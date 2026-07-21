# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**gstreamer.hpp** is a header-only, modern C++20 wrapper for GStreamer, inspired by vulkan.hpp. RAII, strong typing, `nonstd::expected` error handling, a declarative pipeline DSL. The `gst` namespace is implemented and covered by tests.

Detail lives in on-demand docs — read them when the task touches that area:

- `docs/api-reference.md` — full symbol tables: `gst::` handles, `gst::raii::` types, free functions, pipeline DSL
- `docs/dev-environment.md` — devcontainer internals, dependencies, sanitizers, coverage, tutorials structure
- `docs/roadmap.md` — phase status, what's implemented vs planned

## Development Environment (Docker — REQUIRED)

**All builds, tests, and binary runs MUST happen inside the dev container.**
Never invoke `cmake`, `make`, `ninja`, `ctest`, `clang-format`, `clang-tidy`, or any
compiled binary directly on the host — GStreamer, `expected-lite`, and the toolchain
only exist inside the container image (currently `nvcr.io/nvidia/deepstream:9.0-samples-multiarch`,
defined in `.devcontainer/Dockerfile` — it also ships GStreamer and the full toolchain;
swapping to a plain GStreamer base image is a follow-up).

The container runs as non-root `developer` (`USER_UID=1000`) — always exec as UID 1000:

```bash
# Discover the container spun up from .devcontainer (name is auto-generated)
CID=$(docker ps --format '{{.ID}} {{.Image}}' | grep -i deepstream | awk '{print $1}' | head -1)

# Run any build/test command inside it, as the developer user
docker exec -u 1000 "$CID" bash -c 'cd /workspace && <command>'
```

If no container is running, start it via VS Code "Reopen in Container" (see `.devcontainer/`).

## Build & Test Commands

Every command below assumes the `docker exec -u 1000 "$CID" bash -c 'cd /workspace && ...'` wrapper.

```bash
cmake -B build -S .                      # configure (default: tutorials + tests on)
cmake --build build                      # build
ctest --test-dir build                   # run all tests
./build/tests/testGstreamer              # run one test binary
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

Configure options: `-DGST_BUILD_TUTORIALS=ON -DGST_BUILD_TESTS=ON`.
Sanitizers: `-DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address|memory|thread|undefined|none`.
Coverage: `-DENABLE_COVERAGE=ON`, then `cmake --build build -t coverage` (details: `docs/dev-environment.md`).

## Code Quality

Run inside the container.

```bash
# Format all headers
clang-format -i include/gstreamer.hpp include/gstreamer_raii.hpp include/core/*.hpp

# Lint (enforced via .clang-tidy — most checks enabled except google/llvm/abseil/android/fuchsia)
clang-tidy include/gstreamer.hpp -- -I include
```

Warnings are treated as errors (`-Werror`) across GCC and Clang.

## Architecture

Two-layer design, vulkan.hpp-style:

- **Enhanced layer** (`gstreamer.hpp` + `include/core/`): non-owning, trivially-copyable typed handles (`gst::Element`, `gst::Bus`, …) plus free functions returning `nonstd::expected`. `gst::Element` does **not** own.
- **RAII layer** (`gstreamer_raii.hpp`): `gst::raii::*` move-only owning types, each implicitly convertible to its non-owning handle so enhanced-layer functions accept them unchanged.
- **Pipeline DSL**: `gst::Node`/`gst::PipelineDesc` in `gstreamer.hpp`, `gst::build()` in `gstreamer_raii.hpp`.

Full symbol tables (handle types, free-function signatures, DSL): `docs/api-reference.md`.

### CMake targets

| Target | Alias | Header(s) | Notes |
|---|---|---|---|
| `gstreamer_hpp` | `gstreamer::hpp` | `gstreamer.hpp`, `gstreamer_raii.hpp`, `core/*.hpp` | GStreamer handles + pipeline DSL + RAII layer; always built |

## Code Conventions

- C++20, 2-space indentation, 130-column limit, left-aligned pointers (`.clang-format`)
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- New API functions return `nonstd::expected<T, E>` (from `expected-lite`) — not raw pointers or exceptions; fail via `nonstd::make_unexpected(...)`
- New owning GStreamer resource wrappers go in `gstreamer_raii.hpp` as move-only classes convertible to their `gst::` handle; the custom-deleter `unique_ptr` aliases in `gstreamer.hpp` are the implementation detail behind them, not the pattern to copy
- **Every template in `include/` must constrain its type parameters with a named C++20 concept or a `requires` clause.** Unconstrained `typename T` / `class T` parameters are rejected by `scripts/check-concepts.sh` (run in CI). Shared concept vocabulary: `include/core/concepts.hpp`; header-local concepts live in that header.
