# Alloy v0.1.0

First tagged release. Establishes the public runtime story.

## Scope

`alloy` v0.1.0 ships a C++23 bare-metal HAL on top of a descriptor-driven device boundary,
with one canonical runtime async model, a stable board-oriented tooling surface, a published
docs site, and hardware-validated bring-up on three foundational boards.

## What You Get

- **Public HAL surface.** Uniform `Config` + `handle<PeripheralId>` + `configure()` +
  `open<P>(...)` pattern across GPIO, UART, I2C, SPI, DMA, Timer, PWM, ADC, DAC, RTC, CAN,
  and watchdog. Board helper layer (`board::init()`, `board::make_*()`) on
  `same70_xplained`, `nucleo_g071rb`, and `nucleo_f401re`.
- **Runtime async model.** `src/time.hpp`, `src/event.hpp`, and optional `src/async.hpp`.
  `runtime::event::completion::wait_for` returns `Err(Timeout)` as a first-class recoverable
  outcome. Canonical example at `examples/async_uart_timeout/`.
- **Hardware-validated foundational boards.** Full lab telemetry recorded in
  `tests/hardware/*/CHECKLIST.md`.
- **Tooling parity.** `alloyctl compile-commands`, `alloyctl info`, `alloyctl doctor`,
  `alloyctl new`, on top of the existing configure/build/flash/monitor/sweep/explain/diff
  surface.
- **Docs site.** MkDocs Material at the GitHub Pages URL, rebuilt on every push to `main`.

## Compatibility

- `alloy-devices` ref pinned to `fa0630fe1501e7435463c1a2f2df4e9858e507b4`. Downstream projects
  that vendor their own `alloy-devices` must use the same commit (or a descendant that has not
  changed the descriptor schema or the board profiles above).
- Minimum toolchain: CMake 3.25, `arm-none-eabi-gcc` bare-metal toolchain, OpenOCD, Python 3.11
  with `pyserial`. Run `python3 scripts/alloyctl.py doctor` to verify.

## Support Tiers

| Scope | Tier | Evidence |
| --- | --- | --- |
| `same70_xplained` | foundational | host-MMIO + renode + zero-overhead + hardware spot-check |
| `nucleo_g071rb` | foundational | host-MMIO + renode |
| `nucleo_f401re` | foundational | host-MMIO + renode |
| `nucleo_g0b1re` | experimental | build-covered only |
| GPIO / UART / DMA / time / interrupt-event / clock-reset-startup / runtime-device-boundary | foundational | see `docs/RELEASE_MANIFEST.json` |
| I2C / SPI / Timer / PWM / RTC / Watchdog / ADC / DAC / low-power | representative | descriptor-contract smoke + hardware spot-check |
| CAN | experimental | boot/loop banner only — see known gaps below |

## Release Gates

All of the following must pass on the release cut:

- `runtime-device-boundary`, `release-discipline`, `descriptor-contract-smoke`, `host-mmio`,
  `renode-runtime-validation`, `same70-zero-overhead`, `foundational-example-coverage`
- `blocking-only-path`, `alloyctl-tooling-parity`, `docs-site`, `changelog-present`

## Known Gaps

- **CAN:** only boot/loop banner validated on SAME70. A deterministic traffic assertion is
  still pending and blocks promotion to `representative`.
- **Alternate SAME70 UART routes** (`usart0_pb01`, `uart1_pa56`): build-only on the current
  lab wiring. Needs a second USB-UART adapter for honest silicon evidence.
- **STM32F4 DMA hardware rerun:** build is in the `nucleo_f401re` bundle, but the lab rerun
  with a probe attached is still open. Tracked in the foundational hardware validation
  checklist.

## Upgrade Notes

This is the first tagged release. There is no prior `v*` tag to migrate from. Downstream
projects that were consuming `alloy` directly from `main` SHOULD pin to `v0.1.0` and read
`CHANGELOG.md` on every subsequent bump.

## Credits

Primary development by [@lgili](https://github.com/lgili). Automation and documentation
authoring assisted by Claude (Anthropic). The descriptor-driven device boundary is backed by
the external `alloy-devices` project.
