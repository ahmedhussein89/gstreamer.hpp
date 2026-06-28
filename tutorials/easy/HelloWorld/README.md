# Your First GStreamer Application in C++

One of the easiest ways to start learning GStreamer is to display a fake video stream on the screen. This removes the complexity of cameras, video files, decoders, and network streams, allowing you to focus on the core concepts.

Let's examine the following example:

```cpp
#include <cstdlib>

#include <fmt/core.h>

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);

  auto* pipeline = gst_parse_launch(
      "videotestsrc pattern=snow ! "
      "video/x-raw,width=1280,height=720 ! "
      "autovideosink",
      nullptr);

  if(nullptr == pipeline) {
    fmt::println(stderr, "Failed to create pipeline.");
    return EXIT_FAILURE;
  }

  if(GST_STATE_CHANGE_FAILURE ==
     gst_element_set_state(pipeline, GST_STATE_PLAYING)) {
    fmt::println(stderr, "Failed to play the pipeline.");
    return EXIT_FAILURE;
  }

  g_main_loop_run(g_main_loop_new(nullptr, FALSE));

  return EXIT_SUCCESS;
}
```

## What Does This Application Do?

The application generates artificial video frames and displays them on the screen.

The pipeline is:

```text
videotestsrc
    ↓
video/x-raw,width=1280,height=720
    ↓
autovideosink
```

Conceptually:

```text
Generate Fake Video
        ↓
Force Resolution to 1280x720
        ↓
Display on Screen
```

No video file is loaded.

No decoder is used.

No GPU acceleration is required.

This is one of the smallest useful video pipelines you can build.

---

## Step 1: Initialize GStreamer

```cpp
gst_init(&argc, &argv);
```

Before using any GStreamer API, the framework must be initialized.

Think of this as:

```text
Start the GStreamer runtime
```

Without this call, nothing else will work.

---

## Step 2: Create the Pipeline

```cpp
auto* pipeline = gst_parse_launch(
    "videotestsrc pattern=snow ! "
    "video/x-raw,width=1280,height=720 ! "
    "autovideosink",
    nullptr);
```

This is the most important line in the application.

Instead of manually creating and linking elements, we describe the pipeline using a string.

The syntax is identical to `gst-launch-1.0`.

For example:

```bash
gst-launch-1.0 \
    videotestsrc pattern=snow \
    ! video/x-raw,width=1280,height=720 \
    ! autovideosink
```

The same pipeline can therefore be tested from the command line before writing any C++ code.

---

## Step 3: Understanding Each Element

### videotestsrc

```text
videotestsrc
```

This element generates synthetic video frames.

Useful patterns include:

* smpte
* snow
* black
* white
* red
* green
* blue
* checkerboard

In this example:

```cpp
pattern=snow
```

produces television-style static noise.

---

### Caps Filter

```text
video/x-raw,width=1280,height=720
```

This is called a capability filter (Caps).

It tells GStreamer:

```text
I want raw video
with resolution 1280×720
```

Without it, GStreamer would choose a resolution automatically.

---

### autovideosink

```text
autovideosink
```

This is a smart video sink.

It automatically selects the best video output available on the system.

Examples:

* ximagesink
* glimagesink
* waylandsink
* d3d11videosink
* xvimagesink

You don't need to know which one is available.

`autovideosink` chooses for you.

---

## Step 4: Start Streaming

```cpp
gst_element_set_state(
    pipeline,
    GST_STATE_PLAYING);
```

GStreamer pipelines move through states:

```text
NULL
 ↓
READY
 ↓
PAUSED
 ↓
PLAYING
```

Setting the state to `PLAYING` starts data flowing through the pipeline.

At this point:

```text
videotestsrc generates frames
            ↓
frames flow downstream
            ↓
autovideosink displays them
```

---

## Step 5: Run the Main Loop

```cpp
g_main_loop_run(
    g_main_loop_new(nullptr, FALSE));
```

GStreamer is event-driven.

The main loop keeps the application alive and allows GStreamer to process:

* Video frames
* Timers
* Bus messages
* Window events
* State changes

Without the main loop, the application would exit immediately.

---

## Why This Example Is Great for Beginners

This example teaches several fundamental GStreamer concepts:

* Pipeline creation
* Pipeline descriptions
* Source elements
* Capability negotiation
* Sink elements
* State transitions
* Main loop execution

Most real-world pipelines are simply more complex versions of this pattern:

```text
Source
   ↓
Processing
   ↓
Sink
```

For example:

```text
RTSP Source
      ↓
Decoder
      ↓
Inference
      ↓
OSD
      ↓
Display
```

or

```text
MP4 File
     ↓
Demuxer
     ↓
Decoder
     ↓
Display
```

The architecture remains exactly the same.

## Key Takeaway

If you remember only one thing from this example, remember this:

A GStreamer application usually does four things:

1. Initialize GStreamer.
2. Create a pipeline.
3. Set the pipeline to PLAYING.
4. Run the main loop.

Everything else is just changing what happens inside the pipeline.
