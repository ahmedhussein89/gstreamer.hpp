---
name: gst-hpp-build-test
description: Configure, build, test, sanitize, and lint the gstreamer.hpp repo. Use when running cmake/ninja/ctest, enabling sanitizers or coverage, reproducing a CI failure locally, diagnosing a configure or link error, or deciding whether a failure is the code or the host environment.
owner: ahmedhussein89/gstreamer.hpp
service: gstreamer-hpp
version: 1.0.0
reviewed: 2026-08-01
license: same-as-repo
---

# Build, Test & Lint

Build **directly on the host** — the toolchain and GStreamer are installed system-wide. The
devcontainer (`.devcontainer/`) is still valid for a clean-room / CI-equivalent build but is not
required. Run everything from the repo root.

## Pre-flight

Run this before assuming a failure is your code. Several "breakages" on this host are expected
environment gaps.

```bash
bash <skill-path>/scripts/preflight.sh
```

It reports toolchain versions, GStreamer module versions, plugin availability, submodule state, and
which optional tools are missing — then tells you which of those are expected.

| | |
|---|---|
| Toolchain | cmake 4.x, ninja, make, g++ 15 |
| GStreamer | 1.28.x — core, video, base, net, check, pbutils |
| **Expected to be absent** | `gstreamer-rtsp-server-1.0`, `clang++`, `clang-format`, `clang-tidy` |

Consequences of the gaps: the `RTSPServer` tutorial self-skips at configure time
(`-- Skipping RTSPServer tutorial: gst-rtsp-server not found`) — that is success, not failure — and
formatting/lint cannot run locally, so CI is the enforcer.

## The loop

```bash
cmake -B build -S .                        # configure (tutorials + tests ON by default)
cmake --build build                        # build
ctest --test-dir build --output-on-failure # test

./build/tests/testGstreamer                # one binary
./build/tests/testGstreamer --gtest_filter="GstreamerTest.ParseLaunchValidSimplePipeline"
```

Test binaries: `testGstreamer`, `testGstreamerRaii`, `testPipeline`, `testConcepts`, `testCore`.

## Configure options

| Option | Default | Notes |
|---|---|---|
| `GST_BUILD_TUTORIALS` | `ON` | Pulls in vendored Tracy (`third_party/tracy`, `TRACY_ENABLE` forced on) |
| `GST_BUILD_TESTS` | `ON` | GTest suite |
| `GST_ENABLE_SANITIZERS` | `OFF` | |
| `GST_SANITIZER` | `address` | `address` \| `memory` \| `thread` \| `undefined` \| `none` |
| `ENABLE_COVERAGE` | `OFF` | then `cmake --build build -t coverage` (needs `gcovr`) |

Turn tutorials off for a fast header/test iteration — it skips the Tracy build entirely:

```bash
cmake -B build-fast -S . -DGST_BUILD_TUTORIALS=OFF && cmake --build build-fast
```

## Sanitizers

Use a **separate build directory** — the sanitizer flags are baked in at configure time.

```bash
cmake -B build-asan -S . -DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address
cmake --build build-asan && ctest --test-dir build-asan --output-on-failure
```

Any change touching ownership or refcounts must pass under ASan before it is done.

GStreamer's registry and type system allocate once and never free; those show as "still
reachable"/one-time leaks and are **not** this library's bugs. Judge by the delta against a clean
run on `main`, not the absolute count. `GST_DEBUG=0` keeps sanitizer output readable.

`address` and `undefined` are the useful ones here. `memory` (MSan) needs an MSan-instrumented libc++
*and* GStreamer; without them it reports false positives inside GLib.

## Lint — what CI actually enforces

CI's `lint` job runs **before** build/test and gates it. Three checks:

```bash
find include -name '*.hpp' -print0 | xargs -0 clang-format-22 --dry-run -Werror   # not available on host
bash scripts/check-concepts.sh                                                    # runs anywhere
python3 <path>/gst-hpp-wrap-api/scripts/check_wrapper_conventions.py              # repo conventions
```

`check-concepts.sh` needs only bash — **always run it before pushing.** Expected output:
`OK: All template type parameters in include/ are constrained.` Its most common false alarm is a
template head split across lines; keep heads on one line.

`clang-format`/`clang-tidy` cannot run on this host. Match `.clang-format` by hand: 2-space indent,
130 columns, left-aligned pointers, `if(` with no space, four spaces before a trailing comment.
Include order: STL → boost → fmt → range → gst → gtest → other third-party → project.

## Warning levels are not uniform

| Target links | Effect |
|---|---|
| `gstreamer::warnings_strict` | `-Werror` — `include/`, `tests/` |
| `gstreamer::warnings` | warnings only — tutorials |

Never weaken a header to silence a tutorial warning. Fix the tutorial.

## Reproducing CI locally

CI installs only `libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base
gstreamer1.0-plugins-good libgtest-dev libspdlog-dev libfmt-dev`, and builds both `Release` and
`Debug`. Two consequences:

- A test using an element outside `-base`/`-good` passes locally and fails CI.
- A `Debug`-only or `Release`-only failure is real — build both before blaming CI.

```bash
for cfg in Release Debug; do
  cmake -B "build-$cfg" -S . -DCMAKE_BUILD_TYPE=$cfg && cmake --build "build-$cfg" \
    && ctest --test-dir "build-$cfg" --output-on-failure || echo "FAILED: $cfg"
done
```

CI also checks out submodules (`with: submodules: true`). If `nonstd/expected.hpp` or `fmt` is
missing locally: `git submodule update --init --recursive`.

## Failure triage

| Message | Verdict |
|---|---|
| `Skipping RTSPServer tutorial: gst-rtsp-server not found` | Expected on this host — not a failure |
| `clang-format: command not found` | Expected — CI enforces |
| `fatal error: nonstd/expected.hpp: No such file` | `git submodule update --init --recursive` |
| `undefined reference to gst_harness_*` | Target must link `GStreamer::Check` itself |
| `check-concepts.sh` flags constrained code | Multi-line template head — collapse it |
| Warning became an error in a tutorial | Tutorial linked `warnings_strict`; use `warnings` |
| Passes locally, fails CI | Element outside `-plugins-base`/`-good`, or a display-requiring sink |
| Test hangs | Bus pop with `kClockTimeNone` and no message — use a finite timeout |

Deeper runtime/refcount symptoms: `gstreamer-hpp-dev/references/troubleshooting.md`.
