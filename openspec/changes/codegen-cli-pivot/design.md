# Design: codegen-cli-pivot

## Context

`alloy-devices` repo (pre-generated C++ headers for ~5000 MCUs) is being deleted.
Pre-generation pipeline is unfeasible at that scale.

New model: **local on-demand CLI** — the user runs `alloy-gen` once for the MCU they
are targeting. Headers are generated into their project tree, not committed to a
shared repo.

---

## Old vs New

| | Old (pipeline) | New (CLI) |
|---|---|---|
| Trigger | CI / maintainer `regen --all` | User: `alloy-gen --svd STM32G071.svd ...` |
| Output location | `alloy-devices/{vendor}/{family}/` | `--out <project-dir>` |
| Saved artifacts | `*-ir.json`, `*-coverage.json`, `artifact-manifest.json` | Nothing — headers only |
| Sub-commands | `add`, `regen`, `diff`, `coverage` | Single implicit `generate` |
| Scope | 5000 MCUs pre-committed | One MCU per invocation |

---

## New CLI Surface

```
alloy-gen --svd <path-or-url> \
          --vendor <vendor> \
          --family <family> \
          --device <device> \
          --out <dir> \
          [--patch <extra.yaml> ...] \
          [--strict] \
          [--dry-run]
```

Examples:
```bash
# From local SVD file
alloy-gen --svd STM32G071.svd --vendor st --family stm32g0 --device stm32g071rb --out generated/

# From URL
alloy-gen --svd https://raw.githubusercontent.com/modm-io/cmsis-svd-stm32/main/STM32G071.svd \
          --vendor st --family stm32g0 --device stm32g071rb --out generated/

# With extra patch + strict validation
alloy-gen --svd STM32G071.svd --vendor st --family stm32g0 --device stm32g071rb \
          --out generated/ --patch my-overrides.yaml --strict
```

---

## What Stays

- Ingest: SVD/ATDF → IR dict
- Patches: auto-load from `<codegen>/patches/<vendor>/<family>*.yaml` + `--patch` extras
- Validate: schema checks, print errors (exit 1 if `--strict`)
- Emit: Jinja2 → C++ headers into `--out`
- Coverage: print summary to stdout (not saved to file)

## What Gets Removed

| Item | Reason |
|---|---|
| `cmd_regen` | Needs artifact-manifest; pipeline-only |
| `cmd_diff` | Needs saved IR JSON + git; pipeline-only |
| `cmd_coverage` | Reads committed IR JSON; pipeline-only |
| `_update_artifact_manifest()` | Writes pipeline tracking file |
| `_sha256_bytes()` | Only used by manifest |
| Save IR to `*-ir.json` | Intermediate artifact, not needed locally |
| Save coverage to `*-coverage.json` | CI artifact, not needed locally |
| `ROOT` global (alloy-devices root) | No longer a fixed tree |
| `hashlib`, `subprocess` imports | Used by removed code |

---

## Emitter Output Layout

Given `--out generated/ --vendor st --family stm32g0 --device stm32g071rb`,
emitter writes:

```
generated/
  st/stm32g0/generated/runtime/devices/stm32g071rb/
    connectors.hpp
    routes.hpp
    pins.hpp
    types.hpp
    ...
```

Same path structure as before, rooted at `--out` instead of alloy-devices root.
No change needed in `alloy_cpp_emit/emitter.py` — it already takes `out_dir`.

---

## Distribution

`alloy-gen` will be distributed as part of the `alloy` CLI (see openspec
`alloy-cli-distribution`). The codegen package (`alloy_codegen`, `alloy_svd_ingest`,
`alloy_cpp_emit`, `alloy_ir_validate`) ships alongside it.

Patches directory is bundled with the installed package at
`<package>/patches/<vendor>/`. Users can extend with `--patch`.
