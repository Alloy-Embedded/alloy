## Why

`alloyctl` already covers configure/build/flash/monitor/validate/sweep/explain/diff. Adoption still
lags what Zephyr `west`, ESP-IDF `idf.py`, and PlatformIO `pio` offer because four common
entry points are missing:

- no command to emit a `compile_commands.json` path for clangd/LSP
- no machine-readable environment report (board tiers, pinned `alloy-devices` ref, tool versions,
  release gate state)
- no preflight check that tells the user whether their toolchain, probe, and python deps match
  what the repo actually requires
- no scaffold that turns a foundational board into a minimal starter firmware tree

These are the low-effort pieces of "parity". Menuconfig and driver registry are deliberately
out of scope — they need the driver seed work first.

## What Changes

- Add `alloyctl compile-commands` — export or symlink `compile_commands.json` for a configured
  build dir so IDE LSPs find it.
- Add `alloyctl info` — machine-readable JSON report of: alloy version, pinned `alloy-devices` ref,
  board tier list, required gates per board, detected tool versions (cmake, arm-none-eabi-gcc,
  openocd, python), and current git sha.
- Add `alloyctl doctor` — preflight: verify cmake, arm-none-eabi toolchain, openocd, python deps
  are present and versioned, and that the configured `alloy-devices` submodule ref matches the
  `docs/RELEASE_MANIFEST.json` pin.
- Add `alloyctl new` — scaffold a downstream firmware starter for a chosen foundational board,
  mirroring the blink example surface and pointing at `docs/CMAKE_CONSUMPTION.md`.
- Extend `docs/BOARD_TOOLING.md` and `docs/QUICKSTART.md` with the four new entry points.

## Outcome

A user who clones the repo (or consumes it downstream) can: get IDE completion in one command,
print a BOM-quality environment report for bug reports and release audit, diagnose why a flash
attempt would fail before running it, and spin up a new firmware project without copying files
by hand.

## Impact

- Affected specs:
  - `runtime-tooling`
- Affected code and docs:
  - `scripts/alloyctl.py`
  - `docs/BOARD_TOOLING.md`
  - `docs/QUICKSTART.md`
  - `tests/` (new host test for info/doctor/compile-commands/new)
