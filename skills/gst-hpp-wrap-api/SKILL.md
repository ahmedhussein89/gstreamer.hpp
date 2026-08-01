---
name: gst-hpp-wrap-api
description: Add a new GStreamer C API wrapper to gstreamer.hpp. Use when asked to wrap, expose, or port a gst_* function; add a new gst:: handle type or gst::raii:: owning class; add a scoped enum or Flags bitmask over a GstXxxFlags; or extend the pipeline DSL. Covers the full change set — header, layer choice, transfer semantics, tests, docs, and the CI gates.
owner: ahmedhussein89/gstreamer.hpp
service: gstreamer-hpp
version: 1.0.0
reviewed: 2026-08-01
license: same-as-repo
---

# Wrapping a New GStreamer API

A wrapper is not done when it compiles. It is done when the header, the tests, the docs, and both
CI gates agree. This skill is the checklist.

> **Read `gstreamer-hpp-dev/references/ownership-and-transfer.md` first.** Transfer semantics
> determine the signature, and the signature determines everything else.

## Step 0 — Check it doesn't already exist

```bash
grep -n "gst_element_query_position\|element_query_position" include/*.hpp include/core/*.hpp
python3 <skill-path>/scripts/api_coverage.py --symbol element_query_position
```

`gstreamer-hpp-dev/references/api-surface.md` has the full index by area. Overloads for the
handle-vs-`Ptr` case are expected and encouraged; a second function with a different name for the
same C call is not.

## Step 1 — Look up the transfer annotation

From the GStreamer 1.28 docs or the installed header:

```bash
grep -B8 "gst_element_get_bus" /usr/include/gstreamer-1.0/gst/gstelement.h
gst-inspect-1.0 <element>          # for property names/types, if wrapping a property
```

Write down: return transfer (full/none/floating), each parameter's transfer, and whether the
function can fail. Do not proceed on a guess — every ownership bug in this codebase traces back to
a skipped Step 1.

## Step 2 — Choose the layer

| The wrapper… | Goes in |
|---|---|
| returns a borrowed pointer, or a value | `gstreamer.hpp`, enhanced layer |
| returns an owned resource | `gstreamer.hpp` as `expected<XPtr, string>` **and** optionally `gstreamer_raii.hpp` as `expected<raii::X, string>` |
| is a new owning class | `gstreamer_raii.hpp` |
| is a scoped enum or `to_string` | `include/core/enums.hpp` |
| is a bitmask over a `GstXxxFlags` | `FlagTraits` specialization in `gstreamer.hpp`, using `Flags<Bits>` from `core/flags.hpp` |
| is a new shared concept | `include/core/concepts.hpp` — only if a second header needs it |
| needs `gstreamer-check` or `gst-rtsp-server` | `gstreamer_harness.hpp` / `gstreamer_rtsp.hpp`, inside the existing `__has_include` guard, **not** added to `include/CMakeLists.txt` sources |

## Step 3 — Write it

Recipes for each shape are in [references/wrapping-recipes.md](references/wrapping-recipes.md).
Non-negotiables:

- `inline` (header-only), `noexcept` only if it genuinely cannot fail
- `[[nodiscard]]` on pure accessors
- `std::string_view` params → `std::string name_str(name)` before `.c_str()`
- optional names default to `{}` and map empty → `nullptr`
- `nonstd::make_unexpected` for failures; `fmt::format` when interpolating the caller's identifier
- template heads on **one line**, every type parameter constrained
- a comment stating the transfer contract wherever it isn't obvious

## Step 4 — Test it

Target binary and required test shapes: `gstreamer-hpp-dev/references/test-patterns.md`.

Minimum for a free function: happy path, failure path, and — if it takes a name — that the error
message contains that name. Minimum for a new `raii::X`: the six-test block
(`ConstructFromRawPointer`, `ConstructFromXPtr`, `DefaultConstructorIsNull`,
`ImplicitConversionToHandle`, `ReleaseTransfersOwnership`, `MoveOnly`).

Only `-plugins-base`/`-plugins-good` elements are available in CI.

## Step 5 — Run the gates

```bash
bash scripts/check-concepts.sh                                    # CI gate 1
python3 <skill-path>/scripts/check_wrapper_conventions.py         # repo conventions
cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure
cmake -B build-asan -S . -DGST_ENABLE_SANITIZERS=ON -DGST_SANITIZER=address \
  && cmake --build build-asan && ctest --test-dir build-asan --output-on-failure
```

`clang-format`/`clang-tidy` are **not installed on this host** — CI gate 2 runs them. Match
`.clang-format` by hand: 2-space indent, 130 columns, `if(` with no space, four spaces before a
trailing comment.

## Step 6 — Update the docs in the same commit

- `docs/api-reference.md` — the canonical symbol table
- `README.md` — the user-facing table for the matching layer
- `docs/roadmap.md` — tick the phase item if this closes one

If `README.md` and `docs/api-reference.md` disagree with the header, the header wins and the docs
get fixed. (Known drift: `README.md` still calls `gst::Element` a move-only RAII wrapper. It is the
non-owning handle. Fix it when you touch that table.)

## Scripts

| Script | Purpose |
|---|---|
| `scripts/check_wrapper_conventions.py` | Lints `include/` for repo-specific rules the compiler can't catch: raw-pointer returns from fallible functions, `throw`, `sv.data()` passed to C, missing `inline`, unconstrained templates, multi-line template heads, over-long lines |
| `scripts/api_coverage.py` | Reports which `gst_*` C functions the headers already call, which wrappers exist, and what a named symbol maps to |

Both are stdlib-only Python 3.8+, run from the repo root, and exit non-zero on findings so they can
be wired into CI or a pre-commit hook.

## Definition of done

- [ ] Transfer annotation looked up, not guessed; comment records it
- [ ] Correct layer, correct return type per the decision table
- [ ] Error path unrefs (or documents why it must not)
- [ ] Tests in the right binary, including failure path; pass under ASan
- [ ] `check-concepts.sh` and `check_wrapper_conventions.py` clean
- [ ] `docs/api-reference.md` + `README.md` updated
