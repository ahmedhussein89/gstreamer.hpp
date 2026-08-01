---
name: gstreamer-hpp-dev
description: Development on the gstreamer.hpp header-only C++20 GStreamer wrapper. Use when writing, reviewing, or debugging code in include/gstreamer.hpp, include/gstreamer_raii.hpp, include/core/*, tests/, or tutorials/ — anything touching gst:: handles, gst::raii:: owning types, nonstd::expected returns, the pipeline DSL, C++20 concept constraints, or GStreamer ownership/refcount semantics.
owner: ahmedhussein89/gstreamer.hpp
service: gstreamer-hpp
version: 1.0.0
reviewed: 2026-08-01
license: same-as-repo
---

# gstreamer.hpp Development Skill

When this skill is active, **read the relevant reference document before writing code**. The
references encode the repo's exact ownership rules, error model, and formatting contract. Do NOT
generate wrapper code from general GStreamer knowledge — GStreamer's C transfer semantics are the
single largest source of bugs in this codebase and they are documented per-pattern in
`references/ownership-and-transfer.md`.

## Architecture in 30 seconds

Two layers, `vulkan.hpp`-style. Getting the layer wrong is the most common review rejection.

| Layer | Namespace | Header | Owns? | Shape |
|---|---|---|---|---|
| Enhanced | `gst::` | `gstreamer.hpp`, `core/*.hpp` | **No** | `struct X : Handle<GstX>` — trivially copyable, `sizeof(GstX*)`, implicit to/from raw pointer |
| RAII | `gst::raii::` | `gstreamer_raii.hpp` | **Yes** | move-only class holding an `XPtr`, with `operator gst::X()` |
| DSL (descriptors) | `gst::` | `gstreamer.hpp` | n/a | `Node`, `PipelineDesc`, `PropertyValue` — no GStreamer resources |
| DSL (builder) | `gst::` | `gstreamer_raii.hpp` | yes | `gst::build()` returns `raii::Pipeline`, so it lives in the RAII header |

`XPtr` aliases (`ElementPtr`, `BusPtr`, `CapsPtr`, …) are `unique_ptr` with a custom deleter. They
are the **implementation detail behind `gst::raii::*`**, not the pattern to copy for new owning
types. They *are* still the correct return type for a transfer-full free function in the enhanced
layer (`element_get_bus` → `expected<BusPtr, string>`).

## Critical Rules

1. **Every fallible function returns `nonstd::expected<T, E>`.** Never raw pointers, never
   exceptions, never `bool` + out-param. Fail via `nonstd::make_unexpected(...)`.
   - Error type is `std::string` (usually `fmt::format`-built) everywhere **except**
     `gst::parse_launch`, which returns `ErrorPtr` because GStreamer hands back a real `GError`.
   - Infallible accessors return the value directly and are marked `noexcept`.

2. **Every template in `include/` must constrain its type parameters.** A named C++20 concept
   (`template <FlagEnum Bits>`) or a `requires` clause. Bare `typename T` / `class T` fails
   `scripts/check-concepts.sh`, which is a CI gate. Shared vocabulary lives in
   `include/core/concepts.hpp`; header-local concepts live next to their use.
   - **Keep the template head on one line.** The checker only regex-matches a single-line
     `template <...>` and a 3-line context window for `requires`. A multi-line template head can
     both false-positive and false-negative. See `references/concepts-and-templates.md`.

3. **Match GStreamer transfer semantics exactly.** Read
   `references/ownership-and-transfer.md` before wrapping any new C function.
   - transfer-full **out** → return `XPtr` (enhanced) or `raii::X` (RAII layer)
   - transfer-none **out** → return the bare `gst::X` handle or `std::string_view`
   - transfer-full **in** → take the `XPtr`/`raii::X` **by value** and call `.release()`
   - floating refs (`gst_element_factory_make`) → sunk by `gst_bin_add`; `bin_add` must
     `gst_object_unref` the raw pointer on the failure path

4. **New owning wrappers go in `gstreamer_raii.hpp`**, as move-only classes: deleted copy,
   defaulted move, `get()`, `release()`, `explicit operator bool()`, implicit `operator gst::X()`,
   and `static_assert`s for the move-only invariants right after the class.

5. **`std::string_view` never reaches C directly.** Always materialize:
   `std::string name_str(name); ... name_str.c_str();` — `sv.data()` is not NUL-terminated.

6. **Optional string params default to `{}` and map empty → `nullptr`:**
   `name.empty() ? nullptr : name_str.c_str()`.

7. **`GList` returns become owning `std::vector<XPtr>`.** Adopt each element into a `Ptr`, then
   `g_list_free(list)` to free only the spine — never `g_list_free_full` on a list whose elements
   you already adopted. See the `registry_get_plugin_list` pattern.

8. **Callbacks heap-allocate the `std::function` and free it via `GDestroyNotify`.** This is the
   only place the codebase uses `new`. Pattern is fixed — copy `bus_add_watch` / `pad_add_probe`
   verbatim rather than inventing a variant.

9. **Optional-dependency headers are quarantined.** `gstreamer_harness.hpp` (needs
   `gstreamer-check-1.0`) and `gstreamer_rtsp.hpp` (needs `gstreamer-rtsp-server-1.0`) are wrapped
   in `#if __has_include(...)` and are **deliberately excluded** from the `gstreamer_hpp` INTERFACE
   target's sources in `include/CMakeLists.txt`. Do not add them there — that would force the extra
   link onto every consumer. Consumers include them directly and link the library themselves.

10. **`-Werror` is not universal.** Targets linking `gstreamer::warnings_strict` (`include/`,
    `tests/`) are `-Werror`. Tutorials link plain `gstreamer::warnings` and tolerate warnings. Do
    not "fix" a tutorial by weakening a header.

11. **Docs are part of the change.** Any new/changed public symbol updates the symbol tables in
    both `README.md` and `docs/api-reference.md` in the same commit.
    - **Known drift to fix, not copy:** `README.md` still describes `gst::Element` as "Move-only
      RAII wrapper around `GstElement*`". That is stale — post-split, `gst::Element` is the
      non-owning handle and `gst::raii::Element` is the owning type. `CLAUDE.md` is correct.

12. **Formatting contract** (enforced by `.clang-format`, CI-gated): C++20, 2-space indent,
    130 columns, left-aligned pointers, `if(cond)` with no space after `if`, four spaces before a
    trailing comment (`}    // namespace gst`). Include order: STL → boost → fmt → range → gst →
    gtest → other third-party → project headers.

## Environment Facts (host)

Build directly on the host — no container wrapper needed. The devcontainer remains valid for
clean-room/CI-equivalent builds.

| | |
|---|---|
| Toolchain | cmake 4.x, ninja, make, g++ 15 |
| GStreamer | 1.28.x — core, video, base, net, check, pbutils |
| **Absent** | `gstreamer-rtsp-server-1.0`, `clang++`, `clang-format`, `clang-tidy` |

Consequences: the `RTSPServer` tutorial self-skips at configure time (expected, not a failure), and
formatting/lint must be run in the devcontainer or left to CI. Verify the environment before
assuming a breakage is your code:

```bash
pkg-config --modversion gstreamer-1.0 gstreamer-base-1.0 gstreamer-check-1.0
gst-inspect-1.0 compositor >/dev/null && echo "plugins ok"
```

## Reference Documents

| Document | Read when |
|---|---|
| [references/ownership-and-transfer.md](references/ownership-and-transfer.md) | Wrapping any C function; anything touching refcounts, floating refs, or `release()` |
| [references/api-surface.md](references/api-surface.md) | Looking up what already exists before adding a duplicate; naming a new function |
| [references/error-model.md](references/error-model.md) | Choosing return type, error string wording, `expected` composition |
| [references/concepts-and-templates.md](references/concepts-and-templates.md) | Writing any template; satisfying `check-concepts.sh` |
| [references/test-patterns.md](references/test-patterns.md) | Deciding which test binary a test belongs in; GTest conventions |
| [references/troubleshooting.md](references/troubleshooting.md) | A build, link, sanitizer, or runtime failure |

## Quick Error Reference

| Symptom | Cause / fix |
|---|---|
| `check-concepts.sh` fails on a line that *is* constrained | Template head spans multiple lines — collapse it to one line |
| `error: no matching function ... gst::Element` | Passing a `raii::X` where a `gst::X` is expected is fine (implicit conversion); the reverse is not. Don't construct `raii::X` from a non-owning handle without taking a ref |
| Double-free / `GStreamer-CRITICAL: assertion 'object->ref_count > 0'` | A transfer-full-in wrapper forgot `.release()`, or a transfer-none-out was returned as an `XPtr` |
| Leak reported only under `-DGST_ENABLE_SANITIZERS=ON` | Failure path of a transfer-full-in wrapper didn't unref the raw pointer (see `bin_add`) |
| Element created but pipeline won't link | `gst_element_factory_make` returns a **floating** ref; it must be added to a bin before it is valid to link |
| `undefined reference to gst_harness_*` | Test target must link `GStreamer::Check` itself — `gstreamer_hpp` deliberately does not |
| `Skipping RTSPServer tutorial: gst-rtsp-server not found` | Expected on this host. Install `libgstrtspserver-1.0-dev` only if you need it |
| clang-format/clang-tidy "not found" | Not installed on the host by design — run in the devcontainer or let CI enforce |
| Tutorial warning became an error | You linked `gstreamer::warnings_strict` in a tutorial; tutorials use `gstreamer::warnings` |

## Related Skills

- `gst-hpp-wrap-api` — the step-by-step workflow for adding a new wrapper (use this for "wrap X")
- `gst-hpp-pipeline` — authoring/validating pipelines with the DSL or `parse_launch`
- `gst-hpp-build-test` — configure/build/test/sanitize/coverage loop and its failure modes
- `gst-hpp-tutorial` — scaffolding a new three-variant tutorial
