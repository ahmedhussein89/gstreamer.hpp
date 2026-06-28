# Roadmap: Building the DeepStream C++ Wrapper From deepstream-app

## Phase 0 — Preparation ✅ COMPLETE

**Goal:** Familiarize yourself with deepstream-app internals & define project structure.

### Tasks

* [x] Clone DeepStream SDK source and extract deepstream-app
* [x] Create repository structure (`include/`, `tests/`, `tutorials/`, `docs/`, `cmake/`)
* [x] Enable CI infrastructure:
  * [x] `.clang-format` (Google style, 130-col, C++20)
  * [x] `.clang-tidy`
  * [x] CMake modules: `sanitizers.cmake`, `coverage.cmake`, `compiler_warnings.cmake`
  * [x] DevContainer with GPU passthrough and X11 forwarding (`.devcontainer/`)
  * [ ] GitHub Actions CI (GCC + Clang builds, sanitizer runs) — not yet wired

**Deliverable:** ✅ Skeleton project with CI infrastructure, docs, and folder structure.

---

## Phase 1 — Understand & Extract the Core Pipeline Logic ✅ COMPLETE

**Goal:** Understand deepstream-app logic and extract the minimal functioning pipeline.

### Tasks

* [x] Study deepstream-app internals (reference in `examples/deepstream-app/`)
* [x] Get a basic pipeline running (tutorials: `HelloWorld`, `VideoFilePlayer`)
* [x] Standalone C++ pipeline not depending on deepstream-app

**Deliverable:** ✅ `tutorials/easy/HelloWorld` and `tutorials/easy/VideoFilePlayer` demonstrate working GStreamer pipelines.

---

## Phase 2 — Introduce RAII Wrappers for Core GStreamer Objects ✅ COMPLETE

**Goal:** Create the core C++ abstractions that manage GStreamer lifetimes.

### Tasks

* [x] `gst::Element` — RAII move-only wrapper around `GstElement*` (`include/gstreamer.hpp`)
* [x] `gst::ElementPtr` — `unique_ptr<GstElement, GstElementDeleter>`
* [x] `gst::BusPtr` — `unique_ptr<GstBus, GstBusDeleter>`
* [x] `gst::ErrorPtr` — `unique_ptr<GError, GstErrorDeleter>`
* [x] `gst::MessagePtr` — `unique_ptr<GstMessage, GstMessageDeleter>`
* [x] Automatic cleanup on destruction (custom deleters call `gst_object_unref`)
* [x] Typed property setters via `Node::prop(key, value)` chaining (`include/pipeline.hpp`)
* [x] `gst::PadPtr` RAII wrapper + `gst::element_get_static_pad()` helper (`include/gstreamer.hpp`)
* [x] `gst::CapsPtr` RAII wrapper + `gst::caps_from_string()` helper (`include/gstreamer.hpp`)
* [x] `gst::Pipeline` RAII wrapper around `GstPipeline*`; descriptor renamed `gst::PipelineDesc` (`include/gstreamer.hpp`, `include/pipeline.hpp`)

**Deliverable:** ✅ `include/gstreamer.hpp` contains all core RAII wrappers. `include/pipeline.hpp` adds declarative pipeline construction via `gst::PipelineDesc` + `gst::build()`.

---

## Phase 3 — Create Strongly-Typed DeepStream Components ✅ COMPLETE

**Goal:** Wrap all major DeepStream plugins into C++ typed classes.

### Tasks

#### Sources
* [x] `FileSource` — wraps `filesrc`; setter: `location`
* [x] `RTSPSource` — wraps `rtspsrc`; setters: `location`, `latency`, `protocols`
* [x] `CameraSource` — wraps `v4l2src`; setter: `device`

#### Transformations
* [x] `StreamMux` — wraps `nvstreammux`; setters: `batch_size`, `width`, `height`, `batched_push_timeout`, `live_source`, `sync_inputs`
* [x] `VideoConverter` — wraps `nvvideoconvert`
* [x] `OSD` — wraps `nvdsosd`; setters: `process_mode`, `display_text`, `display_bbox`, `display_mask`

#### Inference
* [x] `PrimaryInfer` — wraps `nvinfer` with `process-mode=1`; setters: `config_file`, `batch_size`, `unique_id`
* [x] `SecondaryInfer` — wraps `nvinfer` with `process-mode=2`; setters: `config_file`, `batch_size`, `unique_id`, `infer_on_id`

#### Tracking
* [x] `Tracker` — wraps `nvtracker`; setters: `lib_file`, `config_file`, `tracker_width`, `tracker_height`

#### Output
* [x] `WindowSink` — wraps `nveglglessink`; setter: `sync`
* [x] `FileSink` — wraps `filesink`; setters: `location`, `sync`

Each wrapper contains:
* Move-only RAII element (holds `gst::Element`)
* Type-safe setters returning `*this` for chaining
* Error-checked `create()` returning `nonstd::expected<T, std::string>`
* Graceful failure when DeepStream plugin is unavailable

**Deliverable:** ✅ `include/elements/` module with `ds::` namespace typed wrappers. Umbrella header `include/elements.hpp`. CMake target `ds::elements`. Tests in `tests/testElements.cpp` (31 tests; GPU-only paths skip gracefully without DeepStream).

---

## Phase 4 — Implement the Pipeline Builder ✅ COMPLETE

**Goal:** Introduce fluent pipeline creation inspired by vulkan.hpp / modern C++ builders.

### Tasks

* [x] Declarative pipeline descriptor: `gst::PipelineDesc` + `gst::Node` + `gst::build()` (`include/pipeline.hpp`)
* [x] Node property chaining: `gst::Node{"fakesrc"}.prop("num-buffers", 10)`
* [x] Internal linear element graph (nodes + sequential links)
* [x] Error-checked build: returns `nonstd::expected<Pipeline, std::string>`
* [x] `gst::element_set_state()` helper (templated; accepts any `gst::Element` or `gst::Pipeline`)
* [x] High-level fluent builder: `ds::Builder{}.add(...).add(...).build()` (`include/builder.hpp`)
* [x] Caps compatibility validation — static pad template check before linking; incompatible pairs fail early with a descriptive error
* [x] Mandatory-node validation — build() fails with a clear message if no elements are added; post-build call also fails cleanly
* [x] Unique element ID enforcement — duplicate element names detected at add() time and reported by build()

**Deliverable:** ✅ `include/builder.hpp` provides `ds::Builder` — a fluent, validated pipeline builder. Accepts any `ds::` typed element or raw `gst::Element`. Returns `nonstd::expected<gst::Pipeline, std::string>`. Tests in `tests/testBuilder.cpp` (16 tests).

---

## Phase 5 — Metadata System ✅ COMPLETE

**Goal:** Replace raw metadata parsing with modern C++ typed views.

### Tasks

* [x] `FrameMetaView` — wraps `NvDsFrameMeta`; exposes batch_id, pad_index, frame_num, buf_pts, source_id, num_objects
* [x] `ObjectMetaView` — wraps `NvDsObjectMeta`; exposes class_id, object_id, confidence, tracker_confidence, label, rect()
* [x] `BoundingBox` — plain struct with left/top/width/height + derived right()/bottom()/area()
* [x] `ClassifierMetaView` — wraps `NvDsClassifierMeta`; exposes unique_component_id, classifier_type, label iteration
* [x] `LabelInfoView` — wraps `NvDsLabelInfo`; exposes label, class_id, probability, label_id
* [x] `TensorMetaView` — wraps `NvDsInferTensorMeta`; exposes unique_id, gpu_id, output_layers range
* [x] `TensorLayerView` — wraps `NvDsInferLayerInfo`; exposes name, data_type, shape (std::span), buffer
* [x] `UserMetaView` — wraps `NvDsUserMeta`; meta_type check, as_tensor_meta() optional accessor
* [x] `BatchMetaView` — wraps `NvDsBatchMeta`; from_buffer() GstBuffer factory, frame iteration
* [x] `MetaListView<View, Native>` — generic forward-iterator range over `NvDsMetaList` (= GList)
* [x] Range-based iteration over frames, objects, classifiers, labels, and tensor output layers
* [x] User metadata extensibility via `UserMetaView::raw_data()` + `as_tensor_meta()`

Each view type:
* Non-owning (holds a raw pointer, no lifetime management)
* Read-only access to the underlying DeepStream struct
* No exceptions — all accessors are noexcept by construction

**Deliverable:** ✅ `include/metadata/` module in `ds::` namespace with typed, range-iterable views. Umbrella header `include/metadata.hpp`. CMake target `ds::metadata` (requires DeepStream SDK). Tests in `tests/testMetadata.cpp` (22 tests; built only when DeepStream is found).

---

## Phase 6 — Debug Layer & Error Handling ✅ COMPLETE

**Goal:** Introduce predictable, architect-level diagnostics.

### Tasks

* [x] `nonstd::expected<T, E>` used throughout for error propagation (no exceptions)
* [x] Validation callbacks (similar to Vulkan layers) — `ds::DebugLayer::set_callback()`
* [x] Logging macros (`DS_DEBUG`, `DS_INFO`, `DS_WARN`, `DS_ERROR`) — fmt-style, routed through `DebugLayer`
* [x] Typed error types: `ds::ErrorKind` (10 variants), `ds::Error`, `ds::ElementError`, `ds::PipelineError`
* [x] `utils/error.hpp`, `utils/debug.hpp`
* [x] spdlog integration — `ds::DebugLayer::set_logger(shared_ptr<spdlog::logger>)`

#### `utils/error.hpp`
* `ds::ErrorKind` enum — 10 structured categories (Unknown, ElementCreation, ElementLink, ElementState, NoElements, DuplicateName, IncompatibleCaps, PipelineCreation, BinAdd, ParseLaunch)
* `ds::Error` — kind + message; string-compatible interface (`empty()`, `find()`, `operator==`) for zero migration cost
* `ds::ElementError : Error` — returned by all `ds::*::create()` factories
* `ds::PipelineError : Error` — returned by `ds::Builder::build()`

#### `utils/debug.hpp`
* `ds::DebugLevel` enum (Debug / Info / Warn / Error / Off)
* `ds::DebugMessage` struct — level, kind, message, file, line
* `ds::DebugLayer` singleton — thread-safe; set_callback(), clear_callback(), set_min_level(), set_logger(); min-level filtering suppresses below-threshold messages
* `DS_DEBUG(fmt, ...)`, `DS_INFO(...)`, `DS_WARN(...)`, `DS_ERROR(...)` — dispatch through `DebugLayer`

#### Updated APIs
* All `ds::*::create()` element factories now return `nonstd::expected<T, ds::ElementError>` (fully backward compatible — `ElementError` exposes `find()` / `empty()` / `operator==`)
* `ds::Builder::build()` returns `nonstd::expected<gst::Pipeline, ds::PipelineError>` and dispatches each failure through `DebugLayer` before returning

**Deliverable:** ✅ `include/utils/error.hpp` + `include/utils/debug.hpp`. `ds::DebugLayer` validates pipeline construction at the Vulkan-layer level. Tests in `tests/testDebug.cpp` (22 tests).

---

## Phase 7 — Examples & Tests 🔶 PARTIAL

**Goal:** Add examples to prove the wrapper works end-to-end.

### Example Apps

* [x] Basic file player (`tutorials/easy/VideoFilePlayer`)
* [x] Declarative pipeline API demo (`tutorials/easy/VideoFilePlayer/main_declarative.cpp`)
* [ ] `01_basic_file_infer` (requires DeepStream/nvinfer)
* [ ] `02_multi_stream`
* [ ] `03_secondary_inference`
* [ ] `04_custom_metadata`
* [ ] `05_rtsp_input`

### Tests

* [x] `tests/testGstreamer.cpp` — unit tests for `gst::` wrappers (293 lines)
* [x] `tests/testPipeline.cpp` — unit tests for `gst::build()`, `gst::Node`, property application
* [ ] Integration test for a full DeepStream pipeline
* [ ] Metadata extraction tests

**Deliverable:** GStreamer-level unit tests complete. DeepStream-specific examples and integration tests not yet started.

---

## Phase 8 — Documentation & Release ❌ NOT STARTED

**Goal:** Publish the wrapper.

### Tasks

* [x] `README.md` with API reference and quick-start example
* [x] `docs/description.md`, `docs/requirements.md`, `docs/roadmap.md`
* [ ] Getting started guide
* [ ] Architecture overview doc
* [ ] Element list doc
* [ ] Pipeline builder tutorial
* [ ] Doxygen configuration
* [ ] Publish `v0.1.0`

**Deliverable:** Not yet published.

---

## Project Status Summary

| Phase | Description                    | Status           |
| ----- | ------------------------------ | ---------------- |
| 0     | Preparation                    | ✅ Complete       |
| 1     | Core Pipeline Logic            | ✅ Complete       |
| 2     | RAII GStreamer Wrappers         | ✅ Complete       |
| 3     | Typed DeepStream Components    | ✅ Complete       |
| 4     | Pipeline Builder               | ✅ Complete       |
| 5     | Metadata System                | ✅ Complete       |
| 6     | Debug Layer & Error Handling   | ✅ Complete       |
| 7     | Examples & Tests               | 🔶 Partial        |
| 8     | Documentation & Release        | ❌ Not started    |

**Current state:** Phases 0–6 are complete. The GStreamer abstraction, DeepStream element, fluent builder, typed metadata, and debug/error layers are all solid. The project is ready to begin Phase 7 (example apps and integration tests) or Phase 8 (documentation and release).

---

## Optional Add-ons (for later versions)

### v0.2 – JSON/YAML Pipelines

Import pipelines from files.

### v0.3 – CUDA Integration

Expose GPU buffers directly.

### v0.4 – Python bindings

Use pybind11 to expose the wrapper.

### v1.0 – Production Release

Full coverage + stable API.
