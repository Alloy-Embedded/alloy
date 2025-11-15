# SysTick + RTOS Type-Safety Integration Design

## Context

This design integrates two major improvements:
1. **Standardized SysTick integration** across all boards (from `improve-systick-board-integration`)
2. **Compile-time type safety** for RTOS (bringing it to HAL-level safety)

The goal is to create a **state-of-the-art embedded RTOS** that combines:
- Zero-overhead abstractions (Policy-Based Design)
- 100% compile-time validation (Templates + Concepts)
- Consistent error handling (Result<T,E>)
- Portable code (same API across all boards)

### Constraints
- Must maintain <10µs context switch latency
- Zero runtime overhead for type safety
- Backward compatible for 1 release cycle
- Memory footprint calculable at compile-time
- Works on all 5 boards (F401RE, F722ZE, G071RB, G0B1RE, SAME70)

### Stakeholders
- Embedded RTOS developers
- Safety-critical application developers
- Educators teaching modern C++ embedded
- Open-source contributors

## Goals / Non-Goals

### Goals
1. **Unify** SysTick integration for RTOS across all boards
2. **Eliminate** runtime task registration (move to compile-time)
3. **Enforce** type-safe IPC with C++20 concepts
4. **Standardize** error handling with Result<T,E>
5. **Calculate** total memory footprint at compile-time
6. **Provide** migration path from old API
7. **Document** modern C++ RTOS patterns

### Non-Goals
1. **Not** changing scheduler algorithm (priority-based is proven)
2. **Not** adding dynamic task creation (static only by design)
3. **Not** supporting non-ARM architectures in this phase
4. **Not** implementing software timers (future enhancement)
5. **Not** adding FreeRTOS compatibility layer

## Decisions

### Decision 1: Variadic Task Registration with TaskSet

**Choice**: Tasks defined as types, registered via variadic template.

```cpp
// Define tasks as types
using SensorTask = Task<512, Priority::High, "Sensor">;
using DisplayTask = Task<512, Priority::Normal, "Display">;
using LogTask = Task<256, Priority::Low, "Log">;

// Register all tasks in one place
using AllTasks = TaskSet<SensorTask, DisplayTask, LogTask>;

int main() {
    board::init();
    RTOS::start<AllTasks>();  // Compile-time validated, never returns
}
```

**Compile-Time Validations**:
```cpp
template <typename... Tasks>
class TaskSet {
    // Total RAM calculation
    static constexpr size_t total_ram =
        ((Tasks::stack_size + sizeof(TaskControlBlock)) + ...);

    // Validate total RAM fits in budget
    static_assert(total_ram <= MAX_RAM_FOR_RTOS,
                  "Total task RAM exceeds available memory");

    // Validate at least one task
    static_assert(sizeof...(Tasks) > 0,
                  "At least one task required");

    // Optional: Validate unique priorities
    static_assert(AllUniquePriorities<Tasks...>() || !STRICT_MODE,
                  "Tasks cannot share same priority in strict mode");

    // Validate task count
    static_assert(sizeof...(Tasks) <= MAX_TASKS,
                  "Too many tasks (max 32)");
};
```

**Benefits**:
- ✅ Single source of truth for all tasks
- ✅ Total RAM known at compile-time
- ✅ Priority conflicts detected at compile-time
- ✅ Stack overflow impossible (validated before deployment)
- ✅ Clear application structure

**Alternatives Considered**:
- ❌ Keep dynamic registration: Loses compile-time validation
- ❌ Macro-based registration: Loses type safety
- ❌ Boost.MPL style: Too complex, dated pattern

---

### Decision 2: C++20 Concepts for Type-Safe IPC

**Choice**: Use concepts to validate message types and producer/consumer relationships.

```cpp
// Concept: Valid IPC message
template <typename T>
concept IPCMessage = requires {
    requires std::is_trivially_copyable_v<T>;
    requires sizeof(T) <= 256;  // Reasonable size limit
    requires alignof(T) <= 8;   // Standard alignment
};

// Concept: Task can produce to this queue
template <typename Task, typename Queue>
concept QueueProducer = requires(Task t, Queue& q) {
    { t.send_to(q) } -> std::same_as<Result<void, RTOSError>>;
};

// Concept: Task can consume from this queue
template <typename Task, typename Queue>
concept QueueConsumer = requires(Task t, Queue& q) {
    { t.receive_from(q) } -> std::same_as<Result<typename Queue::value_type, RTOSError>>;
};

// Queue enforces message type
template <IPCMessage T, size_t Capacity>
class Queue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be power of 2");
    // ...
};
```

**Compile-Time Validation Example**:
```cpp
struct SensorData {
    uint32_t timestamp;
    int16_t temperature;
    int16_t humidity;
};

// This compiles - valid message type
Queue<SensorData, 8> sensor_queue;

// This FAILS at compile-time - not trivially copyable
struct BadMessage {
    std::string data;  // Error: has non-trivial destructor
};
Queue<BadMessage, 8> bad_queue;  // Compile error with clear message

// This FAILS at compile-time - producer/consumer mismatch
Queue<SensorData, 8> queue;
static_assert(QueueProducer<SensorTask, decltype(queue)>,
              "SensorTask must be producer for sensor_queue");
```

**Benefits**:
- ✅ 10x better error messages than SFINAE
- ✅ Catches type mismatches at compile-time
- ✅ Documents requirements clearly
- ✅ Zero runtime overhead

**Alternatives Considered**:
- ❌ SFINAE (enable_if): Cryptic errors, dated pattern
- ❌ Runtime type checking: Overhead + not embedded-friendly
- ❌ Duck typing: No validation

---

### Decision 3: Result<T,E> Error Handling

**Choice**: Replace `bool` returns with `Result<T, RTOSError>` for consistency with HAL.

```cpp
// Error type
enum class RTOSError : uint8_t {
    Timeout,
    NotOwner,
    QueueFull,
    QueueEmpty,
    InvalidPriority,
    StackOverflow,
    NoMemory,
    Deadlock
};

// Updated APIs
class Mutex {
public:
    Result<void, RTOSError> lock(u32 timeout_ms = INFINITE);
    Result<void, RTOSError> unlock();
};

template <typename T, size_t Capacity>
class Queue {
public:
    Result<void, RTOSError> send(const T& msg, u32 timeout = INFINITE);
    Result<T, RTOSError> receive(u32 timeout = INFINITE);
};
```

**Usage with Monadic Operations**:
```cpp
// Error propagation
auto result = sensor_queue.send(data, 1000)
    .and_then([](auto) {
        return event_flags.set(SENSOR_READY);
    })
    .or_else([](RTOSError err) {
        log_error("Send failed", err);
        return Err(err);
    });

// Pattern matching
sensor_queue.receive(1000)
    .match(
        [](SensorData data) { display_data(data); },
        [](RTOSError err) { display_error(err); }
    );
```

**Backward Compatibility Helper**:
```cpp
// Old code (still works with warning)
if (mutex.lock()) {  // Deprecated warning
    // ...
    mutex.unlock();
}

// New code
if (mutex.lock().is_ok()) {
    // ...
    mutex.unlock();
}

// Or use RAII
auto lock_result = mutex.lock();
if (lock_result.is_ok()) {
    LockGuard guard(mutex);  // RAII unlocks automatically
    // ...
}
```

**Benefits**:
- ✅ Consistent with HAL error handling
- ✅ Forces error handling (Result must be consumed)
- ✅ Composable with monadic operations
- ✅ Zero overhead (union-based)

**Alternatives Considered**:
- ❌ Keep bool: Loses error information
- ❌ Exceptions: Not suitable for embedded
- ❌ Error codes: Easy to ignore

---

### Decision 4: SysTick as RTOS Tick Source

**Choice**: Standardize on SysTick for RTOS tick across all ARM boards.

```cpp
// In board.cpp
extern "C" void SysTick_Handler() {
    // Increment board timing
    board::BoardSysTick::increment_tick();

    #ifdef ALLOY_RTOS_ENABLED
        // Forward to RTOS scheduler
        auto result = RTOS::tick();
        if (result.is_err()) {
            // Handle tick error (optional)
        }
    #endif
}

// RTOS::tick() signature
namespace RTOS {
    Result<void, RTOSError> tick();
}
```

**Compile-Time Validation**:
```cpp
template <typename TickSource>
concept RTOSTickSource = requires {
    // Must have 1ms tick rate for RTOS
    requires TickSource::tick_period_ms == 1;

    // Must provide tick count
    { TickSource::get_tick_count() } -> std::same_as<u32>;

    // Must provide microsecond timing
    { TickSource::micros() } -> std::same_as<u64>;
};

// Validate at RTOS initialization
template <RTOSTickSource TickSource>
void RTOS::initialize() {
    // Guaranteed to have valid tick source
}
```

**Benefits**:
- ✅ Consistent across all boards
- ✅ 1ms tick rate validated at compile-time
- ✅ Single interrupt handler pattern
- ✅ RTOS integration is opt-in (#ifdef)

**Alternatives Considered**:
- ❌ Custom timer per board: Inconsistent
- ❌ Software timer: Drift issues
- ❌ External RTC: Not always available

---

### Decision 5: Task Notifications (Lightweight IPC)

**Choice**: Add task notifications as memory-efficient alternative to queues.

```cpp
template <size_t StackSize, Priority Pri, const char* Name>
class Task {
    // Notification state (8 bytes total)
    volatile uint32_t notification_value_;
    volatile bool notification_pending_;

public:
    // Send notification (callable from ISR)
    Result<void, RTOSError> notify(uint32_t value = 0);

    // Wait for notification (blocks task)
    Result<uint32_t, RTOSError> wait_notification(u32 timeout_ms);

    // Check without blocking
    Result<uint32_t, RTOSError> try_get_notification();
};
```

**Comparison**:
| Feature | Queue<u32, 1> | TaskNotification |
|---------|---------------|------------------|
| Footprint | 16+ bytes | 8 bytes |
| ISR-safe | ✅ Yes | ✅ Yes |
| Latency | ~20 cycles | ~10 cycles |
| Use case | Multiple producers | Single producer |

**Usage**:
```cpp
// ISR → Task communication
void GPIO_IRQHandler() {
    high_priority_task.notify(sensor_value);
}

void high_priority_task_func() {
    while (1) {
        auto result = current_task.wait_notification(INFINITE);
        if (result.is_ok()) {
            uint32_t value = result.unwrap();
            process(value);
        }
    }
}
```

**Benefits**:
- ✅ 50% smaller than queue
- ✅ Faster than queue (fewer copies)
- ✅ Perfect for ISR → Task signaling
- ✅ FreeRTOS-compatible pattern

---

### Decision 6: Static Memory Pools

**Choice**: Provide type-safe memory allocation within RTOS.

```cpp
template <typename T, size_t PoolSize>
class StaticPool {
    static_assert(std::is_trivially_destructible_v<T>,
                  "Pool objects must be trivially destructible");
    static_assert(PoolSize > 0 && PoolSize <= 256,
                  "Pool size must be 1-256 objects");

    struct Block {
        alignas(T) uint8_t storage[sizeof(T)];
        bool in_use;
    };

    Block blocks_[PoolSize];

public:
    // Type-safe allocation
    template <typename... Args>
    Result<T*, RTOSError> allocate(Args&&... args) {
        for (auto& block : blocks_) {
            if (!block.in_use) {
                block.in_use = true;
                T* ptr = new (&block.storage) T(std::forward<Args>(args)...);
                return Ok(ptr);
            }
        }
        return Err(RTOSError::NoMemory);
    }

    // Safe deallocation
    Result<void, RTOSError> deallocate(T* ptr) {
        // Validate pointer is in pool
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t base = reinterpret_cast<uintptr_t>(blocks_);

        if (addr < base || addr >= base + sizeof(blocks_)) {
            return Err(RTOSError::InvalidPointer);
        }

        ptr->~T();  // Call destructor

        size_t index = (addr - base) / sizeof(Block);
        blocks_[index].in_use = false;

        return Ok();
    }

    // Statistics
    size_t available() const;
    static constexpr size_t capacity() { return PoolSize; }
};
```

**Benefits**:
- ✅ Type-safe (wrong type = compile error)
- ✅ Bounds-checked (invalid pointer = runtime error)
- ✅ Deterministic (no heap fragmentation)
- ✅ Compile-time size known

---

### Decision 7: Multi-Mutex Deadlock-Free Lock

**Choice**: Automatic lock ordering by memory address.

```cpp
template <typename... Mutexes>
class ScopedMultiLock {
    static_assert(sizeof...(Mutexes) > 0 && sizeof...(Mutexes) <= 4,
                  "Support 1-4 mutexes");

    std::tuple<Mutexes&...> mutexes_;
    bool locked_;

public:
    explicit ScopedMultiLock(Mutexes&... mutexes)
        : mutexes_(mutexes...), locked_(false) {
        // Sort mutexes by memory address (deadlock-free)
        auto ordered = sort_by_address(mutexes...);

        // Lock in ascending address order
        lock_all(ordered);
        locked_ = true;
    }

    ~ScopedMultiLock() {
        if (locked_) {
            // Unlock in reverse order
            unlock_all_reverse();
        }
    }
};

// Usage
void critical_section() {
    ScopedMultiLock guard(uart_mutex, i2c_mutex, spi_mutex);
    // All mutexes locked in safe order
    use_multiple_resources();
    // Automatic unlock in reverse order
}
```

**Deadlock Prevention**:
- Thread 1: Lock(A), Lock(B) → Actual: Lock(min(A,B)), Lock(max(A,B))
- Thread 2: Lock(B), Lock(A) → Actual: Lock(min(A,B)), Lock(max(A,B))
- Result: Both threads lock in same order → No deadlock possible

---

## Architecture

### Component Diagram

```
┌──────────────────────────────────────────────────────┐
│  Application Layer                                   │
│  - using AllTasks = TaskSet<Task1, Task2, Task3>    │
│  - RTOS::start<AllTasks>()                          │
│  - Compile-time validated                            │
└────────────────────┬─────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────┐
│  RTOS Core (Type-Safe)                               │
│  - TaskSet<Tasks...> variadic registration           │
│  - Result<T, RTOSError> error handling               │
│  - Concepts for IPC validation                       │
│  - Total RAM calculation at compile-time             │
└────────────────────┬─────────────────────────────────┘
                     │
      ┌──────────────┴──────────────┐
      ▼                             ▼
┌──────────────────┐    ┌────────────────────────┐
│  IPC (Type-Safe) │    │  Timing (SysTick)      │
│  - Queue<T, N>   │    │  - BoardSysTick        │
│  - Mutex         │    │  - RTOS::tick()        │
│  - Semaphore     │    │  - 1ms validated       │
│  - EventFlags    │    │  - Result<T,E>         │
│  - Notification  │    └─────────┬──────────────┘
│  - StaticPool<T> │              │
└────────┬─────────┘              │
         │                        │
         ▼                        ▼
┌──────────────────────────────────────────────────────┐
│  Scheduler (Platform-Agnostic)                       │
│  - Priority-based preemptive                         │
│  - O(1) task selection                               │
│  - Priority inheritance                              │
│  - Context switch <10µs                              │
└────────────────────┬─────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────┐
│  Platform Layer (ARM Cortex-M)                       │
│  - Context switch (PendSV)                           │
│  - Critical sections                                 │
│  - Stack initialization                              │
└────────────────────┬─────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────┐
│  Board Layer (board.cpp)                             │
│  - SysTick_Handler() → RTOS::tick()                 │
│  - BoardSysTick type definition                      │
└──────────────────────────────────────────────────────┘
```

### Data Flow: Task Registration

```
1. Compile-Time (Zero Runtime Cost):
   ┌──────────────────────────────────────┐
   │ using AllTasks = TaskSet<            │
   │     Task<512, Pri::High, "Sensor">,  │
   │     Task<512, Pri::Normal, "Display">│
   │ >                                    │
   └────────────┬─────────────────────────┘
                │
                ▼
   ┌──────────────────────────────────────┐
   │ TaskSet Template Instantiation:      │
   │ - Validate stack sizes (512 ≥ 256)  │
   │ - Validate priorities (0-7)          │
   │ - Calculate total RAM: 1088 bytes    │
   │ - Check budget: 1088 ≤ MAX_RAM ✓    │
   └────────────┬─────────────────────────┘
                │
                ▼
   ┌──────────────────────────────────────┐
   │ Compilation Result:                  │
   │ ✅ All checks pass                   │
   │ ✅ Binary includes validated code    │
   │ ✅ No runtime registration overhead  │
   └──────────────────────────────────────┘

2. Runtime (Minimal Overhead):
   ┌──────────────────────────────────────┐
   │ RTOS::start<AllTasks>()              │
   └────────────┬─────────────────────────┘
                │
                ▼
   ┌──────────────────────────────────────┐
   │ For each task in TaskSet:            │
   │ - Initialize stack (already sized)   │
   │ - Add to ready queue (O(1))          │
   │ - No validation needed (done!)       │
   └────────────┬─────────────────────────┘
                │
                ▼
   ┌──────────────────────────────────────┐
   │ Start Scheduler:                     │
   │ - Switch to highest priority task    │
   │ - Enable SysTick interrupt           │
   │ - Never return                       │
   └──────────────────────────────────────┘
```

### Memory Layout (Compile-Time Known)

```
TaskSet<SensorTask, DisplayTask, LogTask>

┌─────────────────────────────────────────┐
│ SensorTask                              │
│ - TCB: 32 bytes                         │
│ - Stack: 512 bytes                      │
│ - Total: 544 bytes                      │
├─────────────────────────────────────────┤
│ DisplayTask                             │
│ - TCB: 32 bytes                         │
│ - Stack: 512 bytes                      │
│ - Total: 544 bytes                      │
├─────────────────────────────────────────┤
│ LogTask                                 │
│ - TCB: 32 bytes                         │
│ - Stack: 256 bytes                      │
│ - Total: 288 bytes                      │
├─────────────────────────────────────────┤
│ IPC Objects                             │
│ - Queue<SensorData, 8>: 72 bytes        │
│ - Mutex: 20 bytes                       │
│ - EventFlags: 12 bytes                  │
│ - Total: 104 bytes                      │
├─────────────────────────────────────────┤
│ RTOS Core                               │
│ - Ready queue: 36 bytes                 │
│ - Scheduler state: 24 bytes             │
│ - Total: 60 bytes                       │
└─────────────────────────────────────────┘

Grand Total: 1,540 bytes ✅ (known at compile-time)
```

## Risks / Trade-offs

### Risk 1: Breaking Changes

**Risk**: Existing code breaks with new API.

**Mitigation**:
- Provide 1 release cycle backward compatibility
- Automatic migration script
- Clear deprecation warnings
- Comprehensive migration guide
- Side-by-side examples (old vs new)

### Risk 2: Increased Compile Time

**Risk**: Heavy template metaprogramming slows compilation.

**Mitigation**:
- Measured <5% increase acceptable
- Extern template instantiations for common types
- Incremental compilation support
- Pre-compiled headers for RTOS core

### Risk 3: Complex Error Messages

**Risk**: Concept errors confusing for beginners.

**Mitigation**:
- Use `static_assert` with clear custom messages
- Provide error message guide
- Examples of common errors in docs
- Educational tutorials explaining concepts

### Risk 4: Learning Curve

**Risk**: Developers unfamiliar with modern C++ struggle.

**Mitigation**:
- Progressive examples (simple → advanced)
- Tutorial series with explanations
- Migration guide from FreeRTOS/Zephyr
- Community support channel

## Performance Targets

| Metric | Current | Target | Validation |
|--------|---------|--------|------------|
| Context Switch | <10µs | <10µs | Oscilloscope |
| Compile Time | Baseline | +5% max | CI measurement |
| Binary Size | Baseline | +1% max | Size comparison |
| RAM Accuracy | Manual | ±2% | Static analysis |
| Concept Errors | N/A | <10 lines | Manual review |

## Migration Plan

### Phase 1: Compatibility (Week 1-2)
- Add new APIs alongside old
- Old APIs work with deprecation warning
- Migration script available
- Examples show both approaches

### Phase 2: Documentation (Week 3-4)
- Migration guide published
- Video tutorials created
- FAQ populated
- Community feedback collected

### Phase 3: Deprecation (Week 5-8)
- Louder warnings
- Announce sunset timeline
- Offer migration support
- Update all examples to new API

### Phase 4: Removal (Release N+2)
- Remove old APIs
- Clean up compatibility layer
- Finalize documentation

## Open Questions

1. **Should we support compile-time deadlock detection?**
   - Pro: Catches bugs at compile-time
   - Con: Complex implementation, may slow compilation
   - Decision: Make it opt-in feature

2. **Should TaskSet enforce unique priorities?**
   - Pro: Clearer task ordering
   - Con: Limits flexibility for some use cases
   - Decision: Make it opt-in (STRICT_MODE)

3. **Should we provide FreeRTOS API compatibility layer?**
   - Pro: Easier migration from FreeRTOS
   - Con: Maintenance burden, API compromises
   - Decision: Not in scope (focus on best API design)

4. **How to handle tick wraparound (49 days @ 1ms)?**
   - Current: Difference calculations handle wraparound
   - Alternative: 64-bit tick counter
   - Decision: Keep 32-bit (standard RTOS practice)

## Success Criteria

1. ✅ All 5 boards run type-safe RTOS
2. ✅ Context switch latency unchanged
3. ✅ Compile-time RAM calculation within ±2% of actual
4. ✅ Migration script converts 3+ projects successfully
5. ✅ All examples build without warnings
6. ✅ Documentation covers 100% of APIs
7. ✅ Positive community feedback on type safety
