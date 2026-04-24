# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project
aims to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html) once the public
API stabilises past `0.x`.

## [Unreleased]

## [0.1.0] — 2026-04-24

The first tagged `alloy` release. Establishes the C++23 bare-metal HAL surface, the
descriptor-driven device boundary, the runtime async model, foundational-board hardware
validation, tooling parity, and the public docs site.

### Added

#### Public HAL surface

- C++23 public HAL with a uniform `Config` + `handle<PeripheralId>` + `configure()` +
  `open<P>(...)` pattern across GPIO, UART, I2C, SPI, DMA, Timer, PWM, ADC, DAC, RTC, CAN,
  and watchdog.
- Board helper layer (`board::init()`, `board::make_*()`) on three foundational boards:
  `same70_xplained`, `nucleo_g071rb`, `nucleo_f401re`.
- Descriptor-driven device boundary: peripheral topology, clock trees, DMA channels, and
  interrupt routing come from the external `alloy-devices` artifact, pinned at commit
  `fa0630fe1501e7435463c1a2f2df4e9858e507b4` for this release.

#### Runtime async model

- `src/time.hpp` — portable `Duration`, `Instant`, `Deadline`, `time::source<TickSource>`.
- `src/event.hpp` — typed `runtime::event::completion<Tag>` with `wait_for` returning
  `core::Result<void, core::ErrorCode>`; `Timeout` is a first-class recoverable outcome.
- `src/async.hpp` — optional scheduler-friendly adapter that wraps the same HAL operations as
  the blocking+completion path.
- Canonical example `examples/async_uart_timeout/` demonstrating the three observable outcomes:
  success, timeout, recovery.
- Host unit coverage `tests/unit/test_async_completion_timeout.cpp` proving the reusable-token
  timeout semantics on a deterministic mock time source.
- `scripts/check_blocking_only_path.py` guard (+ `blocking-only-path` release gate) enforcing
  that blocking+completion examples never depend on `src/async.hpp`.

#### Hardware validation

- SAME70: all foundational + representative examples flashed and validated on silicon; full
  telemetry captured in `tests/hardware/same70/CHECKLIST.md`.
- STM32G0 (`nucleo_g071rb`) and STM32F4 (`nucleo_f401re`) bring-up runs captured in their
  respective `tests/hardware/*/CHECKLIST.md` files.
- Renode runtime validation covering `same70-runtime-validation`,
  `stm32g0-runtime-validation`, and `stm32f4-runtime-validation` presets.
- `same70-zero-overhead` gate asserting the blocking path pays no runtime cost for the async
  adapter layer.

#### Tooling

- `alloyctl configure | build | bundle | flash | recover | monitor | gdbserver | validate |
  sweep | explain | diff` as the stable board-oriented UX.
- `alloyctl compile-commands` — symlink or copy `compile_commands.json` at the repo root for
  clangd/LSP.
- `alloyctl info` — machine-readable JSON environment report (version, pinned `alloy-devices`
  ref alignment, board tiers, required gates, tool versions, repo git sha).
- `alloyctl doctor` — preflight check for cmake, arm-none-eabi-gcc, openocd, pyserial, and
  device-contract ref alignment.
- `alloyctl new` — scaffold a downstream firmware starter for a foundational board.
- `scripts/check_alloyctl_tooling_parity.py` covering the four new subcommands
  (`alloyctl-tooling-parity` release gate).

#### Docs site

- MkDocs Material configuration at `mkdocs.yml` with User Guide / Reference / Internals
  navigation split.
- `docs/index.md` landing page and `docs/DOCS_SITE.md` build-and-publish guide.
- `.github/workflows/docs.yml` builds and deploys to GitHub Pages on push to `main`.
- `scripts/check_docs_site.py` runs `mkdocs build --strict` as the `docs-site` release gate.

#### Release discipline

- `docs/RELEASE_MANIFEST.json` enumerating release gates, board tiers, required examples per
  foundational board, and the `alloy-devices` compatibility pin.
- `scripts/check_changelog_present.py` enforcing that the version declared in `CMakeLists.txt`
  has a matching changelog section (`changelog-present` release gate).
- `docs/RELEASE_NOTES_v0.1.0.md` as the canonical release-body text for this cut.

### Known gaps

- CAN peripheral class remains `experimental`: boot/loop banner validated on SAME70, but a
  deterministic traffic assertion is still pending and blocks promotion.
- Alternate UART route probes on SAME70 (`usart0_pb01`, `uart1_pa56`) are build-only on the
  current lab wiring; they need a second USB-UART adapter to produce honest on-silicon
  evidence.
- STM32F4 DMA validation on the `nucleo_f401re` bundle is tracked in the foundational hardware
  validation change; the build is in the bundle but the lab rerun with a probe is still open.

[Unreleased]: https://github.com/lgili/alloy/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/lgili/alloy/releases/tag/v0.1.0
