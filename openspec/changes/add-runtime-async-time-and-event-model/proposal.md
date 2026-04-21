## Why

`alloy` can become better than traditional C embedded libraries, but it still lacks a strong story
for time, events, interrupts, and async-style coordination.

This is the clearest place where modern frameworks such as `Embassy` still feel better to use:

- global time primitives
- timer/timeouts that "just work"
- interrupt and DMA completion as first-class events
- executor-agnostic async/blocking coexistence
- low-power coordination with wake sources

Without this, `alloy` remains strong as a typed blocking runtime, but not yet excellent as a
modern embedded application runtime.

## What Changes

- add a first-class runtime time model with monotonic time, deadlines, and timers
- add runtime event/completion primitives for interrupts, DMA, and peripheral operations
- add executor-agnostic async adapters on top of the public HAL
- integrate interrupt ownership with generated interrupt contracts and startup stubs
- define low-power and wakeup coordination as part of the runtime model

## Outcome

After this change, `alloy` should support:

- blocking use
- event-driven use
- async adaptation

without forking the public API or forcing a mandatory RTOS.

## Impact

- Affected specs:
  - `runtime-async-model` (new)
  - `public-hal-api`
  - `startup-runtime`
  - `zero-overhead-runtime`
- Affected code:
  - `src/hal/**`
  - `src/device/**`
  - `src/arch/**`
  - `boards/**`
  - `examples/**`
  - `tests/**`
