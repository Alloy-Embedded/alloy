# Proposal: Add Alloy RTOS

## Summary
Implement a lightweight, compile-time configured Real-Time Operating System (RTOS) for Alloy embedded framework. The RTOS provides priority-based preemptive multitasking with safe inter-task communication, all resolved at compile time for zero runtime overhead and minimal memory footprint.

## Motivation
Currently, Alloy applications run as a single super-loop. This creates several problems:

1. **No multitasking** - Can't run multiple independent tasks concurrently
2. **Poor responsiveness** - High-priority events delayed by low-priority processing
3. **Complex state machines** - Manual scheduling logic becomes unmaintainable
4. **Resource contention** - No safe way to share peripherals between tasks
5. **Timing guarantees** - Hard to meet real-time deadlines in super-loop

A proper RTOS solves these problems with:
- **Preemptive scheduling** - High-priority tasks interrupt low-priority ones
- **Deterministic timing** - Guaranteed response times for critical tasks
- **Safe communication** - Type-safe message passing and synchronization
- **Resource protection** - Mutexes prevent race conditions
- **Clean architecture** - Each task is independent, testable module
- **Zero overhead** - All task scheduling resolved at compile time
- **Minimal memory** - No dynamic allocation, no fragmentation

## Goals

### Core RTOS Features
1. **Priority-based preemptive scheduler**
   - 8 priority levels (0 = lowest, 7 = highest)
   - Higher priority tasks always run first
   - Round-robin within same priority (future enhancement)
   - Tickless idle mode for power savings

2. **Compile-time task configuration**
   - All tasks defined at compile time (no dynamic creation)
   - Stack sizes validated at compile time
   - Zero runtime overhead for task management
   - Constexpr-based priority validation

3. **Safe inter-task communication**
   - **Message Queues**: Type-safe FIFO queues with zero-copy semantics
   - **Semaphores**: Binary and counting semaphores for synchronization
   - **Mutexes**: Priority inheritance to prevent priority inversion
   - **Event Flags**: Lightweight notification mechanism

4. **Memory safety**
   - No dynamic allocation (all static)
   - No heap fragmentation
   - Compile-time memory footprint calculation
   - Stack overflow detection (debug builds)

5. **Integration with SysTick**
   - Uses SysTick as heartbeat (depends on `add-systick-timer-hal`)
   - Configurable tick rate (default 1ms)
   - Accurate task delays and timeouts

### Advanced Features (Future, Out of Scope for Initial Release)
- Software timers (one-shot and periodic)
- Task notifications (lightweight alternative to queues)
- Memory pools for safe dynamic allocation
- CPU usage statistics
- Trace hooks for debugging
- Low-power tickless idle mode

## Non-Goals
1. **Not POSIX-compliant** - Custom API optimized for embedded C++20
2. **Not supporting dynamic task creation** - All tasks compile-time only
3. **Not supporting floating-point context save** - Users enable if needed
4. **Not supporting memory protection units (MPU)** - Future enhancement
5. **Not supporting multi-core** - Single-core only (ESP32 dual-core is future work)
6. **Not implementing networking stack** - Separate concern
7. **Not implementing file system** - Separate concern

## Success Criteria
- [x] Task scheduler with 8 priority levels
- [x] Preemptive context switching on all 5 platforms
- [x] Message queues with type safety and zero-copy
- [x] Binary and counting semaphores
- [x] Mutexes with priority inheritance
- [x] Event flags (32-bit mask)
- [x] All memory statically allocated
- [x] Compile-time configuration and validation
- [x] Context switch latency <10µs on ARM Cortex-M
- [x] Memory footprint <2KB RAM + (stack per task)
- [x] Documentation with examples for all IPC mechanisms
- [x] Example applications demonstrating RTOS features

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Context switch overhead | High - affects all task switches | Optimize assembly, use PendSV on ARM, benchmark all platforms |
| Priority inversion | High - breaks real-time guarantees | Implement priority inheritance for mutexes |
| Stack overflow | Critical - crashes system | Debug build with stack canaries, validate at compile time |
| Interrupt safety | Critical - race conditions | Disable interrupts for critical sections, atomic operations |
| Platform differences | Medium - inconsistent behavior | Abstract platform-specific code, extensive testing |
| Complexity | Medium - hard to debug | Comprehensive examples, trace hooks, good documentation |
| Memory usage | Medium - limited RAM | Static allocation, compile-time sizing, memory calculator |

## Dependencies
- **Requires**: `add-systick-timer-hal` (scheduler heartbeat)
- **Builds on**: Clock configuration (all platforms need accurate timing)
- **Blocks**: Advanced applications (data logging, motor control, UI, etc.)

## Timeline
- **Design & Spec**: 2 days (this proposal + detailed design)
- **Core Scheduler**: 3-4 days (context switch + scheduler for 5 platforms)
- **IPC Mechanisms**: 3-4 days (queues, semaphores, mutexes, events)
- **Testing & Validation**: 2-3 days (unit tests + real hardware)
- **Examples & Documentation**: 2 days
- **Total**: 12-15 days (2.5-3 weeks)

## Alternatives Considered

### 1. Use Existing RTOS (FreeRTOS, Zephyr, etc.)
**Rejected**:
- **Heavy dependencies**: FreeRTOS pulls in lots of code, complex config
- **Dynamic allocation**: Most RTOSes use heap, risk of fragmentation
- **C-style API**: Not type-safe, doesn't leverage C++20
- **Portability concerns**: Need to maintain ports for all platforms
- **Learning curve**: Users must learn external RTOS API

**Our approach**:
- Pure C++20, compile-time configuration
- Zero dynamic allocation
- Type-safe APIs with concepts
- Native to Alloy, no external dependencies

### 2. Cooperative Multitasking Only
**Rejected**:
- **No preemption**: High-priority tasks wait for low-priority to yield
- **Non-deterministic**: Hard to guarantee response times
- **Manual yielding**: Error-prone, easy to forget

**Why preemptive**:
- Automatic preemption ensures responsiveness
- Deterministic priority-based scheduling
- Better for real-time applications

### 3. Single Priority Level (No Priorities)
**Rejected**:
- **No differentiation**: Can't prioritize critical tasks
- **Poor real-time behavior**: All tasks equal
- **Limited use cases**: Not suitable for complex applications

**Why 8 priority levels**:
- Sufficient granularity for most applications
- Matches ARM NVIC priority levels
- Small enough for efficient scheduling

### 4. 32 or 64 Priority Levels
**Rejected**:
- **Overkill**: Most apps use 3-5 priorities
- **More overhead**: Larger ready queue data structures
- **Slower scheduler**: More checks to find highest priority

**Why 8 levels**:
- Sweet spot: enough flexibility, minimal overhead
- Fits in 3 bits (0-7)
- Industry standard (FreeRTOS default)

### 5. Dynamic Task Creation at Runtime
**Rejected**:
- **Memory fragmentation**: Tasks allocated from heap
- **Unpredictable memory usage**: Hard to validate
- **Runtime overhead**: Task creation is expensive

**Why compile-time only**:
- Zero fragmentation (all static)
- Predictable memory footprint
- Faster task switching (no dynamic lookup)
- Safer (no allocation failures)

### 6. Implement Only Message Queues (No Semaphores/Mutexes)
**Rejected**:
- **Message queues too heavy**: Overkill for simple synchronization
- **Less flexible**: Can't model all sync patterns efficiently

**Why multiple IPC mechanisms**:
- **Queues**: Best for producer-consumer, data transfer
- **Semaphores**: Best for resource counting, signaling
- **Mutexes**: Best for exclusive access, priority inheritance
- **Events**: Best for lightweight notifications

Each mechanism optimized for its use case.

## Open Questions
None - all design decisions finalized based on user feedback.

## Architecture Overview

### High-Level Design
```
┌─────────────────────────────────────────────┐
│         User Application Tasks              │
│                                              │
│  Task1 (Pri 7) ─┐                           │
│  Task2 (Pri 5) ─┼→ RTOS Scheduler           │
│  Task3 (Pri 3) ─┘                           │
│  Idle  (Pri 0)                               │
└──────────┬──────────────────────────────────┘
           │
┌──────────▼──────────────────────────────────┐
│         Alloy RTOS Core                     │
│  ┌────────────────────────────────────┐     │
│  │  Scheduler (preemptive, priority)  │     │
│  │  - Ready queue (8 priority levels) │     │
│  │  - Context switcher (ASM)          │     │
│  │  - Tick handler (SysTick ISR)      │     │
│  └────────────────────────────────────┘     │
│                                              │
│  ┌────────────────────────────────────┐     │
│  │  IPC Mechanisms                    │     │
│  │  - Message Queues (type-safe)      │     │
│  │  - Semaphores (binary/counting)    │     │
│  │  - Mutexes (priority inheritance)  │     │
│  │  - Event Flags (32-bit mask)       │     │
│  └────────────────────────────────────┘     │
└──────────┬──────────────────────────────────┘
           │
┌──────────▼──────────────────────────────────┐
│         SysTick Timer HAL                   │
│  - Provides 1ms tick interrupt              │
│  - Scheduler calls on each tick             │
└─────────────────────────────────────────────┘
```

### Key Design Principles

1. **Zero-Cost Abstraction**
   - All task configuration at compile time
   - Constexpr validation
   - Inlined critical paths
   - No virtual functions

2. **Type Safety**
   - Templated message queues (type-safe messages)
   - Concepts for task validation
   - Static_assert for configuration errors

3. **Determinism**
   - Fixed priority scheduling
   - Bounded execution time
   - No dynamic allocation
   - Predictable memory usage

4. **Minimal Overhead**
   - Fast context switch (~5-10µs on ARM)
   - Efficient ready queue (bitmap + array)
   - Atomic operations for thread safety

5. **Safety First**
   - Stack overflow detection (debug)
   - Priority inheritance (prevents inversion)
   - Deadlock detection (debug builds)
   - Validate all parameters at compile time

## References
- FreeRTOS: Industry-standard RTOS design patterns
- ARM Cortex-M RTOS: PendSV for context switching
- Modern C++ Design: Template metaprogramming for compile-time config
- Real-Time Concepts for Embedded Systems: Priority inversion, scheduling theory
- µC/OS-II: Classic RTOS architecture
