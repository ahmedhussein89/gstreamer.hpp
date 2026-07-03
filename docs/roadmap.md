# Roadmap: `deepstream.hpp` — a `vulkan.hpp` for DeepStream

This roadmap defines how `deepstream.hpp` becomes to NVIDIA DeepStream what
[`vulkan.hpp`](https://github.com/KhronosGroup/Vulkan-Hpp) is to Vulkan: a
header-only, zero-overhead, strongly-typed C++ layer that sits directly on top
of the C API, plus a separate RAII layer that owns resources.

It is both a **guide** (Part I — the philosophy we copy from `vulkan.hpp`) and a
**plan** (Part II — the phased engineering work, with current status).

> **Decisions locked for this roadmap**
> - **Architecture:** two-layer, exactly like `vulkan.hpp` — a thin non-owning
>   *enhanced* layer (`gst::`, `ds::`) and a separate owning *RAII* layer
>   (`gst::raii::`, `ds::raii::`). This is a deliberate refactor of the current
>   single RAII-first design.
> - **Error model:** `nonstd::expected<T, E>` everywhere. We do **not** adopt
>   `vulkan.hpp`'s exception default. Where `vulkan.hpp` throws, we return
>   `unexpected`. (See §1.7.)
> - **Target:** DeepStream SDK in Docker with full GPU access, so every
>   DeepStream tutorial and integration test is authored and run end-to-end.

---

# Part I — The Guide: `vulkan.hpp` philosophy, mapped to DeepStream

`vulkan.hpp` is not "a class per object." It is a small set of consistent
principles applied uniformly. Below, each principle is stated, then mapped to
the GStreamer/DeepStream world that we wrap.

## 1.1 Zero-overhead, header-only

Every wrapper is `inline`/`constexpr`, compiles to the identical C call, and
adds no members beyond the handle it wraps. A `gst::Element` is exactly a
`GstElement*` in size and layout. No vtables, no hidden allocations, no runtime
registry unless the user opts in.

**Rule:** if a wrapper cannot compile down to the raw call, it does not belong
in the enhanced layer — push it to a helper or the builder.

## 1.2 Two layers, two namespaces

This is the single most important structural idea we adopt.

| `vulkan.hpp`                | `deepstream.hpp`                                                                    | Owns?                                           | Analogy                               |
| --------------------------- | ----------------------------------------------------------------------------------- | ----------------------------------------------- | ------------------------------------- |
| `vk::Instance` (handle)     | `gst::Element`, `gst::Pipeline`, `gst::Pad`, `gst::Caps`, `gst::Bus`, `gst::Buffer` | **No** — thin typed handle over the raw pointer | a typed `GstElement*`                 |
| `vk::raii::Instance`        | `gst::raii::Element`, `gst::raii::Pipeline`, …                                      | **Yes** — destructor unrefs/frees               | today's `gst::Element`                |
| `vk::` free funcs / methods | `gst::` free funcs / handle methods                                                 | —                                               | `gst_element_link` → `src.link(sink)` |
| `vk::su::` samples utils    | `ds::` builder + helpers                                                            | —                                               | high-level composition                |

- **Enhanced layer** (`gst::`, `ds::`): non-owning typed handles, scoped enums,
  `Flags<>` bitmasks, struct wrappers with defaults + setters, methods on
  handles, `ArrayProxy` parameters, and `expected` returns. You can hand a
  `GstElement*` in and get one out for free. This is where 95% of the API lives.
- **RAII layer** (`gst::raii::`, `ds::raii::`): each type owns its resource and,
  where relevant, its parent, and releases in the destructor. Constructed from
  the enhanced layer, implicitly convertible **to** the enhanced handle so every
  enhanced function works on a RAII object unchanged.

> **Migration note.** Today `gst::Element` is already an owning move-only type
> (see `include/gstreamer.hpp`). Under this roadmap that owning type becomes
> `gst::raii::Element`, and `gst::Element` is re-introduced as the thin
> non-owning handle. The `ElementPtr`/`BusPtr`/… `unique_ptr` aliases become the
> implementation detail behind `gst::raii::*`. See Phase 2/3.

## 1.3 Strong typing over `void*` and strings

`vulkan.hpp` turns `uint32_t` flags into `vk::Format`, `VkImageUsageFlags` into
`vk::ImageUsageFlags`. We do the same:

- **Enums** → scoped `enum class`: `GstState` → `gst::State`, `GstFlowReturn` →
  `gst::FlowReturn`, `GstStateChangeReturn` → `gst::StateChangeReturn`,
  `GstPadDirection` → `gst::PadDirection`. (`gst::MessageType` already exists.)
- **Bitmasks** → `gst::Flags<Bits>` (see §1.4): `GstSeekFlags`,
  `GstPadProbeType`, `GstBufferCopyFlags`, `GstMessageType`.
- **Element properties** → typed setters instead of stringly-typed
  `g_object_set`: `mux.batch_size(4)` not `g_object_set(m, "batch-size", 4, …)`.
- **Caps / structures** → typed field accessors returning `expected`.

## 1.4 `Flags<>` — type-safe bitmasks

Port `vulkan.hpp`'s `Flags<BitType>` template: a wrapper over the underlying
integer that only allows OR/AND/XOR with its own bit enum, is `constexpr`, and
carries a `FlagTraits` specialization listing the valid bits. Replaces the ad-hoc
`operator|`/`operator&` currently hand-written for `gst::MessageType` with one
reusable template used by every bitmask.

## 1.5 Structs mirror C structs, with defaults + fluent setters

`vulkan.hpp` structs (`vk::InstanceCreateInfo`) are layout-compatible with the C
struct, default-initialize `sType`, and expose `setXxx()` chaining. Our analogs:

- `gst::VideoInfo` ↔ `GstVideoInfo`, `gst::CapsFeatures`, config structs for
  `nvstreammux`, `nvinfer`, `nvtracker`, `nvdspreprocess`, `nvdsanalytics`.
- Setters return `*this` for chaining (already the pattern in `ds::FileSource`
  etc.). Add designated-initializer-friendly aggregate forms where possible.

## 1.6 `ArrayProxy` for count+pointer pairs and lists

`vulkan.hpp`'s `ArrayProxy<T>` accepts a single value, an initializer list, a
`std::array`, a `std::vector`, or a raw range as one `(count, ptr)` argument.
Our version:

- Wrap "add many elements", "link a chain", "select these caps".
- Wrap DeepStream **`GList`/`NvDsMetaList`** iteration as first-class C++ ranges
  (already prototyped in `metadata/meta_list_view.hpp`) — this is our `ArrayProxy`
  on the metadata side.

## 1.7 Result handling — `expected`, not exceptions

`vulkan.hpp` throws `vk::SystemError` by default and offers a
`VULKAN_HPP_NO_EXCEPTIONS` mode returning `ResultValue<T>`. **We invert the
default:** `nonstd::expected<T, E>` is the only mode. Mapping:

| `vulkan.hpp`                          | `deepstream.hpp`                                                                    |
| ------------------------------------- | ----------------------------------------------------------------------------------- |
| `throw vk::OutOfHostMemoryError`      | `return nonstd::make_unexpected(ds::Error{Kind::…, msg})`                           |
| `ResultValue<T>` (no-exceptions mode) | `nonstd::expected<T, ds::Error>` (always)                                           |
| `Result` enum                         | `ds::ErrorKind` + `gst::FlowReturn`/`gst::StateChangeReturn` for flow-level results |

Error types already exist: `ds::ErrorKind`, `ds::Error`, `ds::ElementError`,
`ds::PipelineError` (`include/utils/error.hpp`). Extend with `gst::Error` for the
GStreamer layer so `gst::` does not depend on `ds::`.

## 1.8 Explicitness and no hidden state

Like Vulkan, nothing is implicit: every element is visible, every link is
written, no automatic state transitions. The builder (§Phase 7) is sugar over
explicit calls, never a black box — it can print the exact equivalent explicit
code it will run.

## 1.9 Validation layers

`vulkan.hpp` pairs with Vulkan validation layers. Our analog is
`ds::DebugLayer` (`include/utils/debug.hpp`): a togglable, callback-driven
diagnostic layer that validates pipeline construction (missing links, missing
mandatory properties, incompatible caps, duplicate names) and routes structured
messages. Keep it strictly opt-in and zero-cost when disabled.

## 1.10 Optional code generation (the long game)

Real `vulkan.hpp` is **generated from `vk.xml`**. GStreamer's equivalent registry
is **`gst-inspect`** (GObject introspection): every element advertises its
properties (name, type, default, range), pad templates, and caps. Phase 11 adds a
generator that reads `gst-inspect` JSON and emits typed property setters and caps
templates — turning hand-written wrappers into generated ones and letting us cover
*every* element without hand-writing each. Until then, wrappers are hand-written
but follow the exact shape the generator will emit.

---

# Part II — The Plan: phases and status

Legend: ✅ complete · 🔶 partial · 🔜 next · ⬜ not started · ♻️ refactor of
existing code.

## Status at a glance

| Phase | Theme                                                             | Status                      |
| ----- | ----------------------------------------------------------------- | --------------------------- |
| 0     | Project & CI foundations                                          | ✅                           |
| 1     | Core handle model (raw → typed)                                   | ✅                           |
| 2     | **Enhanced layer** `gst::` (non-owning, Flags, enums, ArrayProxy) | ✅                           |
| 3     | **RAII layer** `gst::raii::`                                      | ✅                           |
| 4     | DeepStream enhanced elements `ds::`                               | ✅                           |
| 5     | DeepStream RAII `ds::raii::`                                      | ⬜                           |
| 6     | Metadata views                                                    | ✅                           |
| 7     | Pipeline builder / DSL                                            | 🔶                           |
| 8     | Debug / validation layer                                          | ✅                           |
| 9     | Tests, examples, integration                                      | 🔶                           |
| 10    | Tutorials (C + wrapper, full coverage)                            | 🔶 → see `docs/tutorials.md` |
| 11    | Codegen, docs, packaging, release                                 | ⬜                           |

---

## Phase 0 — Foundations ✅

- [x] Repo layout (`include/`, `tests/`, `tutorials/`, `docs/`, `cmake/`).
- [x] `.clang-format` (C++20, 130 col), `.clang-tidy`, `-Werror` on GCC+Clang.
- [x] `sanitizers.cmake`, `coverage.cmake`, `compiler_warnings.cmake`.
- [x] DevContainer with `--gpus=all` + X11.
- [ ] **GitHub Actions CI** (GCC + Clang, sanitizer + coverage runs, a
      DeepStream-image job). *Only remaining item.*

## Phase 1 — Core handle model ✅

**Goal:** one uniform handle concept underneath both layers.

- [x] Custom-deleter `unique_ptr` aliases (`ElementPtr`, `BusPtr`, `ErrorPtr`,
      `MessagePtr`, `PadPtr`, `CapsPtr`).
- [x] `Handle<T>` concept: trivially-copyable non-owning wrapper holding one raw
      pointer, with `get()`, `operator bool`, `operator==`, implicit from/to raw
      pointer. Lives in `include/core/handle.hpp`.
- [x] Owning `gst::Element` split into non-owning handle (Phase 2) + owning
      `gst::raii::Element` (Phase 3).

## Phase 2 — Enhanced layer `gst::` ✅ **(pivotal refactor — complete)**

**Goal:** the thin, non-owning, strongly-typed layer — our `vulkan.hpp` core.

- [x] `gst::Handle<T>` base and typed non-owning handles: `Element`, `Pipeline`,
      `Bin`, `Pad`, `GhostPad`, `Caps`, `Bus`, `Message`, `Buffer`, `Structure`
      — in `include/gstreamer.hpp` and `include/core/handle.hpp`.
- [x] `gst::Flags<Bits>` + `FlagTraits` (§1.4) in `include/core/flags.hpp`;
      `MessageType` migrated to `Flags<MessageType>` → `MessageTypeFlags`.
- [x] Scoped enums `gst::State`, `StateChangeReturn`, `FlowReturn`,
      `PadDirection`, `PadPresence`, `Format`, `SeekType` with `to_string()`
      in `include/core/enums.hpp`.
- [x] `gst::ArrayProxy<T>` (§1.6) in `include/core/array_proxy.hpp`.
- [x] Free functions updated to non-owning handle parameters; `Pipeline →
      Element` implicit conversion so a pipeline passes anywhere an element is.
- [ ] Handle **methods** (`element.link(sink)`, `element.set_state(…)`, etc.)
      — deferred to a follow-up; free functions remain the primary API.
- [ ] `gst::SeekFlags`, `PadProbeType`, `BufferCopyFlags` as `Flags<>` types.
- [ ] `gst::VideoInfo`, `gst::Structure` typed field get/set.
- [ ] `gst::Error` type (so `gst::` has zero `ds::` dependency).

**Deliverable:** `include/gstreamer.hpp` becomes the enhanced layer. Every
existing test that used the owning `gst::Element` migrates to `gst::raii::` or to
the non-owning handle as appropriate. **API-compat shim** kept for one minor
version.

## Phase 3 — RAII layer `gst::raii::` ✅

**Goal:** owning handles, one destructor rule per type — the `vk::raii::` analog.

- [x] `gst::raii::Element`, `Pipeline`, `Bus`, `Pad`, `Caps`, `Message` —
      move-only, unref/free in destructor, implicitly convertible to the
      matching `gst::` enhanced handle. Lives in `include/gstreamer_raii.hpp`.
- [x] Factory functions: `gst::raii::element_factory_make(…)`,
      `pipeline_new(…)`, `parse_launch(…)` → owning types.
- [x] `gst::raii::bin_add(Pipeline&, Element)` → transfers ownership into bin,
      returns non-owning `gst::Element` handle for subsequent linking.
- [x] `gst::raii::element_get_bus`, `element_get_static_pad` → owning types.
- [x] `gst::raii::Pipeline` returned from `gst::build()` and `ds::Builder::build()`.
- [x] `ds::` element classes (`FileSource`, `FileSink`, etc.) now store
      `gst::raii::Element` internally.
- [x] 133/133 tests pass after full migration.
- [ ] `gst::raii::Bin`, `Buffer`, `Sample` (not yet needed).
- [ ] `release()` + `[[nodiscard]]` encoding of "transfer full/none" semantics
      more explicitly in docs.

**Deliverable:** `include/gstreamer_raii.hpp`. The two headers together == the
`vulkan.hpp` / `vulkan_raii.hpp` pairing.

## Phase 4 — DeepStream enhanced elements `ds::` ✅

**Goal:** typed wrappers for every DeepStream plugin, shaped like Phase 2 handles.

Implemented (`include/elements/`):
- `FileSource`, `RTSPSource`, `CameraSource` (`sources.hpp`)
- [x] `UriSource` (`nvurisrcbin`), `MultiUriSource` (`nvmultiurisrcbin`), `V4L2Decoder` (`nvv4l2decoder`)
- `StreamMux` + `StreamMuxConfig` (`transformations.hpp`)
- [x] `StreamDemux` (`nvstreamdemux`), `Tiler` (`nvmultistreamtiler`)
- `VideoConverter`, `OSD` (`transformations.hpp`)
- `PrimaryInfer`, `SecondaryInfer` + `NvInferConfig` (`inference.hpp`)
- [x] `InferServer` (`nvinferserver`), `Preprocess` + `PreprocessConfig` (`nvdspreprocess`), `Analytics` + `AnalyticsConfig` (`nvdsanalytics`)
- `Tracker` + `TrackerConfig` (`tracking.hpp`)
- `WindowSink`, `FileSink` (`sinks.hpp`)
- [x] `H264Encoder` (`nvv4l2h264enc`), `H265Encoder` (`nvv4l2h265enc`), `RtspOutSink` (`nvrtspoutsink`), `FakeSink` (`encode.hpp`)
- [x] `MsgConv` (`nvmsgconv`), `MsgBroker` (`nvmsgbroker`) (`messaging.hpp`)
- [x] `SegVisual` (`nvsegvisual`), `OpticalFlow` (`nvof`), `OpticalFlowVisual` (`nvofvisual`), `Dewarper` (`nvdewarper`) (`auxiliary.hpp`)
- [x] `SmartRecord` signal helper (`smart_record.hpp`)
- [x] Property structs (§1.5): `StreamMuxConfig`, `NvInferConfig`, `TrackerConfig`, `PreprocessConfig`, `AnalyticsConfig`

Remaining items (deferred):
- [ ] `RtspInThenDecode` composite helper (superseded by `UriSource`/`nvurisrcbin`)
- [ ] `EncodeToFile` encodebin-style composite helper
- [ ] `MsgSink` alias / `nvinferaudio` audio inference element
- [ ] Legacy vs new `nvstreammux` mode selection on `StreamMux`

**Deliverable:** `include/elements/*.hpp` + umbrella `include/elements.hpp`,
target `ds::elements`. GPU-only paths skip gracefully when DeepStream is absent.

## Phase 5 — DeepStream RAII `ds::raii::` ⬜

- [ ] Mirror Phase 3 for `ds::` elements: owning wrappers in `ds::raii::` that
      convert to the `ds::` enhanced element and to `gst::Element`.
- [ ] Config-file resources (GIE/tracker/analytics configs) as RAII-loaded typed
      config objects with validation on load (`expected`).

## Phase 6 — Metadata views ✅

Zero-cost non-owning views over `NvDs*` (`include/metadata/`): `BatchMetaView`,
`FrameMetaView`, `ObjectMetaView`, `ClassifierMetaView`, `LabelInfoView`,
`TensorMetaView`, `TensorLayerView`, `UserMetaView`, `MetaListView`,
`BoundingBox`. Range-iterable. Built only when DeepStream is found.

Follow-ups (not blocking):
- [ ] Writable meta helpers (acquire/add object & user meta with pool RAII).
- [ ] `DisplayMeta` view (`NvDsDisplayMeta`) for OSD text/lines/rects.
- [ ] `NvBufSurface` typed view + optional CUDA/`cv::Mat` zero-copy adapter.

## Phase 7 — Pipeline builder / DSL 🔶

**Goal:** fluent, validated composition — sugar over Phase 2/3, never magic.

- [x] `gst::PipelineDesc` + `gst::Node.prop(...)` + `gst::build()` (linear).
- [x] `ds::Builder{}.add(...).build()` with caps check, duplicate-name check,
      mandatory-node check → `expected<gst::Pipeline, ds::PipelineError>`.
- [ ] **Domain chain methods** from `description.md`:
      `.source<FileSource>(...).mux().infer(cfg).tracker(cfg).osd().sink()`.
- [ ] Branching topologies (tee/queue, demux→tiler, N sources → mux).
- [ ] `explain()` — print the explicit element/link/property calls the builder
      will execute (supports §1.8).
- [ ] Optional import/export: build a `PipelineDesc` from a YAML/JSON file
      (v0.2 add-on) and dump `gst-launch` string.

## Phase 8 — Debug / validation layer ✅

`ds::DebugLayer` singleton (thread-safe, opt-in), `DS_DEBUG/INFO/WARN/ERROR`,
spdlog + callback routing, min-level filter, structured `ds::Error*` types.
Follow-up: a `gst::DebugLayer` sibling so the GStreamer layer can validate
independently of `ds::`.

## Phase 9 — Tests, examples, integration 🔶

- [x] `tests/testGstreamer.cpp`, `testPipeline.cpp`, `testBuilder.cpp`,
      `testElements.cpp`, `testMetadata.cpp`, `testDebug.cpp`.
- [ ] Tests for the new **two-layer** API: handle non-ownership, RAII destruction
      order, Flags algebra, ArrayProxy overloads.
- [ ] **DeepStream integration tests** (now feasible — full GPU in Docker):
      basic file→infer→osd→sink; multi-stream mux; SGIE on PGIE; metadata
      extraction correctness; msgbroker payload shape.
- [ ] `GstHarness`-based unit tests for any custom element (Phase 10 hard track).
- [ ] Micro-benchmarks: throughput/latency vs. raw C (prove zero-overhead).

## Phase 10 — Tutorials 🔶

The full dual-track (C API **and** `deepstream.hpp`) curriculum covering every
GStreamer and DeepStream concept lives in **[`docs/tutorials.md`](tutorials.md)**.
Summary of the contract:
- Every tutorial ships a `main.cpp` (**raw C GStreamer/DeepStream**) and a
  wrapper version; where instructive, both an enhanced (`main_enhanced.cpp`) and
  RAII/declarative (`main_raii.cpp` / `main_declarative.cpp`) variant.
- Each has a `README.md`: concept, pipeline diagram, C-vs-wrapper diff, exercises.
- Tiers: Easy (GStreamer fundamentals) → Medium (real GStreamer apps) → Hard
  (production GStreamer + custom plugins) → **DeepStream** (inference, tracking,
  analytics, brokers, multi-stream) → **Capstone** (DeepStream-style framework).

## Phase 11 — Codegen, docs, packaging, release ⬜

- [ ] **`gst-inspect` code generator** (§1.10): emit typed property setters, caps
      templates, and enum wrappers from introspection JSON.
- [ ] Doxygen + a docs site; "Getting started", "Architecture", "Two-layer
      guide", per-element reference.
- [ ] Packaging: CMake `install()` + `find_package(deepstream_hpp)`, then vcpkg /
      Conan ports.
- [ ] Releases: `v0.1.0` (two-layer `gst::` + `ds::` + docs), `v0.2.0`
      (YAML/JSON pipelines), `v0.3.0` (CUDA/NvBufSurface zero-copy), `v0.4.0`
      (pybind11 bindings), `v1.0.0` (codegen coverage + stable API).

---

## Appendix A — File layout target

```
include/
  gstreamer.hpp          # Phase 2 — enhanced gst:: (non-owning handles)
  gstreamer_raii.hpp     # Phase 3 — gst::raii:: (owning)
  deepstream.hpp         # umbrella: pulls elements + metadata (enhanced ds::)
  deepstream_raii.hpp    # umbrella: ds::raii::
  pipeline.hpp           # DSL: PipelineDesc / build()
  builder.hpp            # ds::Builder fluent + validation
  elements.hpp           # umbrella for elements/*
  elements/{sources,transformations,inference,tracking,sinks,messaging}.hpp
  metadata.hpp           # umbrella for metadata/*
  metadata/*.hpp
  utils/{error,debug}.hpp
  core/{handle,flags,array_proxy}.hpp   # NEW — shared enhanced-layer primitives
```

## Appendix B — `vulkan.hpp` → `deepstream.hpp` cheat-sheet

| You want…                 | `vulkan.hpp` idiom                     | `deepstream.hpp`              |
| ------------------------- | -------------------------------------- | ----------------------------- |
| A typed non-owning handle | `vk::Image`                            | `gst::Element`                |
| An owning handle          | `vk::raii::Image`                      | `gst::raii::Element`          |
| A bitmask                 | `vk::ImageUsageFlags`                  | `gst::Flags<SeekFlagBits>`    |
| A create struct           | `vk::ImageCreateInfo{}.setExtent(...)` | `gst::VideoInfo{}.width(...)` |
| Many args as one          | `vk::ArrayProxy<const T>`              | `gst::ArrayProxy<T>`          |
| Fallible call             | `throw` / `ResultValue`                | `nonstd::expected<T, Error>`  |
| Validation                | validation layers                      | `ds::DebugLayer`              |
| Generated from spec       | `vk.xml`                               | `gst-inspect` JSON (Phase 11) |

## Appendix C — Optional add-ons (post-1.0)

- **v0.2** JSON/YAML pipeline import/export.
- **v0.3** CUDA/`NvBufSurface` zero-copy buffer access; `cv::Mat`/`cv::cuda`
  adapters.
- **v0.4** pybind11 Python bindings mirroring the two-layer API.
- **v1.0** full generated element coverage, ABI/API stability guarantee.
