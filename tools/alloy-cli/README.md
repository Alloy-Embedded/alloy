# alloy-cli

User-facing entry point for the Alloy multi-vendor bare-metal runtime.

This package provides the `alloy` command. Phase 1 wraps the existing
`scripts/alloyctl.py` shipped inside an Alloy runtime checkout. Later phases add an SDK
manager, a toolchain manager, project scaffolding, and editor integration so that a fresh
machine can go from zero to a flashing board with a single install command.

The full design lives in
[`openspec/changes/add-project-scaffolding-cli`](../../openspec/changes/add-project-scaffolding-cli/).

## Install (development)

```bash
pipx install --editable tools/alloy-cli
# or, with uv:
uv tool install --editable tools/alloy-cli
```

## Usage

```bash
alloy --help
alloy doctor
alloy configure --board nucleo_g071rb
alloy flash --board nucleo_g071rb --target blink --build-first
```

The CLI locates the runtime checkout in this order:

1. The `ALLOY_ROOT` environment variable, when set.
2. The current working directory, walking up until `scripts/alloyctl.py` and
   `cmake/board_manifest.cmake` are found.

Set `ALLOY_ROOT` when invoking `alloy` from outside a checkout, e.g. inside a downstream
project tree:

```bash
export ALLOY_ROOT=$HOME/code/alloy
alloy doctor
```

A future release will replace this lookup with a versioned cache under `~/.alloy/sdk/`.
