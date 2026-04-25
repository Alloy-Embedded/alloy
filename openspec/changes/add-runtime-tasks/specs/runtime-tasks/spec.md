# runtime-tasks Spec Delta: Initial Capability

## ADDED Requirements

### Requirement: The Capability Shall Provide A Coroutine Task Type

The capability SHALL provide a `Task<T>` C++20 coroutine type that represents one
unit of cooperatively scheduled application work. The type SHALL support `co_await`
on the awaiters defined by this capability and SHALL propagate a typed return value
to its parent on completion.

#### Scenario: Task that returns a value
- **WHEN** application code declares `Task<int> compute()` and `co_return 42` from
  it
- **THEN** the parent task receives `42` through the awaiter contract
- **AND** the coroutine's frame is freed back to the scheduler's pool on completion

#### Scenario: Task that propagates cancellation
- **WHEN** a task awaits `delay(100ms)` and its cancellation token fires before
  the deadline
- **THEN** the awaiter returns `Result<void, Cancelled>` with `is_err()` true
- **AND** the task drops back to its caller, unwinding RAII normally

### Requirement: The Capability Shall Provide A Single-Threaded Cooperative Scheduler

The capability SHALL provide a `scheduler<MaxTasks, MaxFrameBytes>` type that runs
spawned tasks on a single CPU thread, switching between them only at user-visible
`co_await` points. The scheduler SHALL NOT preempt tasks.

#### Scenario: Two tasks share the CPU at await points
- **WHEN** two tasks alternate between work and `co_await yield_now()`
- **THEN** they both make progress without preempting each other
- **AND** application code observes no race conditions between them on shared
  state, because no context switch happens between `co_await` points

#### Scenario: Spawn returns the typed task identifier
- **WHEN** application code calls `scheduler.spawn(my_task(), priority::normal)`
- **THEN** the call returns `Result<task_id, SpawnError>` with `is_ok()` true and
  the identifier referencing the running task
- **AND** the application can pass that identifier to subsequent calls (cancel,
  query priority, query state) without searching

### Requirement: The Capability Shall Allocate Coroutine Frames From A Fixed Pool

Coroutine frames SHALL be allocated from a fixed-capacity pool owned by the
scheduler instance. The capability SHALL NOT allocate from the heap in steady
state. The pool SHALL be sized at compile time through the
`scheduler<MaxTasks, MaxFrameBytes>` template parameters.

#### Scenario: Pool exhaustion is reported, not silently dropped
- **WHEN** the user spawns more tasks than the pool can hold
- **THEN** the spawn call returns `Result<task_id, SpawnError::PoolFull>` and the
  pool state is unchanged
- **AND** no allocation through `new`/`malloc` is performed

#### Scenario: Frame too large is reported, not silently dropped
- **WHEN** the user spawns a task whose coroutine frame size exceeds the
  configured `MaxFrameBytes` slot
- **THEN** the spawn call returns `Result<task_id, SpawnError::FrameTooLarge>`
  and the pool state is unchanged
- **AND** the error message includes the actual frame size and the configured
  slot size so the user can adjust the template parameter

### Requirement: The Capability Shall Support Static Priority Scheduling

The capability SHALL support four static priority levels (`high`, `normal`,
`low`, `idle`). Priority SHALL be assigned at spawn time and SHALL NOT change
afterward. The scheduler SHALL always resume the highest-priority ready task at
each scheduling decision; ties within a level are broken FIFO.

#### Scenario: High-priority task wins contention
- **WHEN** a `priority::high` task and a `priority::low` task both become
  ready at the same scheduling point
- **THEN** the scheduler resumes the high-priority task to completion or to
  its next `co_await`
- **AND** the low-priority task observes that the high-priority work has
  completed before its own next resume

#### Scenario: FIFO ordering within a priority level
- **WHEN** three `priority::normal` tasks all become ready in spawn order A,
  B, C
- **THEN** the scheduler resumes them in spawn order A, B, C, A, B, C until
  one yields or completes

### Requirement: The Capability Shall Provide The Common Awaiter Set

The capability SHALL provide awaiters covering: a duration-based delay, an
explicit yield, an event subscription that integrates with `runtime-async-model`,
a predicate that the scheduler polls, and combinators for waiting on the first
or all of a set of awaitables.

#### Scenario: Delay suspends until the deadline
- **WHEN** a task `co_await alloy::tasks::delay(100ms)`
- **THEN** the task is removed from the ready queue
- **AND** the scheduler resumes it no earlier than 100 ms after the call,
  measured against the runtime's monotonic time source

#### Scenario: yield_now requeues at the end of the same priority level
- **WHEN** a task calls `co_await alloy::tasks::yield_now()`
- **THEN** the task is added to the back of its priority level's FIFO
- **AND** the scheduler resumes other ready tasks at that priority before
  returning to it

#### Scenario: any_of returns the first awaitable to complete
- **WHEN** a task `co_await any_of(delay(50ms), on(button_event))`
- **THEN** the call returns when either the delay elapses or the button
  event fires, whichever happens first
- **AND** the unfinished branch is cancelled and its frame released

### Requirement: The Capability Shall Be Cancellable Via Typed Tokens

The capability SHALL provide a `cancellation_token` type that the user passes
into `spawn`. Awaiters SHALL check the token before suspending and at each
resume; a cancelled awaiter SHALL return `Result<T, Cancelled>` rather than
the awaited value. Cancellation SHALL NOT be expressed as a C++ exception.

#### Scenario: Cancellation propagates to a child task
- **WHEN** a parent task's cancellation token is fired
- **THEN** the parent's next awaiter returns `Cancelled`
- **AND** any child task spawned with a token chained to the parent's also
  observes cancellation on its next awaiter

### Requirement: The Capability Shall Be Single-Threaded Per Instance

The scheduler SHALL run on exactly one CPU thread per instance. The capability
SHALL NOT provide cross-instance task migration, mutexes, or shared ready
queues. Multi-core users SHALL instantiate one scheduler per core and SHALL
coordinate between them through `runtime-async-model` events posted across
cores by ISRs or platform glue.

#### Scenario: Two scheduler instances run independently
- **WHEN** an ESP32-S3 application instantiates `scheduler` on each core
- **THEN** tasks spawned on one scheduler do not migrate to the other
- **AND** there is no shared ready queue or cross-core mutex inside the
  scheduler

### Requirement: The Capability Shall Stay Inside The Footprint Budget

The capability SHALL fit within 4 KB of code (`.text + .rodata`) and 4 KB of RAM
(`.data + .bss + pool`) for a representative configuration of one
`scheduler<8, 256>` plus two tasks using `delay`, `yield_now`, and `on`, on a
Cortex-M0+ target compiled with `-Os -fno-exceptions -fno-rtti`. CI SHALL
measure footprint against that configuration on every change.

#### Scenario: Footprint gate fails on regression
- **WHEN** a change pushes the representative configuration's footprint past
  the budget
- **THEN** the size-tracking CI job fails the build
- **AND** the failure message names which segment grew and by how much

### Requirement: The Capability Shall Be Opt-In

The capability SHALL NOT affect builds that do not include its headers. Code
that targets the existing super-loop or descriptor-driven `runtime-async-model`
without the scheduler SHALL link no scheduler symbols and pay no scheduler
RAM cost.

#### Scenario: Super-loop application links nothing from runtime-tasks
- **WHEN** an application that does not include `<alloy/tasks/scheduler.hpp>`
  is built and linked
- **THEN** the resulting image contains no symbols from `alloy::tasks`
- **AND** static analysis confirms no scheduler RAM is reserved

### Requirement: The Capability Shall Refuse To Build On Unsupported Architectures

The build system SHALL refuse to enable the capability with a clear `FATAL_ERROR`
when the active target architecture lacks a working C++20 coroutine implementation
in the alloy-pinned toolchain. As of v1 this applies to `arch=avr`.

#### Scenario: AVR build refuses cleanly
- **WHEN** an application targeting `arch=avr` includes the runtime-tasks
  CMake target
- **THEN** the configure step fails with an error message naming the
  architecture and pointing at the alternative (super-loop or reactor-style
  Task, when added)
- **AND** the project does not enter compilation only to fail on coroutine
  intrinsics
