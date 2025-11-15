# Integrate SysTick with RTOS Type-Safety Improvements

## Why

The Alloy RTOS currently has **excellent fundamental implementation** with priority-based preemptive scheduling, but it doesn't fully leverage the **compile-time type safety philosophy** used throughout the HAL layer. Additionally, the integration with SysTick is functional but could benefit from improvements.

### Current Strengths (Best-in-Class)

| Feature | Implementation | Status |
|---------|---------------|--------|
| **O(1) Scheduler** | CLZ-based priority bitmap | ⭐⭐⭐⭐⭐ |
| **Context Switch** | PendSV with <10µs latency | ⭐⭐⭐⭐⭐ |
| **Static Memory** | Zero heap usage | ⭐⭐⭐⭐⭐ |
| **Type Safety (IPC)** | Template-based Queue<T,N> | ⭐⭐⭐⭐ |
| **Priority Inheritance** | Full implementation for mutexes | ⭐⭐⭐⭐⭐ |
| **Code Quality** | Clean, well-documented | ⭐⭐⭐⭐⭐ |
| **TCB Size** | 32 bytes (3x smaller than FreeRTOS) | ⭐⭐⭐⭐⭐ |

### Areas for Improvement

1. **Compile-time task registration** - Tasks are currently registered dynamically in constructors
2. **Type-safe IPC validation** - Message queues use basic `static_assert` instead of C++20 concepts
3. **Result<T,E> consistency** - RTOS APIs return `bool` while HAL uses `Result<T,E>`
4. **SysTick integration** - Uses platform-specific ifdefs, not standardized across boards
5. **Memory footprint calculation** - Computed manually instead of at compile-time
6. **C++23 features** - Not leveraging `consteval`, `if consteval`, deducing this, etc.

This creates inconsistency between HAL (100% compile-time safe) and RTOS (mostly runtime safe).

**Vision**: Make RTOS as type-safe as the HAL, with compile-time validation and zero-overhead abstractions using modern C++23.

## What Changes

### Phase 1: SysTick Integration (Foundation)
1. **Standardize RTOS tick source** across all boards
   - Unified `BoardSysTick` type integration
   - Compile-time tick rate validation (1ms standard for RTOS)
   - Consistent `SysTick_Handler() → RTOS::tick()` pattern
   - Support for tickless idle (future)

### Phase 2: Compile-Time Task System
2. **Variadic template task registration**
   - Define all tasks as types: `TaskSet<Task1, Task2, Task3>`
   - Total RAM calculated at compile-time
   - Priority conflict detection at compile-time
   - Single `RTOS::start<TaskSet>()` entry point

3. **Task configuration validation**
   - Stack size validation (min 256 bytes, 8-byte aligned)
   - Priority range validation (0-7)
   - Total memory budget validation
   - Stack overflow detection in debug builds

### Phase 3: Type-Safe IPC
4. **C++20 Concepts for message types**
   - `IPCMessage` concept for queue messages
   - `QueueProducer` and `QueueConsumer` concepts
   - Compile-time producer/consumer validation
   - Type-safe message routing

5. **Enhanced synchronization primitives**
   - Multi-mutex deadlock-free lock guard
   - Scoped locks with RAII guarantees
   - Lock order validation at compile-time (optional)

### Phase 4: Error Handling Consistency
6. **Result<T,E> integration**
   - Replace `bool` returns with `Result<T, RTOSError>`
   - Monadic operations for error composition
   - Consistent with HAL error handling
   - Zero overhead (union-based)

### Phase 5: C++23 Features
7. **consteval for guaranteed compile-time** - Memory calculations MUST happen at compile-time
8. **if consteval for dual-mode functions** - Same function works compile-time and runtime
9. **Deducing this** - Cleaner CRTP patterns and better error messages
10. **fixed_string task names** - Zero RAM cost (names stored in .rodata)

### Phase 6: Advanced Features
11. **Task notifications** (lightweight alternative to queues - 8 bytes vs 20+ bytes)
12. **Static memory pools** with type safety and bounds checking
13. **Scheduler configuration** as template parameters
14. **Compile-time deadlock detection** (optional, advanced)
15. **Tickless idle hooks** for low-power applications

## Impact

### Affected Specs
- `rtos-core` (NEW) - Core RTOS with compile-time task registration
- `rtos-timing` (NEW) - SysTick integration and timing validation
- `rtos-type-safety` (NEW) - Concepts, Result<T,E>, and type-safe IPC
- `board-support` (MODIFIED) - Add RTOS tick integration
- `examples-rtos` (NEW) - RTOS examples demonstrating type safety

### Affected Code

**Core RTOS (Modified):**
- `src/rtos/rtos.hpp` - Add variadic task registration, C++23 features
- `src/rtos/queue.hpp` - Add concepts for type safety
- `src/rtos/mutex.hpp` - Add multi-lock guard, Result<T,E>
- `src/rtos/scheduler.hpp` - Add Result<T,E> returns
- `src/rtos/platform/arm_context.hpp` - FPU lazy context saving (Cortex-M4F/M7F)
- `src/rtos/platform/critical_section.hpp` - Unchanged

**New Files:**
- `src/rtos/task_notification.hpp` (NEW) - Lightweight signaling (8 bytes)
- `src/rtos/memory_pool.hpp` (NEW) - Type-safe allocation
- `src/rtos/concepts.hpp` (NEW) - All C++20/23 concepts
- `src/rtos/config.hpp` (NEW) - Compile-time RTOS configuration

**Board Integration (Standardized):**
- `boards/nucleo_f401re/board.cpp` - Unified SysTick integration
- `boards/nucleo_f722ze/board.cpp` - Unified SysTick integration
- `boards/nucleo_g071rb/board.cpp` - Unified SysTick integration
- `boards/nucleo_g0b1re/board.cpp` - Unified SysTick integration
- `boards/same70_xplained/board.cpp` - Unified SysTick integration

**Examples (NEW):**
- `examples/rtos/hello_world/` - Minimal TaskSet example
- `examples/rtos/producer_consumer/` - Type-safe queue IPC
- `examples/rtos/mutex_example/` - Resource sharing
- `examples/rtos/task_notifications/` - Lightweight IPC
- `examples/rtos/memory_pool/` - Dynamic allocation
- `examples/rtos/multi_mutex/` - Deadlock-free locking

**Removed:**
- `src/rtos/platform/arm_systick_integration.cpp` - Replaced by unified board integration

### Benefits
- ✅ **100% compile-time validation** - Task configuration errors caught before deployment
- ✅ **Type-safe messaging** - Producer/consumer mismatches impossible
- ✅ **Zero overhead** - All validation at compile-time, no runtime cost
- ✅ **Consistent error handling** - Result<T,E> throughout stack
- ✅ **Portable RTOS code** - Same task code works on all boards
- ✅ **Educational examples** - Showcase modern C++ in embedded RTOS
- ✅ **Memory transparency** - Total footprint known at compile-time
- ✅ **RTOS philosophy alignment** - Matches HAL's compile-time safety

### Breaking Changes
- **BREAKING**: Task registration changes from constructor-based to variadic template
  - **Migration**: Replace individual `Task<>` declarations with `TaskSet<>`
  - **Timeline**: Provide compatibility shim for 1 release cycle

- **BREAKING**: RTOS APIs return `Result<T,E>` instead of `bool`
  - **Migration**: Replace `if (lock())` with `if (lock().is_ok())`
  - **Timeline**: Provide compatibility helpers for 1 release cycle

### Non-Breaking Additions
- Task notifications (new feature)
- Memory pools (new feature)
- Concepts-based validation (opt-in)
- Multi-mutex lock guard (new API)
- Compile-time deadlock detection (opt-in)

## Migration Strategy

### Backward Compatibility Period (1 Release)

**Old API (still works with deprecation warning)**:
```cpp
Task<512, Priority::High> task1(func1, "Task1");
Task<512, Priority::Normal> task2(func2, "Task2");
RTOS::start();  // Old API
```

**New API (recommended)**:
```cpp
using Task1 = Task<512, Priority::High, "Task1">;
using Task2 = Task<512, Priority::Normal, "Task2">;
RTOS::start<TaskSet<Task1, Task2>>();  // New API
```

**Automatic Migration Tool**:
```bash
# Provided script to convert old code to new API
./scripts/migrate_rtos_api.py src/
```

### Phased Rollout

**Phase 1 (Weeks 1-2)**: Result<T,E> Integration (non-breaking with deprecation)
- Replace `bool` returns with `Result<T, RTOSError>`
- Add backward compatibility helpers (`.unwrap_or(false)`)
- Update all RTOS APIs (Mutex, Queue, Semaphore, RTOS::tick())
- Migration guide published

**Phase 2 (Weeks 3-5)**: Compile-Time TaskSet (breaking with migration path)
- Variadic template `TaskSet<Tasks...>` added
- Compile-time RAM calculation and validation
- Old API still works with deprecation warning
- Automated migration script provided
- Examples show both old and new

**Phase 3 (Weeks 6-7)**: Concept-Based Type Safety (non-breaking)
- `IPCMessage<T>` concept for queues
- `RTOSTickSource` concept for tick validation
- `QueueProducer`/`QueueConsumer` concepts (opt-in)
- Old static_assert still works
- Examples demonstrate concepts

**Phase 4 (Weeks 8-9)**: Unified SysTick Integration (non-breaking)
- Standardize all board.cpp SysTick_Handler patterns
- Remove platform-specific ifdefs
- Concept-based tick source validation
- All 5 boards unified

**Phase 5 (Weeks 10-12)**: C++23 Enhancements (non-breaking)
- `consteval` for guaranteed compile-time calculations
- `if consteval` for dual-mode functions
- Deducing this for cleaner CRTP
- `fixed_string` for zero-RAM task names
- Update CMakeLists.txt to require C++23

**Phase 6 (Weeks 13-16)**: Advanced Features (non-breaking)
- Task notifications (8 bytes per task)
- Static memory pools
- Tickless idle hooks
- Enhanced examples

**Phase 7 (Weeks 17-18)**: Documentation & Release
- Complete API documentation
- Integration guides
- Migration tutorials
- Final validation and release

## Success Criteria

### Technical Metrics
1. ✅ Total RAM footprint calculated at compile-time (< 5% error from actual)
2. ✅ Zero runtime overhead (same assembly as current implementation)
3. ✅ 100% compile-time error detection for invalid task configurations
4. ✅ Type-safe IPC with zero producer/consumer mismatches possible
5. ✅ Context switch latency unchanged (<10µs on Cortex-M4 @ 100MHz)

### Quality Metrics
1. ✅ 90%+ test coverage for new APIs
2. ✅ All examples compile and run on all 5 boards
3. ✅ Migration guide tested with 3+ real projects
4. ✅ Documentation complete (API docs + tutorials)
5. ✅ Zero regressions in existing RTOS functionality

### Adoption Metrics
1. ✅ New API used in at least 3 example projects
2. ✅ Positive developer feedback on type safety
3. ✅ Migration tool successfully converts existing code
4. ✅ Integration guide followed for new board ports
