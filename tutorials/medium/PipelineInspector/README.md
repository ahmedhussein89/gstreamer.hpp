# Pipeline Inspector

## Goal

Enumerate every element registered in the GStreamer plugin registry, printing
its name, class, description, and pad templates. Supports an optional substring
filter to narrow the output.

## Concepts

- `GstRegistry` — the runtime database of all loaded plugins and elements
- `gst_registry_get_plugin_list` — iterate installed plugins
- `gst_registry_get_feature_list_by_plugin` — list elements per plugin
- `GstElementFactory` metadata — name, class, long description
- `GstStaticPadTemplate` — static pad direction and presence

## Running

```bash
# list everything
./build/tutorials/medium/PipelineInspector

# filter by substring
./build/tutorials/medium/PipelineInspector video
./build/tutorials/medium/PipelineInspector decode
./build/tutorials/medium/PipelineInspector rtsp
```

## Variants

| Binary | Source | Approach |
|---|---|---|
| `PipelineInspector` | `main.cpp` | Imperative GStreamer C API |
| `PipelineInspectorRAII` | `main_raii.cpp` | `gst::` RAII wrappers |

## Sample Output

```
x264enc
  class:       Codec/Encoder/Video
  description: libx264-based H264 encoder
    pad[sink] direction=sink presence=always
    pad[src] direction=src presence=always

Total elements found: 1
```

## Why This Is Useful

When porting pipelines to new machines or containers, you often need to know
whether a specific element is available before writing any pipeline code. This
tool answers "what can I use?" without leaving C++.

It also demonstrates the registry API, which can be used programmatically to
validate element availability at application startup.
