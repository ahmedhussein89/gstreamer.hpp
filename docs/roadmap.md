# 🚀 **Roadmap: Building the DeepStream C++ Wrapper From deepstream-app**

## 🔵 **Phase 0 — Preparation (1–2 days)**

**Goal:** Familiarize yourself with deepstream-app internals & define project structure.

### Tasks

* Clone DeepStream SDK source and extract:

  * `deepstream-app` (primary reference)
  * `deepstream-test1`, `deepstream-test2`, `deepstream-test3`
* Identify pipeline stages:

  * Source(s)
  * StreamMux
  * Infer
  * Tracker
  * OSD
  * Sink
* Create repository structure:

  ```
  deepstream-hpp/
    include/deepstream/...
    src/ (only if needed)
    examples/
    tests/
    docs/
  ```
* Enable CI early:

  * clang-format, clang-tidy
  * GCC + Clang builds
  * Sanitizers

**Deliverable:** Empty skeleton project with CI, docs, and folder structure.

---

# 🔵 **Phase 1 — Understand & Extract the Core Pipeline Logic (3–5 days)**

**Goal:** Understand deepstream-app logic and extract the minimal functioning pipeline.

### Tasks

* Study deepstream-app:

  * `create_source_bin()`
  * `create_pipeline()`
  * `link_elements()`
  * metadata probe callbacks
  * event loops
* Copy only **minimal core logic** into a new prototype:

  ```
  prototype/
    minimal_pipeline.cpp
  ```
* Get a basic pipeline running:

  * File → Mux → Infer → OSD → Sink

**Deliverable:** A standalone C++ minimal DeepStream pipeline *not depending on deepstream-app*.

---

# 🔵 **Phase 2 — Introduce RAII Wrappers for Core GStreamer Objects (5–7 days)**

**Goal:** Create the core C++ abstractions that manage GStreamer lifetimes.

### Tasks

* Create RAII wrappers:

  * `ds::Element` → wraps `GstElement*`
  * `ds::Pipeline` → wraps `GstPipeline*`
  * `ds::Pad`, `ds::Caps`, `ds::Bus`, `ds::Message`
* Ensure automatic cleanup (unref, set NULL state on destruction).
* Implement typed property setters:

  ```cpp
  nvstreammux.set("batch-size", 4);
  ```

**Deliverable:** `core/` module with RAII wrappers replacing raw pointers.

---

# 🔵 **Phase 3 — Create Strongly-Typed DeepStream Components (7–10 days)**

**Goal:** Wrap all major DeepStream plugins into C++ typed classes.

### Tasks

Implement classes:

### **Sources**

* `FileSource`
* `RTSPSource`
* `CameraSource` (V4L2)

### **Transformations**

* `StreamMux`
* `VideoConverter`
* `OSD`

### **Inference**

* `PrimaryInfer` (`nvinfer`)
* `SecondaryInfer`

### **Tracking**

* `Tracker`

### **Output**

* `WindowSink`
* `FileSink`

Each wrapper contains:

* RAII element
* Type-safe setters
* Error-checked construction

**Deliverable:** `elements/` module containing typed wrappers.

---

# 🔵 **Phase 4 — Implement the Pipeline Builder (7–10 days)**

**Goal:** Introduce fluent pipeline creation inspired by vulkan.hpp / modern C++ builders.

### Tasks

* Create `PipelineBuilder` struct/class.
* Support chaining:

  ```cpp
  auto pipeline = ds::PipelineBuilder()
      .source("video.mp4")
      .streamMux({.batchSize = 1})
      .infer("model.engine")
      .osd()
      .windowSink()
      .build();
  ```
* Implement internal representation as a node graph:

  * Nodes = elements
  * Edges = links
* Add validation:

  * Caps compatibility
  * Mandatory nodes
  * Unique IDs for elements

**Deliverable:** `pipeline/` module with working builder API.

---

# 🔵 **Phase 5 — Metadata System (7–10 days)**

**Goal:** Replace raw metadata parsing with modern C++ typed views.

### Tasks

* Create:

  * `FrameMetaView`
  * `ObjectMetaView`
  * `ClassifierMetaView`
  * `TensorMetaView`
* Provide iteration:

  ```cpp
  for (auto& frame : batch.frames()) {
      for (auto& obj : frame.objects()) {
          // typed access
      }
  }
  ```
* Support user metadata extensibility.

**Deliverable:** `metadata/` module with typed access.

---

# 🔵 **Phase 6 — Debug Layer & Error Handling (4–6 days)**

**Goal:** Introduce predictable, architect-level diagnostics.

### Tasks

* Add validation callbacks (similar to Vulkan layers).
* Add logging macros:

  ```cpp
  DS_DEBUG("Inferred frame {}, objects={}", frameId, objCount);
  ```
* Catch DeepStream errors and convert to:

  * `std::runtime_error`
  * `ds::PipelineError`
  * `ds::ElementError`

**Deliverable:** `utils/debug.hpp`, `utils/error.hpp`.

---

# 🔵 **Phase 7 — Examples & Tests (5–7 days)**

**Goal:** Add examples to prove the wrapper works end-to-end.

### Example Apps

* 01_basic_file_infer
* 02_multi_stream
* 03_secondary_inference
* 04_custom_metadata
* 05_rtsp_input

### Tests

* Unit tests for each wrapper
* Integration test for sample pipeline
* Metadata extraction tests

**Deliverable:** `examples/`, `tests/` folders with full coverage.

---

# 🔵 **Phase 8 — Documentation & Release (5–7 days)**

**Goal:** Publish the wrapper.

### Tasks

* Write documentation:

  * Getting started
  * Architecture overview
  * Element list
  * Pipeline builder tutorial
* Add Doxygen config
* Publish FIRST RELEASE: `v0.1.0`

**Deliverable:** Publicly usable, documented library.

---

# 🧠 Project Duration Summary

| Phase | Duration  |
| ----- | --------- |
| 0     | 1–2 days  |
| 1     | 3–5 days  |
| 2     | 5–7 days  |
| 3     | 7–10 days |
| 4     | 7–10 days |
| 5     | 7–10 days |
| 6     | 4–6 days  |
| 7     | 5–7 days  |
| 8     | 5–7 days  |

### **Estimated Total: 44–64 days**

(~6–9 weeks of focused work)

---

# 🎯 Optional Add-ons (for later versions)

### **v0.2 – JSON/YAML Pipelines**

Import pipelines from files.

### **v0.3 – CUDA Integration**

Expose GPU buffers directly.

### **v0.4 – Python bindings**

Use pybind11 to expose the wrapper.

### **v1.0 – Production Release**

Full coverage + stable API.
