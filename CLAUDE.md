# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**deepstream.hpp** is a header-only, modern C++20 wrapper for the NVIDIA DeepStream SDK, inspired by vulkan.hpp. RAII, strong typing, `nonstd::expected` error handling, builder-pattern pipeline API. The `gst` namespace (GStreamer primitives) is implemented; the `ds` namespace (DeepStream elements, metadata, pipeline builder) is in progress — see `docs/roadmap.md` for status.

Detail lives in on-demand docs — read them when the task touches that area:

- `docs/api-reference.md` — full symbol tables: `gst::` handles, `gst::raii::` types, free functions, pipeline DSL, `ds::` elements/builder/metadata
- `docs/dev-environment.md` — devcontainer internals, dependencies, sanitizers, coverage, tutorials structure
- `docs/roadmap.md` — phase status, what's implemented vs planned

## Development Environment (Docker — REQUIRED)

**All builds, tests, and binary runs MUST happen inside the DeepStream dev container.**
Never invoke `cmake`, `make`, `ninja`, `ctest`, `clang-format`, `clang-tidy`, or any
compiled binary directly on the host — the GStreamer/DeepStream SDK, `expected-lite`,
`tracy`, and the toolchain only exist inside the container image
(`nvcr.io/nvidia/deepstream:9.0-samples-multiarch`, defined in `.devcontainer/Dockerfile`).

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
cmake -B build -S .                      # configure (default: tutorials + tests on, examples off)
cmake --build build                      # build
ctest --test-dir build                   # run all tests
./build/tests/testGstreamer              # run one test binary
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

Configure options: `-DDS_BUILD_EXAMPLES=OFF -DDS_BUILD_TUTORIALS=ON -DDS_BUILD_TESTS=ON`.
Sanitizers: `-DDS_ENABLE_SANITIZERS=ON -DDS_SANITIZER=address|memory|thread|undefined|none`.
Coverage: `-DENABLE_COVERAGE=ON`, then `cmake --build build -t coverage` (details: `docs/dev-environment.md`).

## Code Quality

Run inside the container.

```bash
# Format all headers
clang-format -i include/gstreamer.hpp include/gstreamer_raii.hpp \
  include/deepstream.hpp include/deepstream_raii.hpp \
  include/elements.hpp include/builder.hpp include/metadata.hpp \
  include/core/*.hpp include/utils/*.hpp include/elements/*.hpp include/metadata/*.hpp

# Lint (enforced via .clang-tidy — most checks enabled except google/llvm/abseil/android/fuchsia)
clang-tidy include/gstreamer.hpp -- -I include
```

Warnings are treated as errors (`-Werror`) across GCC and Clang.

## Architecture

Two-layer design, vulkan.hpp-style:

- **Enhanced layer** (`gstreamer.hpp` + `include/core/`): non-owning, trivially-copyable typed handles (`gst::Element`, `gst::Bus`, …) plus free functions returning `nonstd::expected`. `gst::Element` does **not** own.
- **RAII layer** (`gstreamer_raii.hpp`): `gst::raii::*` move-only owning types, each implicitly convertible to its non-owning handle so enhanced-layer functions accept them unchanged.
- **Pipeline DSL**: `gst::Node`/`gst::PipelineDesc` in `gstreamer.hpp`, `gst::build()` in `gstreamer_raii.hpp`. There is no `pipeline.hpp` header or `pipeline_hpp` target.
- **`ds` layer**: element factory wrappers (`include/elements/*.hpp`), `ds::Builder` (`builder.hpp`), NvDs metadata views (`include/metadata/*.hpp`, only when `DeepStream_FOUND`), structured `ds::Error` (`utils/error.hpp`).

Full symbol tables (handle types, free-function signatures, DSL, metadata views): `docs/api-reference.md`.

### CMake targets

| Target | Alias | Header(s) | Notes |
|---|---|---|---|
| `gstreamer_hpp` | `gstreamer::hpp` | `gstreamer.hpp`, `gstreamer_raii.hpp`, `core/*.hpp` | GStreamer handles + pipeline DSL + RAII layer; always built |
| `deepstream_hpp` | `ds::hpp` | `deepstream.hpp` | Umbrella over the non-owning `ds::` layer |
| `deepstream_raii_hpp` | `ds::raii` | `deepstream_raii.hpp` | Umbrella pulling `builder.hpp` + `elements.hpp`. Target name only — no `ds::raii` **namespace** yet (roadmap Phase 5) |
| `deepstream_elements` | `ds::elements` | `elements.hpp`, `builder.hpp`, `elements/*.hpp`, `utils/*.hpp` | DeepStream element wrappers; always built |
| `deepstream_metadata` | `ds::metadata` | `metadata.hpp`, `metadata/*.hpp` | NvDs metadata views; only built when `DeepStream_FOUND` |

## Code Conventions

- C++20, 2-space indentation, 130-column limit, left-aligned pointers (`.clang-format`)
- Include ordering (by priority): STL → boost → fmt → range → gst → gtest → other third-party → project headers
- New API functions return `nonstd::expected<T, E>` (from `expected-lite`) — not raw pointers or exceptions; fail via `nonstd::make_unexpected(...)`
- New owning GStreamer resource wrappers go in `gstreamer_raii.hpp` as move-only classes convertible to their `gst::` handle; the custom-deleter `unique_ptr` aliases in `gstreamer.hpp` are the implementation detail behind them, not the pattern to copy
- `gst` namespace: GStreamer primitives and pipeline DSL; `ds` namespace: DeepStream elements, metadata, and builder abstractions
- Metadata views in `include/metadata/` are `#ifdef`-guarded on DeepStream availability; don't add unconditional NvDs includes outside that tree
- **Every template in `include/` must constrain its type parameters with a named C++20 concept or a `requires` clause.** Unconstrained `typename T` / `class T` parameters are rejected by `scripts/check-concepts.sh` (run in CI). Shared concept vocabulary: `include/core/concepts.hpp`; header-local concepts live in that header.
