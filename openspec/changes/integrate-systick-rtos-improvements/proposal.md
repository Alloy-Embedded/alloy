# Integrate SysTick with RTOS Type-Safety Improvements

## Why

The Alloy RTOS currently has **excellent fundamental implementation** with priority-based preemptive scheduling, but it doesn't fully leverage the **compile-time type safety philosophy** used throughout the HAL layer. Additionally, the integration with SysTick is functional but could benefit from:

1. **Compile-time task registration** - Tasks are currently registered dynamically in constructors
2. **Type-safe IPC validation** - Message queues use basic `static_assert` instead of C++20 concepts
3. **Result<T,E> consistency** - RTOS APIs return `bool` while HAL uses `Result<T,E>`
4. **SysTick integration** - Not standardized across boards, limiting RTOS portability
5. **Memory footprint calculation** - Computed manually instead of at compile-time

This creates inconsistency between HAL (100% compile-time safe) and RTOS (mostly runtime safe).

**Vision**: Make RTOS as type-safe as the HAL, with compile-time validation and zero-overhead abstractions.

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

### Phase 5: Advanced Features
7. **Task notifications** (lightweight alternative to queues)
8. **Static memory pools** with type safety
9. **Scheduler configuration** as template parameters
10. **Compile-time deadlock detection** (optional, advanced)

## Impact

### Affected Specs
- `rtos-core` (NEW) - Core RTOS with compile-time task registration
- `rtos-timing` (NEW) - SysTick integration and timing validation
- `rtos-type-safety` (NEW) - Concepts, Result<T,E>, and type-safe IPC
- `board-support` (MODIFIED) - Add RTOS tick integration
- `examples-rtos` (NEW) - RTOS examples demonstrating type safety

### Affected Code
- `src/rtos/rtos.hpp` - Add variadic task registration
- `src/rtos/queue.hpp` - Add concepts for type safety
- `src/rtos/mutex.hpp` - Add multi-lock guard
- `src/rtos/scheduler.hpp` - Add Result<T,E> returns
- `src/rtos/task_notification.hpp` (NEW) - Lightweight signaling
- `src/rtos/memory_pool.hpp` (NEW) - Type-safe allocation
- `boards/*/board.cpp` - Standardize RTOS tick integration
- `examples/rtos/` (NEW) - Type-safe RTOS examples

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

**Week 1-2**: SysTick integration (non-breaking)
- All boards get standardized tick
- Examples demonstrate integration
- RTOS::tick() gets Result<T,E> wrapper

**Week 3-4**: Compile-time task registration (breaking)
- Variadic template system added
- Old API still works with deprecation warning
- Examples show both old and new

**Week 5-6**: Type-safe IPC (non-breaking)
- Concepts added (opt-in)
- Old static_assert still works
- Examples demonstrate concepts

**Week 7-8**: Result<T,E> integration (breaking)
- All APIs return Result<T,E>
- Compatibility wrappers provided
- Migration guide published

**Week 9-10**: Advanced features (non-breaking)
- Task notifications
- Memory pools
- Enhanced examples

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
