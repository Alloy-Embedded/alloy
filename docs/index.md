# Alloy

**C++23 bare-metal HAL and runtime with a descriptor-driven device boundary.**

Alloy ships one public HAL surface across ARM microcontroller families. Every peripheral
configure, every route choice, every DMA binding, and every completion token is backed by a
generated device descriptor — so the runtime never guesses and the boundary between portable
code and vendor silicon is explicit and testable.

## Start Here

- [Quickstart](QUICKSTART.md) — `pipx install alloy-cli`, scaffold a project, build and flash
- [CLI Reference](CLI.md) — every `alloy` subcommand and flag
- [Custom Boards](CUSTOM_BOARDS.md) — the `ALLOY_BOARD=custom` runtime contract every scaffolded project rides on
- [Board Tooling](BOARD_TOOLING.md) — probe and flashing notes per board
- [Cookbook](COOKBOOK.md) — canonical examples for the active runtime path
- [Support Matrix](SUPPORT_MATRIX.md) — what is foundational, representative, or experimental today

## Why Alloy

- **Descriptor-driven boundary.** The device contract is a generated artifact, not hand-written
  vendor glue. The runtime device boundary is enforced by a release gate.
- **Typed completion with first-class timeout.** `runtime::event::completion::wait_for` returns
  `Err(Timeout)` as a recoverable outcome — no undefined behavior on deadline expiry. See the
  [Runtime Async Model](RUNTIME_ASYNC_MODEL.md).
- **Zero-overhead blocking path.** Blocking+completion code never pays for the optional async
  adapter layer. The invariant is enforced by a `blocking-only-path` release gate.
- **Hardware-validated foundational boards.** SAME70, STM32G0, and STM32F4 are exercised on
  silicon and recorded in per-board checklists.

## Foundational Boards

| Board | Tier | Runtime validation |
| --- | --- | --- |
| `same70_xplained` | foundational | [host MMIO + renode + zero-overhead + hardware spot-check](SUPPORT_MATRIX.md) |
| `nucleo_g071rb` | foundational | host MMIO + renode |
| `nucleo_f401re` | foundational | host MMIO + renode |

## Deeper Dives

- [Runtime Async Model](RUNTIME_ASYNC_MODEL.md)
- [Runtime Device Boundary](RUNTIME_DEVICE_BOUNDARY.md)
- [CMake Consumption](CMAKE_CONSUMPTION.md)
- [Architecture](ARCHITECTURE.md)

## Contribute

Source lives at [github.com/lgili/alloy](https://github.com/lgili/alloy). Release discipline and
the full release checklist are under [Internals](RELEASE_DISCIPLINE.md).
