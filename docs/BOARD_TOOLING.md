# Board Tooling

The user-facing entry point is the `alloy` command (see [CLI.md](CLI.md) and
[QUICKSTART.md](QUICKSTART.md) for installation):

```bash
alloy <command> ...
```

For boards outside the in-tree foundational set, declare a custom board in your downstream
project; see [CUSTOM_BOARDS.md](CUSTOM_BOARDS.md) for the runtime contract and a working
recipe. The CLI shortcut is `alloy new --mcu <part>`.

Inside the runtime checkout the legacy `python3 scripts/alloyctl.py <command> ...` form
remains a working alias during the transition; new docs and examples should use `alloy`.

This is the supported board-oriented UX for:

- configure
- build
- bundle
- explain
- diff
- flash
- recover
- monitor
- gdbserver
- sweep
- validation through workflow presets
- compile-commands (clangd/LSP bootstrap)
- info (machine-readable environment report)
- doctor (preflight diagnostics)
- new (downstream starter scaffold)

Supported boards:

- `same70_xplained`
- `nucleo_g071rb`
- `nucleo_f401re`

Validation-only target:

- `host`

## Configure

```bash
python3 scripts/alloyctl.py configure --board same70_xplained
python3 scripts/alloyctl.py configure --board nucleo_g071rb --build-tests
```

`configure` creates or refreshes the board-specific build directory under `build/hw/...`.

## Build

```bash
python3 scripts/alloyctl.py build --board same70_xplained --target uart_logger
python3 scripts/alloyctl.py build --board nucleo_g071rb --target analog_probe
```

## Explain

```bash
python3 scripts/alloyctl.py explain --board same70_xplained
python3 scripts/alloyctl.py explain --board same70_xplained --connector debug-uart
python3 scripts/alloyctl.py explain --board same70_xplained --clock
python3 scripts/alloyctl.py explain --board same70_xplained --peripheral dma
```

`explain` is the supported place to answer board-oriented questions before editing code:

- which published connector alias should I use on this board?
- what clock profile and debug UART does this board actually ship with?
- what release gates or examples are part of the supported story?

If you mistype a connector alias, `alloyctl` now prints the valid alternatives with the board-visible peripheral path.

## Diff

```bash
python3 scripts/alloyctl.py diff --from same70_xplained --to nucleo_g071rb
```

Use `diff` for migration work, not just inspection. The output highlights clock, debug-UART, connector, example, and release-gate differences that usually force code or validation changes.

## Build full hardware bundle

```bash
python3 scripts/alloyctl.py bundle --board same70_xplained
python3 scripts/alloyctl.py bundle --board nucleo_g071rb
python3 scripts/alloyctl.py bundle --board nucleo_f401re
```

`bundle` builds the board's declared hardware validation bundle target.

## Flash

```bash
python3 scripts/alloyctl.py flash --board same70_xplained --target uart_logger --build-first
python3 scripts/alloyctl.py flash --board nucleo_f401re --target dma_probe --build-first
python3 scripts/alloyctl.py flash --board nucleo_f401re --target dma_probe --recover --dry-run
```

Current flash backend:

- SAME70: `openocd -f board/atmel_same70_xplained.cfg`
- STM32G0: `openocd -f interface/stlink.cfg -f target/stm32g0x.cfg`
- STM32F4: `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg`

Supported recovery path:

- use one public command first:

```bash
python3 scripts/alloyctl.py recover --board nucleo_g071rb --target blink
python3 scripts/alloyctl.py recover --board nucleo_f401re --target blink
python3 scripts/alloyctl.py recover --board nucleo_g071rb --target blink --dry-run
```

Use `--dry-run` when you want the maintained public recovery plan without touching hardware. The output tells you which backend `alloyctl` selected and the supported steps it will run.

- STM32 boards can also use `python3 scripts/alloyctl.py flash --board <board> --target <target> --recover`
- when `STM32_Programmer_CLI` is installed, `alloyctl` prefers it automatically for STM32 recovery
- otherwise `alloyctl` falls back to the slower `openocd` recovery path

Board-aware debug and recovery guidance:

- `same70_xplained`: use the EDBG path exposed by the board and confirm logs with `monitor` on the published debug UART
- `nucleo_g071rb`: recover with the public ST-LINK flow first, then reflash `blink` or another small probe before resuming broader bring-up
- `nucleo_f401re`: use the same ST-LINK recovery flow; after recovery, validate with `blink` first if a larger target previously failed to flash

Force one backend explicitly:

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --recover --flash-backend stm32cube
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --recover --flash-backend openocd
```

## UART monitor

```bash
python3 scripts/alloyctl.py monitor --board same70_xplained
python3 scripts/alloyctl.py monitor --board nucleo_f401re --port /dev/cu.usbmodem1234
```

## Sweep

```bash
python3 scripts/alloyctl.py sweep --board same70_xplained -j8
```

## GDB server

```bash
python3 scripts/alloyctl.py gdbserver --board same70_xplained
python3 scripts/alloyctl.py gdbserver --board nucleo_g071rb --gdb-port 3333
```

## Validation

```bash
python3 scripts/alloyctl.py validate --board host
python3 scripts/alloyctl.py validate --board same70_xplained
python3 scripts/alloyctl.py validate --board same70_xplained --kind zero-overhead
python3 scripts/alloyctl.py validate --board nucleo_g071rb --kind smoke
python3 scripts/alloyctl.py validate --board nucleo_f401re
```

Current validation mapping:

- `host` -> `host-mmio-validation`
- `same70_xplained` -> `same70-runtime-validation`, `same70-renode-smoke`, `same70-zero-overhead`
- `nucleo_g071rb` -> `stm32g0-runtime-validation`, `stm32g0-renode-smoke`
- `nucleo_f401re` -> `stm32f4-runtime-validation`, `stm32f4-renode-smoke`

## Compile Commands (clangd/LSP)

```bash
python3 scripts/alloyctl.py compile-commands --board same70_xplained
python3 scripts/alloyctl.py compile-commands --board nucleo_g071rb --configure
```

Symlinks `compile_commands.json` at the repo root to the selected board's build directory so a
clangd-backed IDE picks up the current configure. Pass `--configure` to configure the board build
first when the file is missing. Pass `--copy` to copy instead of symlink.

## Info

```bash
python3 scripts/alloyctl.py info
```

Prints a stable JSON document with: alloy version, pinned `alloy-devices` ref (and whether it
matches the local checkout), board tier membership, required release gates per board, detected
tool versions (cmake, ninja, arm-none-eabi-gcc, openocd, python), and current repo git sha. Use
this for bug reports and release audit.

## Doctor

```bash
python3 scripts/alloyctl.py doctor
```

Preflight check: confirms cmake, the ARM bare-metal toolchain, openocd, and `pyserial` are on
PATH, and that the sibling `../alloy-devices` checkout matches the ref pinned in
`docs/RELEASE_MANIFEST.json`. Any failure prints a hint and exits non-zero.

## New (downstream starter)

```bash
python3 scripts/alloyctl.py new --board same70_xplained --path ./my-firmware
python3 scripts/alloyctl.py new --board nucleo_g071rb --path ./my-firmware --name my_fw
```

Scaffolds a minimal downstream firmware project targeting a foundational board. The generated
tree uses the documented CMake package consumption path — see
[CMAKE_CONSUMPTION.md](CMAKE_CONSUMPTION.md).

For a shorter getting-started path, use [QUICKSTART.md](QUICKSTART.md). For canonical examples, use [COOKBOOK.md](COOKBOOK.md).
