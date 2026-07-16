# Dev Environment Details

Supplement to `CLAUDE.md` (which holds the binding rules + commands).

## DevContainer

`.devcontainer/` provides the Docker-based dev environment that **all** builds and
tests must run in:

- Base image `nvcr.io/nvidia/deepstream:9.0-samples-multiarch` with the full DeepStream SDK.
- GPU passthrough (`--gpus=all`), `--network=host`, `--ipc=host`, and X11 forwarding
  (`/tmp/.X11-unix` bind mount + `DISPLAY`) for GUI sink windows.
- Extra tooling baked in via `Dockerfile`: `ninja-build`, `libgtest-dev`, `libspdlog-dev`,
  `expected-lite` v0.10.0 and `tracy` v0.13.0 (both from source).
- Runs as a non-root `developer` user (`USER_UID=1000`); `setup_user.sh` provisions it,
  which is why host commands must exec with `-u 1000`.
- DeepStream samples are copied to `/workspace/deepstream-samples`; the repo mounts at `/workspace`.
- VS Code extensions: cpptools, clangd, CMake Tools, Python, GitLens, Git Graph.

## Dependencies (found via CMake `find_package`)

All preinstalled in the dev container image — do not install them on the host.

- `GStreamer` (with `Video` component) — via `cmake/Modules/FindGStreamer.cmake` ([docs](https://gstreamer.freedesktop.org/documentation/?gi-language=c))
- `DeepStream` — bundled in the container (SDK 9.0); enables `ds::metadata` target when found
- `expected-lite` (`nonstd::expected-lite`) — installed from source (v0.10.0) in the image
- `tracy` — profiler (v0.13.0), installed from source in the image
- `fmt` — for formatted output
- `spdlog` — for logging in `ds::elements`
- `GTest` — for tests only

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

## Tutorials structure

Tutorials live under `tutorials/easy/`, `tutorials/medium/`, `tutorials/hard/`. Each is
a standalone CMake subdirectory (build/run inside the container like everything else).

- **easy**: `HelloWorld`, `VideoFilePlayer`, `WebcamViewer`, `AudioPlayer`, `CapsAndFilters`, `ElementByHand`, `StatesAndSeeking`, `PipelineBuilder`
- **medium**: `DynamicPipeline`, `EventsAndQueries`, `BuffersAndMemory`, `ClocksAndSync`, `CPUVideoProcessing`, `ImageCapture`, `PipelineInspector`, `RTSPClient`, `TagsAndMetadata`, `VideoRecorder`
- **hard**: (none yet)

Every tutorial ships the same three sources — the dual-track contract from
`docs/tutorials.md`:

| Source | Approach |
|---|---|
| `main.cpp` | Raw imperative GStreamer C API |
| `main_raii.cpp` | `gst::raii::` owning wrappers |
| `main_view.cpp` | `gst::` non-owning handles (the enhanced layer) |
