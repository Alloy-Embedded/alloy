## ADDED Requirements

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
