Cross-Check: docs/description.md vs. Current Implementation

What's Fully Implemented

┌─────────────────────────────────────────────────────────────────────┬────────────────────────────────────────────────────────────────────────────────┐
│                          Description claim                          │                                 Actual status                                  │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ RAII resource management                                            │ gst::Element, gst::Pipeline — move-only, custom deleters throughout            │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ nonstd::expected error handling                                     │ Yes, all gst:: and ds:: APIs return expected<T, E>                             │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ gst::MessageType enum with |/& operators                            │ Implemented in gstreamer.hpp:72                                                │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ gst::parse_launch(), gst::bus_timed_pop_filtered(),                 │ All present in gstreamer.hpp                                                   │
│ gst::message_parse_error()                                          │                                                                                │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ Smart pointer aliases (ElementPtr, BusPtr, ErrorPtr, MessagePtr)    │ Yes, plus PadPtr, CapsPtr (not mentioned in description)                       │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ Pipeline DSL node (gst::Node)                                       │ Present in pipeline.hpp as gst::Node + gst::PipelineDesc (not gst::Graph)      │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ Metadata views (metadata/*.hpp)                                     │ Fully structured: batch_meta, frame_meta, object_meta, classifier_meta,        │
│                                                             eta                                                         │
├─────────────────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ Logging / debug layer                                               │ ds::DebugLayer singleton with spdlog + callback + DS_DEBUG/INFO/WARN/ERROR     │
│                                                                     │ macros                                                                         │
├─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ Structured error types                                              │ ds::ErrorKind, ds::Error, ds::ElementError, ds::PipelineError                  │
└─────────────────────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────────────────────────┘

---
Where the API Diverged from the Description

┌─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                            Description says                            │                        Implementation actually does                         │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ ds::Element::create("nvstreammux") — generic factory                   │ Typed factories: ds::StreamMux::create(), ds::FileSource::create(), etc.    │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ nvstreammux.setBatchSize(4) — camelCase setters                        │ snake_case chained setters: .batch_size(4), .width(), .height()             │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ ds::PipelineBuilder with .source(), .infer(), .tracker(), .osd(),      │ ds::Builder with generic .add(element) only — no domain-specific chain      │
│ .sinkWindow()                                                          │ methods                                                                     │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ ds::FileSource("video.mp4") — constructor                   create().value().location("video.mp4") — named factory +    │
│                                                                        │ fluent                                                                      │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ ds::Debug::enableVerbose()                                  instance().set_min_level(ds::DebugLevel::Debug)             │
├────────────────────────────────────────────────────────────────────────┼─────────────────────────────────────────────────────────────────────────────┤
│ NvInfer, NvTracker, NvStreamMux, NvDsOSD class names                   │ PrimaryInfer, Tracker, StreamMux, OSD — no Nv prefix                        │
└────────────────────────────────────────────────────────────────────────┴─────────────────────────────────────────────────────────────────────────────┘

---
What's in the Description but Not Yet Implemented

- Domain-specific builder methods — .source(), .infer(), .tracker(), .osd(), .sinkWindow() on PipelineBuilder. Only ds::Builder::add() exists.
- Compile-time safety — described as static checks for missinges. Only runtime caps compatibility check is in place(can_link_statically in builder.hpp).
- Custom plugin support — described but no adapter mechanism exists.                                                                                       - Medium/Hard tutorials — directories exist but are empty.

---
What's Implemented But Not Mentioned in Description

- ds::RTSPSource, ds::CameraSource — two additional source types
- ds::SecondaryInfer — secondary inference (chained on primary GIE ID)
- ds::VideoConverter — nvvideoconvert wrapper                                                                                                              - gst::PadPtr, gst::CapsPtr — additional smart handles
- gst::bin_add(), gst::element_link(), gst::element_set_state() — explicit pipeline wiring helpers                                                         - ds::DebugLayer callback + spdlog dual routing — more capablee() described

---
Bottom Line

The gst namespace is complete and solid — all described primitives exist and match the spirit of the description. The ds element layer is implemented with typed wrappers, but the naming and API shape diverged from thelder not PipelineBuilder, no Nv prefixes). The main gap is thehigh-level fluent PipelineBuilder with domain methods — the description's flagship usage example doesn't compile against the current API.
