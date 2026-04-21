## Context

The current runtime is descriptor-driven and increasingly coherent, but it is still mostly a
blocking peripheral runtime.

To be a top-tier embedded library, it needs a stronger coordination model:

- time
- completions
- interrupt ownership
- low-power wakeup
- optional async integration

This must be done without breaking the current runtime strengths:

- small public API
- zero-overhead hot paths
- no mandatory kernel
- descriptor-driven behavior

## Goals

- executor-agnostic event model
- runtime time primitives usable across boards
- typed interrupt/completion ownership
- low-power integration hooks
- optional async adaptation without penalizing blocking-only builds

## Non-Goals

- mandating coroutines for normal users
- introducing a required RTOS
- replacing the existing blocking APIs
- building a networking stack in the same change

## Decision 1: Time Is A Runtime Service

The runtime SHALL expose a monotonic time service with:

- `Instant`
- `Duration`
- deadlines/timeouts
- timer wait primitives

The backing implementation may use SysTick, RTC, or another published timing source, but the
public time model must be board- and family-agnostic.

## Decision 2: Events Are Separate From Drivers But Typed By Them

The runtime SHALL expose event/completion primitives that drivers can use for:

- IRQ completion
- DMA completion
- timer expiry
- peripheral-ready conditions

Drivers keep ownership of peripheral semantics; the event layer provides coordination.

## Decision 3: Async Is Optional And Executor-Agnostic

The async layer SHALL be optional.

Blocking-only builds must remain lean. Async-capable builds may add adapters that work with:

- polling
- callbacks
- coroutines/futures
- or future executors

without forcing a single runtime model today.

## Decision 4: Interrupt Ownership Must Stay Typed

Interrupt hookups SHALL continue to use typed runtime contracts:

- startup stubs
- interrupt ids
- generated bindings

The async/event layer SHALL reuse that model rather than recreating a dynamic registry.

## Decision 5: Low Power Is Part Of The Coordination Story

The runtime SHALL treat low-power entry and wakeup as part of the event/time model.

The public model must be able to express:

- what can wake the system
- whether a timer/event source remains valid in low power
- what board/runtime hooks are needed before and after sleep

## Validation

At minimum this change must prove:

- one foundational time source per representative family
- interrupt and DMA completion tests
- timeout behavior
- low-power/wakeup smoke paths where supported
- no overhead regression in blocking-only builds
