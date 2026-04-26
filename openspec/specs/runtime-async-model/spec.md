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

### Requirement: Async Primitives SHALL Be Sound Across Cores When ALLOY_SINGLE_CORE Is Off

Alloy MUST provide a compile-time `ALLOY_SINGLE_CORE` switch that selects the
fence strength used by the runtime async primitives (`Event`, `Channel`,
`CancellationToken`). When `ALLOY_SINGLE_CORE = 1`, primitives MUST use
`std::atomic_signal_fence` so single-core builds incur zero memory-barrier
cost. When `ALLOY_SINGLE_CORE = 0`, the same primitives MUST upgrade to
`std::atomic_thread_fence` (or equivalent acquire/release atomic operations)
so cross-core observers cannot read stale data.

#### Scenario: Single-core targets default to signal fences

- **WHEN** the runtime is compiled with `ALLOY_SINGLE_CORE = 1` (the default
  on STM32, SAME70, nRF52, AVR-DA, and any other single-core platform cmake)
- **THEN** `Event`, `Channel`, and `CancellationToken` use
  `std::atomic_signal_fence` for ISR↔task ordering
- **AND** no `std::atomic_thread_fence` or full DMB instruction is emitted
  in the hot path
- **AND** the zero-overhead release gate continues to pass

#### Scenario: Dual-core targets enable thread fences

- **WHEN** the runtime is compiled with `ALLOY_SINGLE_CORE = 0` (set by the
  ESP32 and RP2040 platform cmake files)
- **THEN** `Event`, `Channel`, and `CancellationToken` upgrade their
  cross-core paths to `std::atomic_thread_fence` (or release/acquire
  `std::atomic` operations)
- **AND** a host-level two-thread test (TSan-clean) demonstrates zero data
  races across producer/consumer threads

### Requirement: Runtime SHALL Provide A CrossCoreChannel Primitive

The runtime MUST provide `alloy::tasks::CrossCoreChannel<T, N>` — a
single-producer single-consumer ring buffer using
`std::atomic<std::size_t>` head/tail with explicit acquire/release ordering
on every access. Head and tail MUST be cache-line padded (`alignas(64)`) to
prevent false sharing across cores. The API MUST mirror the existing
`Channel<T, N>` (`try_push`, `try_pop`, `size`, `empty`, `drops`).

#### Scenario: CrossCoreChannel transports values between threads without loss

- **WHEN** a host test pushes 1,000,000 monotonic integers from one
  `std::thread` and pops them from another
- **THEN** every value is delivered exactly once
- **AND** the test runs clean under ThreadSanitizer
- **AND** the producer and consumer never share a cache line for head/tail

#### Scenario: CrossCoreChannel is the recommended primitive for inter-core data

- **WHEN** documentation or examples describe a producer task on one core
  feeding a consumer task on another core
- **THEN** the recommended primitive is `CrossCoreChannel`, not the legacy
  `Channel<T, N>`
- **AND** `Channel<T, N>` is documented as ISR↔task on the same core
  (cross-core use is best-effort under `ALLOY_SINGLE_CORE = 0`)

### Requirement: Runtime SHALL Provide A SharedEvent For Cross-Core Signalling

The runtime MUST provide `alloy::SharedEvent` — equivalent to the existing
`Event`, but built on `std::atomic<bool>` with `memory_order_release` /
`memory_order_acquire` so a writer on one core synchronises-with a reader
on another core. The legacy `Event` remains for same-core ISR↔task and
MUST NOT be silently upgraded.

#### Scenario: Cross-core signalling synchronises observable state

- **WHEN** core 0 publishes a payload then sets a `SharedEvent`, and core 1
  observes the event set
- **THEN** core 1's subsequent read of the payload sees core 0's write

#### Scenario: Same-core users keep zero-overhead Event

- **WHEN** an ISR signals a task on the same core via `Event`
- **THEN** the existing `signal_fence`-only path is preserved
- **AND** no DMB / `atomic_thread_fence` is emitted on the hot path under
  `ALLOY_SINGLE_CORE = 1`

### Requirement: Scheduler SHALL Support Core Affinity

The cooperative `Scheduler` MUST accept an explicit `CoreAffinity` parameter
on `spawn`. A `SharedScheduler<MaxTasks, MaxFrameBytes>` MUST host two
per-core `Scheduler` instances, route `Core0` / `Core1`-pinned tasks
directly to the matching scheduler, and route `Any`-affinity tasks through
a shared queue protected by a TAS spinlock (byte-sized, cache-line padded).
`tick(core_id)` MUST drive only the scheduler for the given core.

#### Scenario: Pinned affinity is honoured

- **WHEN** a task is spawned with `CoreAffinity::Core0`
- **THEN** it only runs when `tick(0)` is called
- **AND** never runs from `tick(1)`
- **AND** the symmetric assertion holds for `CoreAffinity::Core1`

#### Scenario: Any-affinity tasks round-robin between cores

- **WHEN** multiple `CoreAffinity::Any` tasks are spawned
- **THEN** ticks from both cores pull from the shared queue under the TAS
  spinlock
- **AND** no task is observed to run on both cores simultaneously
- **AND** the spinlock is byte-sized and resides in a cache-line-padded
  region to avoid false sharing with `head_` / `tail_` on neighbouring
  primitives

