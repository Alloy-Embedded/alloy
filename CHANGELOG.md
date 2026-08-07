# Changelog

## 0.3.0 — 2026-08-07

Everything an editor needs to describe a board without reimplementing any of
it. Each verb below has a versioned `--json` envelope, and a readable default
so the same command is useful in a terminal.

**Requires `alloy-devices` >= 0.3.0.** Not a formality: the interrupt-driven
I2C driver reads `CR1.STOPIE`, which 0.2.0 does not carry — against it the
generated IP header silently lacks the accessor and the failure lands as a
template error inside framework headers you do not own.

### New: describe and check a board

- **`alloy board-info [<id>] --json`** — roles, capabilities, used pins and
  problems of **any** board, curated or project-local. Capabilities come from
  the same `role_caps()` the emitter uses, so this cannot disagree with the
  `board::caps` the generated header will carry.
- **`alloy board-validate [<id>] [--file f|-] --json`** — every problem at
  once, located (role, field, pin) and answered with the values that would
  work. The headline rule moves a `static_assert` forward: `alloy::i2c::bind`
  already refuses a pin with no route to its peripheral, but only when the app
  instantiates the bus, and only at build time. This asks at config time.
  Exits non-zero, so it works as a CI gate.
- **`alloy board-clone <src> <new>`** — copy a curated board into your project
  as an editable one.
- **A `board.json` value is a default; `alloy.toml` chooses.** The fields a
  project may pick — `debug_uart.baud`, `watchdog.timeout_ms`, `nvm`/`fs`
  `bytes`, and `[clock]` — can be set per project, so a framework board keeps
  receiving upstream fixes instead of being forked to change a number. Which
  fields those are is declared once, in `roles.py`; anything else is refused
  with the reason and the command that would actually do it, and an override
  for a role the board does not define is reported as inert rather than
  silently dropped. `board-info` reports the effective board *and* a
  `project_overrides` block with the board's own value beside each change.

### New: see what you built

- **`alloy size --json`** — flash and RAM of the last build against the chip's
  real memories, and, on a board with an A/B layout, whether the packed image
  fits each slot. Measured against the regions the linker actually used.
- **`alloy build --json`** — the build result with that size summary attached.
- **`alloy matrix [--boards a,b] --json`** — build this project for every
  board and table the result. One source tree, nine boards, two architectures.
  A board that fails is a row with a reason, not an aborted sweep.
- **`alloy svd [--chip <id>] [-o out]`** — write a CMSIS-SVD file so a debugger
  shows peripheral registers by name. Generated from register maps that were
  already curated; no new data. Refuses a non-ARM chip rather than emitting a
  file no debugger can use.

### New: the whole clock, and CI

- **`alloy clock --graph [--profile p] --json`** — sources, bus prescalers, and
  the kernel clock each peripheral is fed, with what it implies: a UART's baud
  error computed with the driver's own rounding, a timer's reachable range.
- **`alloy ci-init [--boards a,b] [--all] [--force]`** — write a GitHub Actions
  workflow for your project. Targets the boards it actually targets, and
  installs only the toolchains those need.
- **`alloy monitor --json`** — the serial link as NDJSON, for a caller with no
  terminal. End of stdin means "nothing more to send", not "close the link".

### Extended

- `alloy chip-info` gains a **role catalogue** (per role: required and optional
  fields, candidate peripherals by IP class with every pin each signal can
  reach, whether each is curated) and the chip's `package`, when its data
  carries one. Still `alloy.chip_info.v1` — every field is additive.

### Fixed

- `alloy size` measured against the first memory of each kind. Correct on
  Cortex-M, wrong on Xtensa, where `.text` goes to IROM and `.data`/`.bss` to
  DRAM while "the first ram" is a 2 KiB vector window — an ESP32 build reported
  0.3K of 2.0K RAM instead of 0.3K of 272K. Regions now come from the linker
  emitter, and every figure names the memory it refers to.

## 0.2.0 — 2026-07-24

`alloy chips` / `new --chip` for any MCU in the database, the parametric clock
solver, `alloy lib`, and the OTA verbs (`keygen`, `image`, `ports`, `update`).

## 0.1.0

First release: `new`, `build`, `flash`, `monitor`, `run` with `--board`
switching, and the data → codegen → HAL loop behind them.
