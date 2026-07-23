# Tutorials

Hands-on examples for GStreamer and the `gst::` C++ wrappers, ordered by difficulty.

## Easy

Introductory concepts: pipeline creation, state management, sources, decoders, sinks.

See [easy/README.md](easy/README.md) for the full list and progression guide.

| Tutorial | Summary |
|---|---|
| [HelloWorld](easy/HelloWorld/) | Synthetic video, `gst_parse_launch`, main loop |
| [ElementByHand](easy/ElementByHand/) | Manual element creation, floating references, `bin_add` ownership |
| [PipelineBuilder](easy/PipelineBuilder/) | Runtime pipeline strings, `gst::parse_launch` |
| [CapsAndFilters](easy/CapsAndFilters/) | `capsfilter`, caps negotiation, pad caps inspection |
| [VideoFilePlayer](easy/VideoFilePlayer/) | File playback, `decodebin`, dynamic pads, EOS |
| [AudioPlayer](easy/AudioPlayer/) | Audio path, format conversion, resampling |
| [WebcamViewer](easy/WebcamViewer/) | Live V4L2 capture, `videoconvert` |
| [StatesAndSeeking](easy/StatesAndSeeking/) | State machine transitions, `gst_element_seek_simple` |

## Medium

Intermediate patterns: per-frame access, branching, registry inspection, network sources.

See [medium/README.md](medium/README.md) for the full list and progression guide.

| Tutorial | Summary |
|---|---|
| [EventsAndQueries](medium/EventsAndQueries/) | Position/duration queries, seek events |
| [BuffersAndMemory](medium/BuffersAndMemory/) | `GstBuffer`, `GstMemory`, timestamps, pad probes |
| [ClocksAndSync](medium/ClocksAndSync/) | Pipeline clock, sync mode, running time |
| [CPUVideoProcessing](medium/CPUVideoProcessing/) | `appsink`/`appsrc`, CPU pixel manipulation |
| [ImageCapture](medium/ImageCapture/) | Pad probes, single-frame snapshot to PPM |
| [DynamicPipeline](medium/DynamicPipeline/) | Add/remove branches while PLAYING, blocking probes |
| [PipelineInspector](medium/PipelineInspector/) | GStreamer registry, element discovery |
| [TagsAndMetadata](medium/TagsAndMetadata/) | `GstTagList`, bus TAG messages, `GstDiscoverer` |
| [RTSPClient](medium/RTSPClient/) | RTSP stream, chained dynamic pads |
| [VideoRecorder](medium/VideoRecorder/) | `tee`, H.264 encoding, MP4 file output |

## Hard

Production GStreamer and plugin development: multi-source composition, encoding profiles, servers, custom elements, and testing.

See [hard/README.md](hard/README.md) for the full list and progression guide.

| Tutorial | Summary |
|---|---|
| [MultiCameraViewer](hard/MultiCameraViewer/) | `compositor` request sink pads, N live sources composited into one frame |
| [EncodeProfiles](hard/EncodeProfiles/) | `encodebin` driven by a hand-built `GstEncodingProfile` |
| [CustomPlugin](hard/CustomPlugin/) | `GstBaseTransform` subclass (`MyEdgeDetector`), pad templates, runtime registration |
| [CustomSourceSink](hard/CustomSourceSink/) | `GstBaseSrc`/`GstBaseSink` subclassing (`mysrc`/`mysink`) |
| [TestingElements](hard/TestingElements/) | `GstHarness` unit testing, `GST_TRACERS`/`GST_DEBUG` |
| [RTSPServer](hard/RTSPServer/) | `GstRTSPServer` + `GstRTSPMediaFactory`, mount points |
| [NetClockSync](hard/NetClockSync/) | `GstNetClock`, multi-machine synchronized playback |
