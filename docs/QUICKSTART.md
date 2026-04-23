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

See [COOKBOOK.md](COOKBOOK.md) and [../examples/README.md](../examples/README.md) for the official runtime-path examples.
