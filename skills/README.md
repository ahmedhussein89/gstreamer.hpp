# gstreamer.hpp Agentic Skills

Structured knowledge packages that an AI coding assistant (Claude Code, Cursor, and others
supporting the `SKILL.md` convention) discovers and loads automatically while working in this
repository.

Ported from the [NVIDIA DeepStream skills](https://github.com/NVIDIA/DeepStream/tree/main/skills)
and rewritten for `gstreamer.hpp`. The DeepStream set is Python `pyservicemaker` + NVIDIA hardware;
this set is header-only C++20 wrapper development. What carried over is the *shape* — a domain skill
with on-demand references, workflow skills with validated scripts, an eval set per skill — not the
content. Skills without an analogue here (`deepstream-import-vision-model`, `deepstream-run-mv3dt`,
`deepstream-sop`, the four `amc-*` calibration skills) were dropped rather than hollowed out.

## The skills

| Skill | Mode | Use when you want to… |
|---|---|---|
| [`gstreamer-hpp-dev`](gstreamer-hpp-dev/) | Reference | Write, review, or debug anything in `include/`, `tests/`, or `tutorials/` — the architecture, error model, ownership rules, and formatting contract |
| [`gst-hpp-wrap-api`](gst-hpp-wrap-api/) | Workflow | Wrap a new `gst_*` function, add a handle type, a `gst::raii::` class, an enum, or a `Flags` bitmask |
| [`gst-hpp-pipeline`](gst-hpp-pipeline/) | Workflow | Write a pipeline in C++, translate a `gst-launch-1.0` string, or debug one that won't link |
| [`gst-hpp-build-test`](gst-hpp-build-test/) | Workflow | Configure, build, test, sanitize, lint, or reproduce a CI failure |
| [`gst-hpp-tutorial`](gst-hpp-tutorial/) | Workflow | Add a tutorial with all three variants and its CMake and doc wiring |

`gstreamer-hpp-dev` is the one to read first; the other four link back into its references rather
than restating them.

## Layout

```
skills/
├── README.md
├── gstreamer-hpp-dev/
│   ├── SKILL.md
│   ├── skill-card.md
│   ├── evals/evals.json
│   └── references/          ownership-and-transfer, api-surface, error-model,
│                            concepts-and-templates, test-patterns, troubleshooting
├── gst-hpp-wrap-api/
│   ├── SKILL.md · skill-card.md · evals/ · references/wrapping-recipes.md
│   └── scripts/             check_wrapper_conventions.py, api_coverage.py
├── gst-hpp-pipeline/
│   ├── SKILL.md · skill-card.md · evals/ · references/pipeline-patterns.md
│   └── scripts/             validate_pipeline.py
├── gst-hpp-build-test/
│   ├── SKILL.md · skill-card.md · evals/ · (references live in gstreamer-hpp-dev)
│   └── scripts/             preflight.sh
└── gst-hpp-tutorial/
    ├── SKILL.md · skill-card.md · evals/ · references/tutorial-anatomy.md
    └── scripts/             new_tutorial.py
```

## Install

Drop the `skills/` directory at the repo root. Claude Code picks up `.claude/skills/`, so either
place it there or symlink:

```bash
mkdir -p .claude
ln -s ../skills .claude/skills
```

`CLAUDE.md` stays the always-loaded summary. The skills are the on-demand depth beneath it — add a
pointer so the agent knows they exist:

```markdown
## Skills

Task-specific guidance lives in `skills/`. Load the relevant `SKILL.md` before starting:
`gstreamer-hpp-dev` (architecture, ownership, error model), `gst-hpp-wrap-api` (adding a wrapper),
`gst-hpp-pipeline` (pipeline code), `gst-hpp-build-test` (build/test/lint), `gst-hpp-tutorial`
(new tutorials).
```

## Scripts

All are stdlib-only Python 3.8+ or plain bash, run from the repo root, and exit non-zero on
findings — so any of them can be wired into CI or a pre-commit hook.

| Script | Purpose | Verified against |
|---|---|---|
| `gst-hpp-wrap-api/scripts/check_wrapper_conventions.py` | Nine repo conventions the compiler can't catch (E001–E009) | Clean on current `include/`; all nine fire on a synthetic bad header |
| `gst-hpp-wrap-api/scripts/api_coverage.py` | Wrapper ↔ `gst_*` mapping, `--symbol` lookup, `--unwrapped` gap analysis | 97 wrapper names / 120 definitions / 96 distinct C functions |
| `gst-hpp-pipeline/scripts/validate_pipeline.py` | Validate a pipeline and emit DSL / `parse_launch` / manual C++ | Linear, tee, `decodebin`, caps-filter, and malformed inputs |
| `gst-hpp-build-test/scripts/preflight.sh` | Environment report separating real gaps from expected ones | Correctly flags missing GStreamer, submodules, and toolchain |
| `gst-hpp-tutorial/scripts/new_tutorial.py` | Scaffold a three-variant tutorial + CMake wiring | Generates and wires a tutorial; refuses duplicates and bad names |

Suggested CI addition, after the existing `check-concepts` step in the `lint` job:

```yaml
      - name: check wrapper conventions
        run: python3 skills/gst-hpp-wrap-api/scripts/check_wrapper_conventions.py
```

## Documentation drift these skills record

Encoded so the agent corrects rather than propagates:

- `README.md` still calls `gst::Element` a "Move-only RAII wrapper". Post-split it is the
  **non-owning handle**; `gst::raii::Element` owns. `CLAUDE.md` is correct.
- `README.md` Building section says all builds must run inside the dev container. `CLAUDE.md` says
  host builds are the default and the container is optional. `CLAUDE.md` is current.
- `tutorials/easy/README.md` lists `main_declarative.cpp` / `main_dynamic.cpp` variants that don't
  exist, and a `-DDS_BUILD_TUTORIALS=ON` option that should be `GST_BUILD_TUTORIALS`.
- `scripts/check-concepts.sh` points at `docs/concepts-roadmap.md`, which doesn't exist — the
  vocabulary is in `include/core/concepts.hpp`, the plan in `docs/roadmap.md`.

## Evals

Each skill ships `evals/evals.json`: tasks with a ground truth and expected behaviours, including
negative cases that should *not* activate the skill. They are written for review, not yet scored by
a harness — unlike the upstream DeepStream skills, there is no `BENCHMARK.md` here, because no
benchmark has been run. Publishing numbers without a run would be worse than publishing none.

## Maintaining

When the code and a skill disagree, the code wins. Update the skill in the same PR — a stale skill
is more harmful than no skill, because the agent trusts it. `references/api-surface.md` and the
critical-rules lists in each `SKILL.md` are the parts most likely to go stale.
