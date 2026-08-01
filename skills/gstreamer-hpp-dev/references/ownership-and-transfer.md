# Ownership & Transfer Semantics

GStreamer's C API annotates every pointer with a transfer mode. Mapping that mode to the right C++
type is the whole job of this library. Get it wrong and you get either a leak (sanitizer-only) or a
double-free (`GStreamer-CRITICAL: assertion 'object->ref_count > 0' failed`).

**Before wrapping any `gst_*` function, look up its transfer annotation in the GStreamer docs or
the header comment.** Do not guess from the function name.

---

## The four cases

### 1. transfer-full OUT — the callee gives you a ref you must release

Return an owning type.

| Layer | Return type | Example |
|---|---|---|
| Enhanced | `nonstd::expected<XPtr, std::string>` | `element_get_bus` → `expected<BusPtr, string>` |
| RAII | `nonstd::expected<raii::X, std::string>` | `raii::element_get_bus` → `expected<raii::Bus, string>` |

```cpp
inline nonstd::expected<BusPtr, std::string> element_get_bus(Element element) {
  GstBus* bus = gst_element_get_bus(element.get());          // transfer full
  if(bus == nullptr) {
    return nonstd::make_unexpected(std::string("Failed to get bus from element"));
  }
  return BusPtr{bus};                                         // adopt, no extra ref
}
```

Never `gst_object_ref` a transfer-full return. You already own it.

### 2. transfer-none OUT — the callee keeps ownership

Return the bare handle, a `std::string_view`, or a raw `const` pointer. **Never** an `XPtr`.

```cpp
inline Buffer sample_get_buffer(Sample sample) noexcept {          // transfer none
  return Buffer{gst_sample_get_buffer(sample.get())};
}

inline std::string_view structure_get_name(const GstStructure* structure) {  // transfer none
  return std::string_view{gst_structure_get_name(structure)};
}
```

If the caller needs to outlive the parent, that is the caller's problem — document it, don't
silently ref.

### 3. transfer-full IN — the function consumes what you hand it

Take the owning type **by value** and `.release()` into the C call. This makes the consumption
visible in the signature: the caller's variable is moved-from and cannot be used again.

```cpp
// gst_message_new_application() takes ownership of the structure.
inline MessagePtr message_new_application(GstObject* src, StructurePtr structure) noexcept {
  return MessagePtr{gst_message_new_application(src, structure.release())};
}

// gst_harness_push() takes ownership of the buffer.
inline nonstd::expected<void, std::string> harness_push(const HarnessPtr& harness, BufferPtr buffer) {
  const GstFlowReturn ret = gst_harness_push(harness.get(), buffer.release());
  ...
}
```

**Always add a comment stating the consumption**, as the existing code does:
`// Consumes the buffer (transfer-full into the harness), matching gst_harness_push semantics.`

#### The failure-path trap

If the C call can fail *and* still consumes (or fails to consume) the ref, you must handle it
explicitly. Two real examples from the repo, with opposite resolutions:

```cpp
// bin_add: on failure gst_bin_add did NOT take the ref, so we must unref.
inline nonstd::expected<gst::Element, std::string> bin_add(const Pipeline& pipeline, Element element) {
  GstElement* raw = element.release();
  if(gst_bin_add(GST_BIN(pipeline.get()), raw) != TRUE) {
    gst_object_unref(raw);                       // <-- required, else leak
    return nonstd::make_unexpected(std::string("Failed to add element to pipeline"));
  }
  return gst::Element{raw};                      // non-owning; bin owns it now
}

// encoding_container_profile_add_profile: the C API consumes the ref even on failure,
// so there is NO unref on the error path.
inline nonstd::expected<void, std::string> encoding_container_profile_add_profile(
    const EncodingContainerProfilePtr& container, EncodingVideoProfilePtr video_profile) {
  if(!gst_encoding_container_profile_add_profile(container.get(),
                                                 GST_ENCODING_PROFILE(video_profile.release()))) {
    return nonstd::make_unexpected(std::string("Failed to add video profile to container profile"));
  }
  return {};
}
```

Both are correct. The difference is the C API's documented behaviour, not a style choice. Write the
comment explaining which case you are in.

### 4. Floating references

`gst_element_factory_make`, `gst_pipeline_new`, and most `gst_*_new` on `GstObject` subclasses
return a **floating** ref. It is sunk by the first `gst_bin_add` / `g_object_ref_sink`.

Consequences in this codebase:

- `element_factory_make` returns `expected<Element, string>` — a **non-owning handle** in the
  enhanced layer, because the floating ref is expected to be sunk by a bin shortly after.
  `raii::element_factory_make` returns `raii::Element` for the case where the caller wants to own
  it before adding.
- After `bin_add` succeeds, the bin owns the element. The returned `gst::Element` handle stays
  valid for the lifetime of the pipeline and is what you link with. **Do not** keep a `raii::Element`
  alive alongside it — that is a double-free.
- A `raii::Element` that is never added to a bin unrefs a floating ref on destruction. That is
  correct and does not leak.

---

## Return-type decision table

| C signature returns | Nullable? | Wrap as |
|---|---|---|
| transfer-full `GstX*` | yes | `expected<XPtr, string>` / `expected<raii::X, string>` |
| transfer-full `GstX*` | no | `XPtr` (plain, `noexcept`) — e.g. `event_new_eos`, `query_new_latency` |
| transfer-none `GstX*` | yes | `expected<gst::X, string>` if absence is an error, else bare `gst::X` |
| transfer-none `const gchar*` | yes | `std::string_view` (empty view when null) |
| `gboolean` success flag | — | `expected<void, string>` |
| `GList*` of transfer-full | — | `std::vector<XPtr>`, then `g_list_free(list)` on the spine only |
| out-param + `gboolean` | — | `expected<T, string>` with `T` the out-param, or a small POD (`StateChange`, `LatencyInfo`) |

## GList pattern (copy verbatim)

```cpp
inline std::vector<PluginPtr> registry_get_plugin_list(Registry registry) {
  GList* list = gst_registry_get_plugin_list(registry.get());
  std::vector<PluginPtr> plugins;
  for(GList* node = list; node != nullptr; node = node->next) {
    plugins.emplace_back(static_cast<GstPlugin*>(node->data));   // adopt each ref
  }
  g_list_free(list);                                             // spine only
  return plugins;
}
```

`g_list_free_full(list, gst_object_unref)` here would double-free, because each element's ref is
already owned by a `PluginPtr`.

## Callback pattern (copy verbatim)

Heap-allocate the `std::function`, hand the raw pointer as `user_data`, and free it in the
`GDestroyNotify`. This is the only sanctioned `new` in the codebase.

```cpp
inline gulong pad_add_probe(Pad pad, PadProbeTypeFlags mask,
                            std::function<PadProbeReturn(Pad, GstPadProbeInfo*)> callback) {
  auto* cb_ptr = new std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>(std::move(callback));
  return gst_pad_add_probe(
      pad.get(), static_cast<GstPadProbeType>(mask.value()),
      [](GstPad* probe_pad, GstPadProbeInfo* info, gpointer data) -> GstPadProbeReturn {
        auto& cb = *static_cast<std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>*>(data);
        return static_cast<GstPadProbeReturn>(cb(Pad{probe_pad}, info));
      },
      cb_ptr,
      [](gpointer data) { delete static_cast<std::function<PadProbeReturn(Pad, GstPadProbeInfo*)>*>(data); });
}
```

The lambda passed to C **must be captureless** so it converts to a plain function pointer. All state
travels through `user_data`.

## Scoped-guard pattern

`BufferMapGuard` (in `gstreamer.hpp`) is the model for any acquire/release pair that is not a
refcount: RAII class, obtained through a factory returning `expected`, unmaps in the destructor.
Use it for `gst_buffer_map`/`unmap`, and copy its shape for any future
`gst_video_frame_map`/`unmap`-style addition.

## Review checklist

- [ ] Transfer annotation of every `gst_*` call in the new code was looked up, not guessed
- [ ] Transfer-full-in params are by value and `.release()`d
- [ ] Every error path either unrefs or documents why it must not
- [ ] No `XPtr` wrapping a transfer-none pointer
- [ ] `GList` spine freed with `g_list_free`, never `g_list_free_full`
- [ ] A comment states the transfer contract wherever it is not obvious from the name
- [ ] A test in `tests/testGstreamerRaii.cpp` (or `testGstreamer.cpp`) exercises the failure path,
      and the suite passes under `-DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address`
