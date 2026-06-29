# Tutorials

Hands-on examples for GStreamer and the `gst::` C++ wrappers, ordered by difficulty.

## Easy

Introductory concepts: pipeline creation, state management, sources, decoders, sinks.

See [easy/README.md](easy/README.md) for the full list and progression guide.

| Tutorial | Summary |
|---|---|
| [HelloWorld](easy/HelloWorld/) | Synthetic video, `gst_parse_launch`, main loop |
| [PipelineBuilder](easy/PipelineBuilder/) | Runtime pipeline strings, `gst::parse_launch` |
| [VideoFilePlayer](easy/VideoFilePlayer/) | File playback, `decodebin`, dynamic pads, EOS |
| [AudioPlayer](easy/AudioPlayer/) | Audio path, format conversion, resampling |
| [WebcamViewer](easy/WebcamViewer/) | Live V4L2 capture, `videoconvert` |

## Medium

Intermediate patterns: per-frame access, branching, registry inspection, network sources.

See [medium/README.md](medium/README.md) for the full list and progression guide.

| Tutorial | Summary |
|---|---|
| [CPUVideoProcessing](medium/CPUVideoProcessing/) | `appsink`/`appsrc`, CPU pixel manipulation |
| [ImageCapture](medium/ImageCapture/) | Pad probes, single-frame snapshot to PPM |
| [PipelineInspector](medium/PipelineInspector/) | GStreamer registry, element discovery |
| [RTSPClient](medium/RTSPClient/) | RTSP stream, chained dynamic pads |
| [VideoRecorder](medium/VideoRecorder/) | `tee`, H.264 encoding, MP4 file output |

## Hard

Advanced topics (coming soon).
