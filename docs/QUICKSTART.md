# Alloy Quickstart

This is the shortest supported path from clone to a visible `blink` or runtime validation result.

## Supported Entry Points

Use one of these public flows:

- `python3 scripts/alloyctl.py ...` for board-oriented configure, build, flash, monitor, and validation
- `cmake --preset ...`, `cmake --build --preset ...`, and `cmake --workflow --preset ...` for explicit preset-oriented validation

If a flow is not documented here or in [BOARD_TOOLING.md](BOARD_TOOLING.md), it is not part of the supported product story.

## Fastest Path To Blink

Choose one foundational board:

### SAME70 Xplained

```bash
python3 scripts/alloyctl.py flash --board same70_xplained --target blink --build-first
python3 scripts/alloyctl.py monitor --board same70_xplained
```

### Nucleo-G071RB

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --build-first
python3 scripts/alloyctl.py monitor --board nucleo_g071rb
```

### Nucleo-F401RE

```bash
python3 scripts/alloyctl.py flash --board nucleo_f401re --target blink --build-first
python3 scripts/alloyctl.py monitor --board nucleo_f401re
```

Notes:

- `flash` uses OpenOCD and requires a working probe connection
- `monitor` auto-detects the UART port when possible; pass `--port` if multiple candidates exist
- if you only want to build, replace `flash` with `build`
- if an STM32 board is trapped by bad firmware, use `python3 scripts/alloyctl.py recover --board <board> --target blink`
- if you want to inspect the supported recovery path before touching hardware, add `--dry-run`
- before the first flash, run `python3 scripts/alloyctl.py doctor` to verify the toolchain, probe
  tooling, python deps, and the pinned `alloy-devices` ref match the repo state

## Preflight And IDE Bootstrap

```bash
python3 scripts/alloyctl.py doctor
python3 scripts/alloyctl.py compile-commands --board same70_xplained --configure
python3 scripts/alloyctl.py info
```

- `doctor` verifies the host toolchain, probe tooling, python deps, and the device-contract ref
- `compile-commands` exposes `compile_commands.json` at the repo root for clangd/LSP-backed IDEs
- `info` prints a machine-readable JSON environment report for bug reports or release audit

To bootstrap a downstream firmware project:

```bash
python3 scripts/alloyctl.py new --board same70_xplained --path ./my-firmware
```

## Configure And Build Explicitly

If you want the CMake build directory prepared before building:

```bash
python3 scripts/alloyctl.py configure --board nucleo_g071rb
python3 scripts/alloyctl.py build --board nucleo_g071rb --target blink
```

`configure` produces a board-specific build directory under `build/hw/...`.

## Run Validation

Host runtime validation:

```bash
python3 scripts/alloyctl.py validate --board host
```

Board-family runtime validation via workflow presets:

```bash
python3 scripts/alloyctl.py validate --board same70_xplained
python3 scripts/alloyctl.py validate --board nucleo_g071rb
python3 scripts/alloyctl.py validate --board nucleo_f401re
```

Optional validation kinds:

```bash
python3 scripts/alloyctl.py validate --board same70_xplained --kind zero-overhead
python3 scripts/alloyctl.py validate --board nucleo_g071rb --kind smoke
python3 scripts/alloyctl.py validate --board nucleo_f401re --kind smoke
```

## Canonical Next Examples

After `blink`, the recommended examples are:

- `time_probe`
- `uart_logger`
- `dma_probe` on boards that publish the required DMA helpers

Use:

```bash
python3 scripts/alloyctl.py explain --board same70_xplained
python3 scripts/alloyctl.py diff --from same70_xplained --to nucleo_g071rb
```

Use `explain --connector ...` before writing a raw route, and use `diff --from ... --to ...` before migrating code between boards so the published clock, debug UART, and release-gate differences are visible up front.

See [COOKBOOK.md](COOKBOOK.md) and the [`examples/` tree on GitHub](https://github.com/lgili/alloy/tree/main/examples) for the official runtime-path examples.
