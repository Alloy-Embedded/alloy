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
