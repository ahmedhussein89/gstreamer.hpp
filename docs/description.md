# 🚀 **Project Description: deepstream.hpp — A Modern C++ Wrapper for NVIDIA DeepStream**

## **1. Overview**

**deepstream.hpp** is a modern, header-only C++ wrapper around the **NVIDIA DeepStream SDK**, inspired by the architectural and design principles of **vulkan.hpp**.
It aims to make DeepStream **type-safe, expressive, modular, and easy to use** while maintaining zero-overhead abstractions suitable for real-time video analytics.

The library provides modern C++ constructs on top of GStreamer/DeepStream:

* RAII resource management
* Strongly-typed handles
* Builder patterns for pipeline creation
* Fluent and readable pipeline syntax
* Zero-cost abstractions over DeepStream elements
* Compile-time configuration where possible
* Runtime introspection and logging utilities
* High-performance, low-latency design suitable for edge (Jetson) and server GPUs

This project transforms DeepStream from a “GStreamer plugin maze” into a **clean C++ API** that feels like building a rendering pipeline in Vulkan or a graph in TensorFlow.

---

# **2. Motivation**

DeepStream is extremely powerful, but difficult to work with because:

* GStreamer factory APIs are verbose and error-prone.
* Pipeline management requires a lot of boilerplate.
* Element properties are strings, not strongly typed.
* Error handling is inconsistent.
* Custom C++ applications require repetitive code.

**deepstream.hpp** solves this by providing:

* A **C++17 type-safe interface**
* A **minimal, composable API surface**
* **Predictable object lifetimes and RAII**
* **Full control** over the pipeline like Vulkan provides over GPU resources
* Declarative and fluent pipeline construction

Just like Vulkan abstracts GPU hardware while keeping full control, deepstream.hpp gives developers explicit control over DeepStream pipelines while removing complexity.

---

# **3. Design Philosophy (Inspired by vulkan.hpp)**

### 3.1 **Zero-overhead abstractions**

All wrappers are inline, header-only templates that compile down to raw DeepStream/GStreamer calls.

### 3.2 **RAII and deterministic lifetimes**

Every DeepStream/GStreamer element is represented by a RAII C++ object:

```cpp
ds::Element nvstreammux = ds::Element::create("nvstreammux");
```

### 3.3 **Strong typing**

Properties become typed methods:

```cpp
nvstreammux.setBatchSize(4);
nvstreammux.setWidth(1920);
nvstreammux.setHeight(1080);
```

### 3.4 **Builder pattern for Pipelines**

```cpp
auto pipeline = ds::PipelineBuilder()
    .source("file:///video.mp4")
    .infer("model.engine")
    .tracker("nvtracker_config.txt")
    .osd()
    .sinkWindow()
    .build();
```

### 3.5 **Explicitness**

Nothing hidden, nothing magical:

* All elements visible in code
* All links explicit
* No implicit state transitions

### 3.6 **Compile-time safety**

Static checks for:

* Missing links
* Missing mandatory properties
* Unsupported caps
* Null elements

### 3.7 **Extensibility**

Users can plug:

* Custom inference blocks
* Custom GStreamer elements
* Custom DeepStream metadata

using simple C++ inheritance or template-based adapters.

---

# **4. Key Features**

### ✔ **C++17/20 header-only wrapper**

No runtime overhead, easy integration.

### ✔ **Strongly typed DeepStream Elements**

`NvInfer`, `NvTracker`, `NvStreamMux`, `NvDsOSD`, etc.

### ✔ **Declarative Pipeline Builder**

Construct pipelines with fluent API patterns.

### ✔ **Exception-based error handling**

Meaningful exceptions instead of silent GStreamer errors.

### ✔ **Metadata Views**

Typed metadata structures:

```cpp
for (const auto& obj : frame.objects<ds::NvDsObjectMeta>()) {
    // Access metadata with strong typing
}
```

### ✔ **Custom Plugin Support**

Load and wrap custom DeepStream plugins with zero boilerplate.

### ✔ **Jetson + x86_64 Support**

One codebase, multi-target build.

### ✔ **Integrated logging & diagnostics**

Inspired by Vulkan validation layers:

```cpp
ds::Debug::enableVerbose();
```

---

# **5. High-Level Architecture**

### **Modules**

1. **core/**
   GStreamer utilities, smart handles, RAII objects.

2. **elements/**
   Strongly-typed wrappers for DeepStream elements.

3. **pipeline/**
   Pipeline graph, builder, linkers.

4. **metadata/**
   Metadata parsing and typed access.

5. **utils/**
   Error handling, logging, configuration loaders.

### **Example Architecture Diagram**

```
                deepstream.hpp
----------------------------------------------
| core/       | SmartHandle, Element, Caps   |
| elements/   | NvInfer, NvStreamMux, OSD    |
| pipeline/   | Pipeline, PipelineBuilder    |
| metadata/   | FrameView, ObjectView        |
| utils/      | Log, Error, Validation       |
----------------------------------------------
                ↓ calls
         NVIDIA DeepStream + GStreamer
```

---

# **6. Example Usage**

## **6.1 Minimal Example**

```cpp
#include <deepstream/deepstream.hpp>

int main()
{
    ds::Pipeline pipeline;

    auto source = ds::FileSource("video.mp4");
    auto mux    = ds::NvStreamMux().batchSize(1);
    auto infer  = ds::NvInfer("model.engine");
    auto osd    = ds::OSD();
    auto sink   = ds::WindowSink();

    pipeline.add(source, mux, infer, osd, sink);

    pipeline.link(source, mux, infer, osd, sink);

    pipeline.run();
}
```

---

# **7. Target Users**

### ✔ C++ developers building real-time video analytics

### ✔ ML engineers integrating ONNX/TensorRT models

### ✔ Companies deploying on Jetson/edge devices

### ✔ Teams building reusable DeepStream frameworks

### ✔ Architects needing reproducible, maintainable pipelines

---

# **8. Deliverables**

* **deepstream.hpp** header-only library
* Examples + tutorials
* CMake-based project template
* CI pipeline (build, formatting, sanitizers)
* Documentation (Doxygen + Sphinx)
* Benchmarks (latency, throughput)
* Packaging (vcpkg, Conan in the future)

---

# 🎯 **Final Summary**

**deepstream.hpp** brings Vulkan-grade design to DeepStream development: explicit, modern, safe, high-performance, and architecturally clean. It turns DeepStream into a pleasant C++ API with full control, strong typing, and predictable behavior — the perfect project for a technical architect specializing in real-time video pipelines.
