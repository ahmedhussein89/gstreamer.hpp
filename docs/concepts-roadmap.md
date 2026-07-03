# Roadmap: Mandatory C++20 Concepts across `include/`

**Goal:** every template in `include/` is constrained by a named C++20 `concept`
(or a `requires` clause). No template parameter may be an unconstrained bare
`typename T` / `class T`. The rule is *forced* by tooling in CI, not left to
reviewer discipline.

**Why:** the project already targets C++20 (`Standard: c++20`). Today the headers
carry 11 template sites across 8 files, and **none** are constrained — a caller
who passes the wrong type gets a deep, unreadable instantiation error instead of
a one-line "T does not satisfy `gst::HandlePointer`". Concepts give us the same
compile-time contract `vulkan.hpp` gets from its hand-written traits, plus
readable diagnostics and self-documenting APIs.

Legend: ✅ complete · 🔶 partial · 🔜 next · ⬜ not started.

---

## Current state (audit)

| File | Template site(s) | Param(s) today | Target concept |
| --- | --- | --- | --- |
| `core/handle.hpp` | `Handle<T>` | `typename T` | `GstHandlePointer` |
| `core/flags.hpp` | `FlagTraits<Bits>`, `Flags<Bits>`, ADL `\|`/`&`/`^`/`~` | `typename Bits` | `FlagEnum` |
| `core/array_proxy.hpp` | `ArrayProxy<T>` (+ `array<T,N>` ctor) | `typename T` | `ArrayElement` (or `std::copyable`) |
| `builder.hpp` | `Builder::add<T>` | `typename T` | `DsElement` |
| `pipeline.hpp` | `Node::prop<T>`, `PipelineDesc(Nodes&&...)` | `typename T`, `typename... Nodes` | `PropertyValueType`, `std::same_as<Node>` |
| `metadata/meta_list_view.hpp` | `MetaListView<View, Native>` | `typename View, typename Native` | `MetaView` (View constructible from `Native*`) |
| `utils/error.hpp` | (1 template line — audit in Phase 0) | TBD | TBD |

Status: **12 / 12 sites constrained.** ✅

---

## Phase 0 — Enforcement harness first ✅

Force-the-rule tooling must land *first*, wired to fail red on any unconstrained
template. Otherwise new unconstrained templates keep slipping in while we
retrofit the old ones.

- [x] `scripts/check-concepts.sh` — bash gate; scans every `include/**/*.hpp`;
      distinguishes concept definitions (skips them), `requires` clauses
      (allows them), and bare unconstrained params (fails with file:line).
- [x] `CLAUDE.md` Code Conventions updated with the mandatory constraint rule.
- [ ] `.clang-tidy` opt-in: `modernize-use-constraints` (container tooling TBD).
- [ ] GitHub Actions step that runs `scripts/check-concepts.sh` on every PR.

**Deliverable:** a red/green gate. ✅ (script live; CI wiring pending)

---

## Phase 1 — Concept vocabulary in `core/concepts.hpp` ✅

Create one home for shared constraints, mirroring how `core/flags.hpp` /
`core/handle.hpp` centralise primitives. Pulled in by `core/core.hpp`.

- [x] `gst::GstHandlePointer<T>` — `std::is_class_v<T>` (GObject pointee types are C
      structs ≡ C++ class types). Lives in `core/concepts.hpp`.
- [x] `gst::FlagEnum<Bits>` — `std::is_enum_v<Bits>`. Lives in `core/concepts.hpp`.
- [x] `gst::ArrayElement<T>` — `std::copyable<T>`. Lives in `core/concepts.hpp`.
- [x] `gst::PropertyValueType<T>` — `std::constructible_from<PropertyValue, T>`. Lives
      in `pipeline.hpp` (local; depends on `PropertyValue` defined in that file).
- [x] `gst::PipelineNodeType<T>` — `std::same_as<std::remove_cvref_t<T>, Node>`. Lives
      in `pipeline.hpp`.
- [x] `ds::DsElement<T>` — structural concept on `get()`/`release()` → `GstElement*`.
      Lives in `core/concepts.hpp`; the comment at `builder.hpp:115` was the spec.
- [x] `ds::MetaView<View, Native>` — `std::constructible_from<View, Native*>`. Lives
      in `metadata/meta_list_view.hpp` (local; avoids pulling nvdsmeta.h into core).

**Deliverable:** `include/core/concepts.hpp` + local concepts in their respective headers. ✅

---

## Phase 2 — Retrofit `core/` templates ✅

The foundation layer — smallest, highest-leverage, no external callers to break.

- [x] `core/handle.hpp`: `template <GstHandlePointer T> struct Handle`.
- [x] `core/flags.hpp`: `template <FlagEnum Bits> struct FlagTraits` / `Flags`
      and the four ADL operator templates (`|`, `&`, `^`, `~`).
- [x] `core/array_proxy.hpp`: `template <ArrayElement T> class ArrayProxy`.

Each change is source-compatible — existing valid instantiations still compile;
only invalid ones now fail fast.

## Phase 3 — Retrofit `ds::` and DSL templates ✅

- [x] `builder.hpp`: `template <DsElement T> Builder& add(T&& element)` — the
      now-redundant hand-written comment was removed.
- [x] `pipeline.hpp`: `template <PropertyValueType T> Node prop(...)` and
      `PipelineDesc(Nodes&&...)` with `requires (PipelineNodeType<Nodes> && ...)`.
- [x] `metadata/meta_list_view.hpp`: kept `<View, Native>` order, added
      `requires MetaView<View, Native>` clause — no call-site changes needed.
- [x] `utils/error.hpp`: audited — contains no template parameters; no change needed.

## Phase 4 — Tests, lock-in, docs ✅

- [x] `tests/testConcepts.cpp`: positive + negative `static_assert` coverage for
      all seven concepts. `testConcepts` added to `tests/CMakeLists.txt`.
- [x] Gate is live: `scripts/check-concepts.sh` exits 0 (all 12 sites constrained).
- [x] `CLAUDE.md` Code Conventions updated with the mandatory constraint rule.
- [ ] GitHub Actions CI step (wiring pending — depends on Phase 0 CI work in main roadmap).
- [ ] "Concepts" section in `docs/roadmap.md` §1.3 (cosmetic, deferred).

---

## Definition of done

1. ✅ `scripts/check-concepts.sh` reports **0** unconstrained templates in `include/`.
2. ✅ Every concept has positive + negative `static_assert` coverage (`tests/testConcepts.cpp`).
3. ⬜ The gate is a required CI check; a PR reintroducing a bare `typename` in a
   header fails automatically (GitHub Actions wiring pending).
4. ✅ `CLAUDE.md` Code Conventions states the rule.
