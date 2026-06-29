# Easy (Fundamentals)

Goal: Understand how GStreamer is built from elements, pads, buses, and pipelines.

You already have:

1. ✅ HelloWorld
2. ✅ VideoFilePlayer

I would continue with:

### 3. Audio Player

**New concepts**

* audio pipeline
* audio sink
* EOS handling
* state transitions

Pipeline

```text
filesrc
    ↓
decodebin
    ↓
audioconvert
    ↓
audioresample
    ↓
autoaudiosink
```

Concepts learned

* Dynamic pads
* Audio pipeline
* decodebin reuse

---

### 4. Webcam Viewer

**New concepts**

* Live source
* Different clock behavior
* Caps negotiation

Pipeline

```text
v4l2src
    ↓
videoconvert
    ↓
autovideosink
```

Learn

* Live pipeline
* Framerate
* Device selection

---

### 5. Pipeline Builder

Instead of creating another application, create a reusable library.

Students implement

```cpp
Pipeline pipeline;

pipeline
    .source<FileSrc>("video.mp4")
    .decode()
    .convert()
    .sink<AutoVideoSink>();
```

Learn

* Wrapping GStreamer
* RAII
* Error handling
* Builder pattern

This becomes the foundation for every later tutorial.

---

# Medium (Real Applications)

Goal: Understand branching, timing, networking, and custom processing.

---

### 1. Video Recorder

Read webcam

Display

Save simultaneously

Pipeline

```text
            tee
           /   \
camera → queue  queue
        |          |
display      encoder
                 |
              mp4mux
                 |
              filesink
```

Learn

* tee
* queue
* branching
* synchronization

---

### 2. RTSP Client

Pipeline

```text
rtspsrc
    ↓
decodebin
    ↓
videoconvert
    ↓
autovideosink
```

Learn

* Network streaming
* Latency
* Jitter
* Reconnection

---

### 3. Object Detection Overlay (CPU)

Instead of AI

Use

```
appsink
```

Receive frames

Draw rectangles with OpenCV

Return

```
appsrc
```

Pipeline

```text
decode
   ↓
appsink
   ↓
OpenCV
   ↓
appsrc
   ↓
display
```

Learn

* appsink
* appsrc
* Custom processing

---

### 4. Image Capture

Display live video

Press Space

Save JPEG

Learn

* Pad probes
* Snapshot
* JPEG encoder

---

### 5. Pipeline Inspector

Students create

```
gst-inspect clone
```

using

```
gst_registry
```

Learn

* Registry
* Plugins
* Factories
* Capabilities

---

# Hard (Production Grade)

Goal: Build systems similar to DeepStream applications.

---

### 1. Multi-Camera Viewer

```
4 cameras

↓

compositor

↓

display
```

Learn

* compositor
* Synchronization
* Multiple live sources

---

### 2. Video Analytics Pipeline

```
decode

↓

appsink

↓

OpenCV

↓

tracking

↓

overlay

↓

display
```

Learn

* Metadata
* Pipeline architecture
* Zero-copy discussion

---

### 3. RTSP Server

Create your own RTSP server.

Learn

* Encoding
* RTP
* RTSP
* Streaming

---

### 4. Plugin Development

Create custom element

```
MyEdgeDetector
```

Learn

* GstBaseTransform
* Pads
* Negotiation
* Plugin registration

This is where GStreamer becomes much clearer.

---

### 5. DeepStream-style Framework

The final project.

Instead of writing

```cpp
gst_bin_add(...)
gst_element_link(...)
```

Students build

```cpp
Pipeline p;

p.source<FileSource>("video.mp4")
 .decode()
 .infer<Model>("people.engine")
 .tracker()
 .overlay()
 .display();
```

Internally

```
PipelineBuilder

↓

Graph Validation

↓

GStreamer

↓

Execution
```

Topics

* Graph validation
* Strong typing
* Builder DSL
* Compile-time checks
* Production logging
* Configuration
* Metrics
* Plugin discovery
* Unit testing
* Performance measurement

This naturally prepares students to understand and extend NVIDIA DeepStream applications.

---

# Overall Learning Roadmap

| Level  | Tutorial                         | Main Concept                                                      |
| ------ | -------------------------------- | ----------------------------------------------------------------- |
| Easy   | 1. Hello World                   | Initialization, pipeline lifecycle                                |
| Easy   | 2. Video File Player             | Decoding, bus messages, playback                                  |
| Easy   | 3. Audio Player                  | Audio pipeline, dynamic pads                                      |
| Easy   | 4. Webcam Viewer                 | Live sources, caps negotiation                                    |
| Easy   | 5. Pipeline Builder              | RAII, C++ wrapper, fluent API                                     |
| Medium | 1. Video Recorder                | `tee`, `queue`, branching                                         |
| Medium | 2. RTSP Client                   | Network streams, latency, reconnection                            |
| Medium | 3. CPU Video Processing          | `appsink`, `appsrc`, custom frame processing                      |
| Medium | 4. Image Capture                 | Pad probes, snapshots, JPEG encoding                              |
| Medium | 5. Pipeline Inspector            | Plugin registry, factories, capabilities                          |
| Hard   | 1. Multi-Camera Viewer           | `compositor`, synchronization                                     |
| Hard   | 2. Video Analytics Pipeline      | Metadata flow, processing architecture                            |
| Hard   | 3. RTSP Server                   | Encoding, RTP/RTSP streaming                                      |
| Hard   | 4. Custom GStreamer Plugin       | `GstBaseTransform`, custom elements                               |
| Hard   | 5. Production Pipeline Framework | Type-safe DSL, graph validation, DeepStream-inspired architecture |

This sequence has a deliberate progression: each tutorial introduces one new core concept, minimizes cognitive load, and culminates in a production-grade, modern C++20 framework that mirrors the design principles behind NVIDIA DeepStream while remaining approachable for learners.
