# Tutorials — the full GStreamer + DeepStream curriculum

A dual-track, concept-complete curriculum. Every tutorial teaches **one new core
concept**, and every tutorial is written **twice**:

1. **C track** — raw GStreamer / DeepStream C API (`main.cpp`). This is the
   ground truth: what the wrapper actually calls.
2. **Wrapper track** — `deepstream.hpp`. Depending on what the tutorial teaches,
   one or more of:
   - `main_enhanced.cpp` — the non-owning enhanced layer (`gst::` / `ds::`).
   - `main_raii.cpp` — the owning RAII layer (`gst::raii::` / `ds::raii::`).
   - `main_declarative.cpp` — the DSL / builder (`gst::build`, `ds::Builder`).
   - `main_dynamic.cpp` — dynamic-pad / runtime variants where relevant.

The point of the pairing is pedagogical: the reader sees the verbose C code and
the exact same pipeline expressed through the wrapper, side by side, so the value
of each abstraction is concrete.

## The contract for every tutorial

Each tutorial directory contains:

- `README.md` — new concept(s), an ASCII pipeline diagram, the **C-vs-wrapper
  diff**, how to run, expected output, and 2–3 exercises.
- `main.cpp` — C track.
- one or more wrapper-track files (see above).
- `CMakeLists.txt` — builds every variant as a separate target; DeepStream
  tutorials guard on `DeepStream_FOUND`.

Legend below: ✅ exists · 🔶 partial (C or wrapper only) · ⬜ planned.

---

# Tier 1 — Easy: GStreamer fundamentals

> Goal: how a pipeline is built from elements, pads, caps, a bus, and states.

| # | Tutorial | New concept | C | Wrapper | Status |
|---|---|---|---|---|---|
| 1 | HelloWorld | init, `parse_launch`, main loop, bus poll | ✅ | ✅ | ✅ |
| 2 | PipelineBuilder | runtime pipeline strings, factories | ✅ | ✅ | ✅ |
| 3 | VideoFilePlayer | `decodebin`, dynamic pads, EOS, state machine | ✅ | ✅ (raii/decl/dynamic) | ✅ |
| 4 | AudioPlayer | audio path, `audioconvert`/`audioresample` | ✅ | ✅ | ✅ |
| 5 | WebcamViewer | live source, caps negotiation, framerate | ✅ | ✅ | ✅ |
| 6 | ElementByHand ⬜ | build/link elements manually (no `parse_launch`), floating refs, `bin_add` transfer | ⬜ | ⬜ | ⬜ |
| 7 | StatesAndSeeking ⬜ | full state machine, `seek`, `SeekFlags`, position/duration queries | ⬜ | ⬜ | ⬜ |
| 8 | CapsAndFilters ⬜ | `capsfilter`, `gst::Caps`, structures, negotiation failures | ⬜ | ⬜ | ⬜ |

**Concepts covered by tier end:** initialization, elements & factories, static
vs dynamic (sometimes) pads, pad linking, floating references & ownership
transfer, caps and negotiation, the bus and message types, the state machine,
seeking and queries, live vs non-live clocks.

---

# Tier 2 — Medium: real GStreamer applications

> Goal: branching, timing, buffers, app integration, introspection, networking.

| # | Tutorial | New concept | C | Wrapper | Status |
|---|---|---|---|---|---|
| 1 | VideoRecorder | `tee` + `queue`, branching, encode, `mp4mux`, filesink | ✅ | ✅ | ✅ |
| 2 | RTSPClient | network source, latency/jitter, chained dynamic pads | ✅ | ✅ | ✅ |
| 3 | CPUVideoProcessing | `appsink`/`appsrc`, per-frame CPU access, buffer map | ✅ | ✅ | ✅ |
| 4 | ImageCapture | pad **probes**, single-frame snapshot, JPEG/PPM | ✅ | ✅ | ✅ |
| 5 | PipelineInspector | the **registry**: plugins, factories, pad templates, caps | ✅ | ✅ | ✅ |
| 6 | Buffers & Memory ⬜ | `GstBuffer`, `GstMemory`, `map`/`unmap`, `GstMeta`, timestamps | ⬜ | ⬜ | ⬜ |
| 7 | Events & Queries ⬜ | send/receive events, custom events, latency query, QoS | ⬜ | ⬜ | ⬜ |
| 8 | ClocksAndSync ⬜ | pipeline clock, base time, A/V sync, `sync=true/false` | ⬜ | ⬜ | ⬜ |
| 9 | TagsAndMetadata ⬜ | `GstTagList`, `taginfo`, `GstDiscoverer` media probing | ⬜ | ⬜ | ⬜ |
| 10 | DynamicPipeline ⬜ | add/remove branches while PLAYING, pad blocking, `pad probes` for reconfig | ⬜ | ⬜ | ⬜ |

**Concepts covered by tier end:** tee/queue branching, encoders/muxers,
app↔pipeline data exchange, pad probes (buffer/event/idle), the plugin registry
and introspection, buffers/memory/metas, events & queries, clocks and A/V sync,
tags and discovery, and safe dynamic pipeline reconfiguration.

---

# Tier 3 — Hard: production GStreamer + plugin development

> Goal: multi-source composition, servers, and writing your own elements.

| # | Tutorial | New concept | C | Wrapper | Status |
|---|---|---|---|---|---|
| 1 | MultiCameraViewer ⬜ | `compositor`/`videomixer`, N live sources, sync | ⬜ | ⬜ | ⬜ |
| 2 | EncodeProfiles ⬜ | `encodebin`, `GstEncodingProfile`, container/codec selection | ⬜ | ⬜ | ⬜ |
| 3 | RTSPServer ⬜ | `gst-rtsp-server`, RTP payloading, mount points | ⬜ | ⬜ | ⬜ |
| 4 | NetClockSync ⬜ | `GstNetClock`, multi-machine synchronized playback | ⬜ | ⬜ | ⬜ |
| 5 | CustomPlugin ⬜ | `GstBaseTransform` element (`MyEdgeDetector`), pad templates, negotiation, registration | ⬜ | ⬜ | ⬜ |
| 6 | CustomSourceSink ⬜ | `GstBaseSrc`/`GstBaseSink` subclassing | ⬜ | ⬜ | ⬜ |
| 7 | TestingElements ⬜ | `GstHarness` unit testing, `GstCheck`, `GST_TRACERS`, leaks/latency tracers | ⬜ | ⬜ | ⬜ |

**Concepts covered by tier end:** multi-stream compositing, encoding profiles,
RTSP serving, distributed clock sync, and the full custom-element story
(`GstBaseTransform`/`BaseSrc`/`BaseSink`, caps negotiation, plugin registration,
and testing/tracing). This tier is the bridge to understanding *why* DeepStream's
`nv*` elements look the way they do.

---

# Tier 4 — DeepStream (requires DeepStream SDK + GPU)

> Goal: the DeepStream pipeline — batching, inference, tracking, analytics,
> metadata, and messaging. Every tutorial has a C track (raw `nv*` elements +
> `NvDs*` metadata C API) and a `ds::` wrapper track.

| # | Tutorial | New concept | C | Wrapper | Status |
|---|---|---|---|---|---|
| 1 | HelloDeepStream ⬜ | `nvstreammux` batching, buffer flow, `NvBufSurface` basics | ⬜ | ⬜ | ⬜ |
| 2 | PrimaryInference ⬜ | `nvinfer` PGIE, config files, engine build, `nvdsosd` overlay | ⬜ | ⬜ | ⬜ |
| 3 | MetadataWalk ⬜ | walk `NvDsBatch→Frame→Object` meta; C `GList` vs `ds::…View` ranges | ⬜ | ⬜ | ⬜ |
| 4 | Tracking ⬜ | `nvtracker` (IOU/NvDCF/DeepSORT), `object_id`, config | ⬜ | ⬜ | ⬜ |
| 5 | SecondaryInference ⬜ | SGIE on PGIE objects, `infer-on-gie-id`, classifier meta | ⬜ | ⬜ | ⬜ |
| 6 | MultiStream ⬜ | N sources → mux → tiler (`nvmultistreamtiler`) → demux | ⬜ | ⬜ | ⬜ |
| 7 | Preprocess & CustomParser ⬜ | `nvdspreprocess` ROIs; custom bbox output parser; `NvDsInferTensorMeta` | ⬜ | ⬜ | ⬜ |
| 8 | Analytics ⬜ | `nvdsanalytics` ROI / line-crossing / direction / overcrowding | ⬜ | ⬜ | ⬜ |
| 9 | AddCustomMeta ⬜ | acquire/attach `NvDsUserMeta`, custom struct + copy/release funcs | ⬜ | ⬜ | ⬜ |
| 10 | Messaging ⬜ | `nvmsgconv` + `nvmsgbroker` (Kafka/AMQP), event schema, payload | ⬜ | ⬜ | ⬜ |
| 11 | RTSPInOut ⬜ | `nvurisrcbin`/`nvmultiurisrcbin` in, HW encode + RTSP out | ⬜ | ⬜ | ⬜ |
| 12 | SmartRecord ⬜ | event-triggered recording start/stop via signals | ⬜ | ⬜ | ⬜ |
| 13 | Triton (nvinferserver) ⬜ | `nvinferserver` / Triton backend vs native TensorRT | ⬜ | ⬜ | ⬜ |
| 14 | OpticalFlow & Segmentation ⬜ | `nvof`/`nvofvisual`, `nvsegvisual` seg masks meta | ⬜ | ⬜ | ⬜ |
| 15 | AudioInference ⬜ | `nvinferaudio` audio classification path | ⬜ | ⬜ | ⬜ |

**Concepts covered by tier end:** stream muxing/batching, `NvBufSurface`,
primary & secondary inference, tensor/classifier/label metadata, tracking,
tiling/demux for multi-stream, preprocessing + custom parsers, analytics rules,
custom user metadata with proper copy/release, message brokers and event schema,
hardware encode + RTSP egress, smart record, Triton inference, optical flow,
segmentation, and audio inference. This is the "every aspect of DeepStream" goal.

---

# Tier 5 — Capstone: a DeepStream-style framework

> Goal: assemble everything into the flagship API from `docs/description.md`,
> proving the two-layer wrapper design.

**CapstoneFramework ⬜** — build (and let students extend) a typed, validated
pipeline framework so this compiles and runs end-to-end:

```cpp
ds::Builder{}
  .source<ds::FileSource>("video.mp4")
  .mux(ds::StreamMuxConfig{}.batch_size(1).width(1920).height(1080))
  .infer("pgie_config.txt")
  .tracker("tracker_config.yml")
  .infer_secondary("sgie_carcolor.txt")
  .analytics("analytics_config.txt")
  .osd()
  .tiler(/*rows*/2, /*cols*/2)
  .broker("kafka://localhost:9092", "events.schema")
  .sink<ds::WindowSink>()
  .build();               // → nonstd::expected<gst::Pipeline, ds::PipelineError>
```

Topics: domain builder chain methods, graph validation, strong typing,
`explain()` output vs raw C, production logging (`ds::DebugLayer`), configuration
loading, metrics/latency, plugin discovery via the registry, and unit tests with
`GstHarness`. The C track for the capstone is the corresponding hand-written
`deepstream-app`-style program, so the reader sees exactly what the framework
saves.

---

# Concept coverage matrix (index)

Where to look for each core topic. "C+W" = both tracks present.

## GStreamer

| Concept | Tutorial(s) |
|---|---|
| init, main loop, bus | Easy/HelloWorld |
| `parse_launch` / runtime strings | Easy/PipelineBuilder |
| decodebin & dynamic pads | Easy/VideoFilePlayer, Medium/RTSPClient |
| audio path | Easy/AudioPlayer |
| live sources & caps | Easy/WebcamViewer, Easy/CapsAndFilters |
| manual element wiring, ownership/floating refs | Easy/ElementByHand |
| states, seeking, queries | Easy/StatesAndSeeking |
| tee/queue branching | Medium/VideoRecorder |
| appsink/appsrc | Medium/CPUVideoProcessing |
| pad probes | Medium/ImageCapture, Medium/DynamicPipeline |
| registry / introspection | Medium/PipelineInspector |
| buffers, memory, metas | Medium/Buffers&Memory |
| events & queries, QoS | Medium/Events&Queries |
| clocks & A/V sync | Medium/ClocksAndSync |
| tags & discovery | Medium/TagsAndMetadata |
| dynamic reconfig while PLAYING | Medium/DynamicPipeline |
| compositor / multi-source | Hard/MultiCameraViewer |
| encoding profiles / encodebin | Hard/EncodeProfiles |
| RTSP server | Hard/RTSPServer |
| net clock sync | Hard/NetClockSync |
| custom plugins (Transform/Src/Sink) | Hard/CustomPlugin, Hard/CustomSourceSink |
| testing & tracers | Hard/TestingElements |

## DeepStream

| Concept | Tutorial(s) |
|---|---|
| streammux / batching / NvBufSurface | DS/HelloDeepStream |
| primary inference + OSD | DS/PrimaryInference |
| metadata traversal | DS/MetadataWalk |
| tracking | DS/Tracking |
| secondary inference / classifiers | DS/SecondaryInference |
| multi-stream / tiler / demux | DS/MultiStream |
| preprocess & custom parsers / tensor meta | DS/Preprocess&CustomParser |
| analytics (ROI/line/direction) | DS/Analytics |
| custom user metadata | DS/AddCustomMeta |
| message brokers / schema | DS/Messaging |
| RTSP in/out + HW encode | DS/RTSPInOut |
| smart record | DS/SmartRecord |
| Triton / nvinferserver | DS/Triton |
| optical flow / segmentation | DS/OpticalFlow&Segmentation |
| audio inference | DS/AudioInference |
| full framework | Capstone/CapstoneFramework |

---

# Build & directory conventions

```
tutorials/
  easy/    <Name>/{README.md, main.cpp, main_raii.cpp[, main_declarative.cpp, main_dynamic.cpp], CMakeLists.txt}
  medium/  <Name>/…
  hard/    <Name>/…
  deepstream/ <Name>/…      # guarded on DeepStream_FOUND
  capstone/   <Name>/…
```

- The **C track** (`main.cpp`) never includes `deepstream.hpp` headers — it is
  pure C API, so the diff against the wrapper is honest.
- Wrapper variants link `gstreamer::hpp` / `pipeline::hpp` / `ds::elements` /
  `ds::metadata` as needed.
- DeepStream tutorials build only when the SDK is found and are marked
  `LABELS deepstream` in CTest so a CPU-only CI job skips them.

# Authoring order (recommended)

1. Backfill the ⬜ **Easy** items (6–8) — they close the GStreamer fundamentals.
2. Medium 6–10 — buffers, events, clocks, tags, dynamic pipelines.
3. DeepStream 1–5 — the core inference/tracking/metadata story (highest value).
4. Hard 5 (CustomPlugin) — unlocks understanding of the `nv*` elements.
5. Remaining DeepStream + Hard, then the Capstone.
