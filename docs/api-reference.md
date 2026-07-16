# API Reference (gst / ds namespaces)

Detailed symbol tables for the wrapper layers. Headers are the source of truth;
this file mirrors them for quick lookup. See `CLAUDE.md` for build/workflow rules.

## `gst` namespace — `include/core/`

vulkan.hpp-style foundation layer, included transitively by `gstreamer.hpp`:

| Header | Contents |
|---|---|
| `core/core.hpp` | Umbrella that pulls in the rest of `core/` |
| `core/handle.hpp` | Non-owning typed handle wrappers over raw `Gst*` pointers |
| `core/flags.hpp` | Type-safe bit-flag template (bitwise operators on strong enums) |
| `core/enums.hpp` | Strongly-typed enum definitions (`State`, `StateChangeReturn`, `FlowReturn`, `PadDirection`, `PadPresence`, `Format`, `SeekType`) |
| `core/array_proxy.hpp` | Lightweight `span`-like view for passing arrays into the API |
| `core/concepts.hpp` | Shared C++20 concept vocabulary; every template in `include/` must constrain against a concept (CI-enforced) |

## `gst` namespace — `include/gstreamer_raii.hpp`

`gst::raii::*` owning layer, mirroring `vulkan_raii.hpp`: each type owns its resource
and releases it in the destructor, and implicitly converts to the matching non-owning
`gst::` handle so every enhanced-layer free function works on a RAII object unchanged.

Types: `gst::raii::Element`, `Pipeline`, `Bus`, `Pad`, `Caps`, `Message` — all move-only.
Factories returning owning types: `gst::raii::parse_launch`, `pipeline_new`,
`element_factory_make`, `element_get_bus`, `element_get_static_pad`, and
`bin_add(const Pipeline&, Element)` (moves ownership into the bin, hands back a
non-owning `gst::Element` for linking).

## `gst` namespace — `include/gstreamer.hpp`

The enhanced layer: **non-owning** typed handles. `gst::Element` is *not* owning —
the owning type is `gst::raii::Element`.

**Handle types** — trivially-copyable, `sizeof(handle) == sizeof(raw pointer)`, all
derive from `gst::Handle<T>` (`core/handle.hpp`)

| Symbol | Wraps |
|---|---|
| `gst::Element` | `GstElement*` |
| `gst::Pipeline` | `GstElement*` (a `GstPipeline`); implicitly converts to `Element` |
| `gst::Bin` | `GstElement*` (a `GstBin`) |
| `gst::Bus` | `GstBus*` |
| `gst::Pad`, `gst::GhostPad` | `GstPad*` |
| `gst::Caps` | `GstCaps*` |
| `gst::Message` | `GstMessage*` |
| `gst::Buffer` | `GstBuffer*` |
| `gst::Structure` | `GstStructure*` |

**`unique_ptr` aliases** (implementation detail behind `gst::raii::*`; still returned
by some free functions)

| Symbol | Purpose |
|---|---|
| `gst::ElementPtr` | `unique_ptr<GstElement, GstElementDeleter>` |
| `gst::BusPtr` | `unique_ptr<GstBus, GstBusDeleter>` |
| `gst::ErrorPtr` | `unique_ptr<GError, GstErrorDeleter>` |
| `gst::MessagePtr` | `unique_ptr<GstMessage, GstMessageDeleter>` |
| `gst::PadPtr` | `unique_ptr<GstPad, GstPadDeleter>` |
| `gst::CapsPtr` | `unique_ptr<GstCaps, GstCapsDeleter>` |
| `gst::MessageType` | Scoped enum wrapping `GstMessageType`; bitmask ops via `Flags<>` |
| `gst::MessageTypeFlags` | `Flags<MessageType>` — the bitmask type taken by `bus_timed_pop_filtered` |
| `gst::StateChange` | POD struct holding `old_state`, `new_state`, `pending` |

**Free functions**

| Symbol | Returns |
|---|---|
| `gst::init(span<char*>)` | `void` — initialises GStreamer once (static guard) |
| `gst::parse_launch(string_view)` | `expected<Element, ErrorPtr>` |
| `gst::pipeline_new(string_view name={})` | `expected<Pipeline, string>` |
| `gst::element_factory_make(factory, name={})` | `expected<Element, string>` |
| `gst::bin_add(Pipeline, Element)` | `expected<Element, string>` — transfers ownership into bin |
| `gst::element_link(src, sink)` | `expected<void, string>` |
| `gst::element_get_bus(Element)` | `expected<BusPtr, string>` |
| `gst::element_set_state(Element, GstState)` | `expected<void, string>` |
| `gst::element_get_static_pad(element, name)` | `expected<PadPtr, string>` |
| `gst::pad_is_linked(PadPtr)` | `bool` |
| `gst::pad_link(src, sink)` | `expected<void, string>` |
| `gst::pad_get_current_caps(GstPad*)` | `expected<CapsPtr, string>` |
| `gst::caps_from_string(string_view)` | `expected<CapsPtr, string>` |
| `gst::caps_get_structure(CapsPtr, index=0)` | `expected<const GstStructure*, string>` |
| `gst::structure_get_name(GstStructure*)` | `string_view` |
| `gst::message_type(MessagePtr)` | `MessageType` |
| `gst::message_parse_error(GstMessage*)` | `expected<pair<string,string>, string>` |
| `gst::message_parse_state_changed(MessagePtr)` | `StateChange` |
| `gst::state_get_name(GstState)` | `string_view` |
| `gst::bus_timed_pop_filtered(Bus, timeout, MessageTypeFlags)` | `expected<MessagePtr, string>` |

Errors in `gst::` are plain `std::string` (except `parse_launch`, which yields
`ErrorPtr`). The structured `ds::Error` types are `ds::`-only — a `gst::Error` is
still a roadmap item.

## `gst` namespace — pipeline DSL

Declarative DSL. Descriptors live in `gstreamer.hpp`; `build()` lives in
`gstreamer_raii.hpp` because it returns an owning pipeline. There is no
`pipeline.hpp` header or `pipeline_hpp` target.

| Symbol | Purpose |
|---|---|
| `gst::PropertyValue` | `variant<bool, int32, uint32, int64, uint64, double, string>` — typed element property |
| `gst::Node` | Describes one element: factory name, optional instance name, and properties (set via `.prop(key, value)` chaining) |
| `gst::PipelineDesc` | Ordered list of `Node`s that form a linear pipeline |
| `gst::build(PipelineDesc)` | Creates, configures, and links all elements; returns `expected<gst::raii::Pipeline, string>` |

## `ds` namespace — `include/elements.hpp`, `include/builder.hpp`

Typed factory helpers for DeepStream pipeline nodes, one header per family under
`include/elements/`: `sources.hpp`, `transformations.hpp`, `inference.hpp`,
`tracking.hpp`, `sinks.hpp`, `encode.hpp`, `messaging.hpp`, `auxiliary.hpp`,
`smart_record.hpp`. Each element stores a `gst::raii::Element` internally.

`builder.hpp` provides `ds::Builder` — `Builder{}.add(element)....build()`, which
validates duplicate names and static-pad-template caps compatibility and returns
`expected<gst::raii::Pipeline, ds::PipelineError>`. It links linearly; branching
topologies and domain chain methods (`.source().mux().infer()`) are not implemented.

## `ds` namespace — `include/metadata/*.hpp`

Zero-cost views over NvDs metadata structures. Only compiled when DeepStream is found. Requires linking `ds::metadata`.

| Header | Contents |
|---|---|
| `metadata/batch_meta.hpp` | `NvDsBatchMeta` view |
| `metadata/frame_meta.hpp` | `NvDsFrameMeta` view |
| `metadata/object_meta.hpp` | `NvDsObjectMeta` view |
| `metadata/classifier_meta.hpp` | `NvDsClassifierMeta` view |
| `metadata/tensor_meta.hpp` | `NvDsInferTensorMeta` view |
| `metadata/user_meta.hpp` | `NvDsUserMeta` view |
| `metadata/meta_list_view.hpp` | Range adaptor over `NvDsMetaList` |

## `ds` namespace — `include/utils/`

- `utils/error.hpp` — `ds::ErrorKind` enum and `ds::Error` structured error type
- `utils/debug.hpp` — debug/logging helpers
