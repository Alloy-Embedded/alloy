# Changelog

Versions are `MAJOR.MINOR.PATCH`. What each number is allowed to break, which
headers and commands are covered, and how much notice a removal gets are
written down in [docs/reference/stability.md](docs/reference/stability.md).
Read that before planning a product around a version — in particular the part
about pinning `alloy-devices`, which carries the register facts and lives in
its own repo on its own tags.

Entries marked **Deprecated** keep working for at least one MINOR release and
are removed no earlier than the next MAJOR; each one names its replacement.

## Unreleased

### New

- **`alloy devices`** — report the chip database this project resolves: root,
  declared version, and a content digest over every schema/register/chip file.
  `--pin` writes both into `alloy.toml` under `[devices]`, and from then on any
  verb that loads the project refuses a database that does not match. A
  `[devices] path` also redirects discovery, ahead of `ALLOY_DEVICES_ROOT`.
  This closes the hole where a shipped product rebuilt against a moved sibling
  checkout silently compiled different register offsets.
- **`docs/reference/stability.md`** — what is public API, what may break in
  which release, the deprecation window, and how to pin both repos.
- **`docs/reference/safety.md`** — the properties that hold by construction
  (no heap, no exceptions/RTTI, no recursion, bounded stack), each with the
  command that checks it, plus the ones that do **not** hold today.
- **`docs/guide/escape-hatch.md`** + **`examples/escape_hatch/`** — calling a
  vendor HAL, poking a register through `alloy::dev::`, and what does and does
  not collide with CMSIS (nothing at include time; `Reset_Handler` and
  `SysTick_Handler` at link time; peripheral IRQ handlers silently).
- **`scripts/check_static_limits.sh`** + **`scripts/static_limits.py`** —
  recursion, heap, exception/RTTI machinery and a worst-case stack bound read
  out of a linked ELF, each with a negative control. Firmware is now compiled
  with `-fstack-usage`, which changes no code bytes.
- A `static-limits` CI job runs all of the above, plus the two escape-hatch
  link experiments, on every push.

### Fixed

- **`alloy sbom` no longer relabels an undeclared vendored tree as alloy's own.**
  A third-party source under `<alloy>/src/**/vendor/` that matched no entry in
  `sbom.py`'s `_VENDORED` table fell through to the framework bucket and was
  reported as part of "alloy, MIT" — silently, with its own licence file sitting
  beside it. Both packages alloy vendors today live under exactly that path, so
  the next one would have inherited the bug. Such a file is now an **undeclared
  component**, which is also what makes `--strict` refuse it.
- **`docs/guide/async.md` figures re-measured, and one claim withdrawn.** The
  guide said a task parked on an event *or a timer* costs nothing per superstep.
  Measured: eight event-parked tasks cost nothing (34.75 ieq empty poll, same as
  none), but eight parked on `delay()` cost 131.25 — `run_once()` walks the timer
  list every superstep, ≈12 ieq per sleeping task. `examples/concurrency_probe`
  now measures it, the emulation leg gates it as `verdict parked`, and the page
  says how far to read a figure (±1 ieq; a rebuild moves the last digit).
- **`alloy setup` now prints the `export PATH=…` its own install requires.**
  An offline install put the cross compiler in `~/.alloy/tools`, `alloy setup
  --check` called it `ok`, and `alloy build` then failed with "The CXX compiler
  identification is unknown" — because the build looks for the compiler on
  `PATH` only. On an air-gapped host those two messages have nothing connecting
  them. The line is now printed at the end of any install, and
  `docs/guide/supply-chain.md` says to run it.
- Two factual corrections on `docs/reference/stability.md`: `alloy image` was
  listed among the verbs a `[devices]` mismatch refuses, but it takes a built
  binary and never opens a project; and the `### Deprecated` changelog policy
  starts here, not retroactively at 0.1.0. The stale sample `.elf` digest on
  `docs/guide/supply-chain.md` was refreshed and labelled as one build's.

### Deprecated

- Nothing.

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
