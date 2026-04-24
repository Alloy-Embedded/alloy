# runtime-async-model Specification

## Purpose
The runtime async model defines the portable time, event, completion, and low-power coordination
surface that sits on top of the public HAL.

Blocking code remains valid on its own, while interrupt-driven and DMA-driven paths can surface
typed completions and optional async adapters without forcing one executor model or penalizing the
blocking hot path.
## Requirements
### Requirement: Runtime Shall Expose A Monotonic Time Service

The runtime SHALL expose a monotonic time service with portable time primitives.

#### Scenario: User waits until a deadline
- **WHEN** application code requests a deadline or timeout on a supported board
- **THEN** it uses one runtime time model
- **AND** the implementation resolves through the selected timing source without board-specific public APIs

### Requirement: Runtime Shall Expose Typed Event And Completion Primitives

The runtime SHALL expose typed event/completion primitives for interrupt-driven and DMA-driven
operations.

#### Scenario: DMA completion wakes a waiting operation
- **WHEN** a DMA-backed peripheral operation completes
- **THEN** the runtime can signal completion through a typed event primitive
- **AND** the user does not need a handwritten family-local ISR shim as the normal API path

### Requirement: Async Adaptation Shall Be Optional And Executor-Agnostic

The runtime MAY expose async adapters, but they SHALL remain optional and SHALL NOT require one
specific executor model.

#### Scenario: Blocking-only build
- **WHEN** an application uses only blocking HAL calls
- **THEN** it does not need to link or enable the async layer
- **AND** the blocking path remains valid on its own

### Requirement: Low Power Coordination Shall Integrate With Time And Events

The runtime SHALL describe how time and wakeup-capable events interact with low-power modes.

#### Scenario: Timer wakeup from low power
- **WHEN** a board enters a supported low-power mode with a valid timer wake source
- **THEN** the runtime documents and enforces the wakeup path through the same time/event model

### Requirement: The Async Model Shall Prove One Canonical Completion Path

The runtime async model SHALL prove at least one canonical completion-driven driver path in the
official examples and validation ladder.

The canonical path is the documented answer to "how do I wait for a transfer to finish with a
bounded deadline" for users reading the runtime docs.

#### Scenario: User follows the recommended async path

- **WHEN** a user follows the official async/event documentation
- **THEN** they can use one validated completion-driven driver path with timeout support
- **AND** they do not need handwritten interrupt glue as the normal story
- **AND** the path is linked from `docs/RUNTIME_ASYNC_MODEL.md`

### Requirement: Completion Paths Shall Surface Timeout As A First-Class Outcome

Typed completion primitives SHALL expose a timeout outcome that is distinguishable from success
and from other failure modes.

#### Scenario: Timeout elapses before completion

- **WHEN** a completion token is waited on with a finite `Duration` or `Deadline`
- **AND** the underlying operation does not signal before the deadline
- **THEN** the wait result reports a timeout outcome
- **AND** the outcome is type-level distinguishable from a completed transfer
- **AND** the user can retry, escalate, or cancel without undefined behavior

### Requirement: Blocking Path Shall Remain Valid Standalone

Growth of the async/event/completion layer SHALL NOT remove the ability to use HAL operations in
a purely blocking style without linking the async adapter layer.

#### Scenario: Blocking-only application

- **WHEN** an application consumes only blocking HAL calls and does not include `src/async.hpp`
- **THEN** it still builds, links, and runs on foundational boards
- **AND** no async executor symbols appear in the final image

### Requirement: Low Power Coordination Shall Be Observable In Validation

Low-power coordination with time and wake-capable events SHALL be backed by observable validation.

Observable proof means entry, wait, and wake can be asserted through host-MMIO, emulation, or
hardware spot-check evidence — not just documentation.

#### Scenario: Supported wake-capable wait path

- **WHEN** the repo claims a supported low-power + wake interaction
- **THEN** that interaction is backed by host, emulation, or hardware evidence
- **AND** not only by API presence
- **AND** the evidence is referenced from the runtime async docs

### Requirement: Docs Shall Show A Real Async Adapter Example

`docs/RUNTIME_ASYNC_MODEL.md` SHALL include at least one worked example of the optional async
adapter wrapping the same canonical driver path used for the blocking+completion flow.

#### Scenario: User wants to integrate with a scheduler

- **WHEN** a user reads `docs/RUNTIME_ASYNC_MODEL.md`
- **THEN** they find a worked example showing the async adapter polling the same underlying
  operation the blocking path uses
- **AND** the example is consistent with the canonical completion+timeout path

