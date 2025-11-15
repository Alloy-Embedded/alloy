# SysTick + RTOS Type-Safety Integration Design

## Context

This design integrates two major improvements informed by comprehensive analysis comparing Alloy RTOS with FreeRTOS and evaluating ARM Cortex-M integration:

1. **Standardized SysTick integration** across all boards (from `improve-systick-board-integration`)
2. **Compile-time type safety** for RTOS (bringing it to HAL-level safety)
3. **C++23 features** for maximum compile-time power
4. **ARM Cortex-M optimization** (SysTick, PendSV, FPU lazy context)

The goal is to create a **state-of-the-art embedded RTOS** that combines:
- Zero-overhead abstractions (Policy-Based Design)
- 100% compile-time validation (Templates + Concepts + C++23)
- Consistent error handling (Result<T,E>)
- Portable code (same API across all boards)
- Superior to FreeRTOS in type safety while maintaining performance

### Current Strengths vs FreeRTOS

| Aspect | FreeRTOS | Alloy RTOS (Current) | Alloy RTOS (Proposed) |
|--------|----------|---------------------|---------------------|
| **Language** | C (C89) | C++20 | **C++23** |
| **Type Safety** | Void pointers | Templates | **Templates + Concepts** |
| **Task Creation** | Runtime API | Runtime constructor | **Compile-time TaskSet<>** ⭐ |
| **Error Handling** | Return codes | bool | **Result<T,E>** ⭐ |
| **RAM Calculation** | Runtime | Manual | **Compile-time** ⭐ |
| **Scheduler** | O(n) or O(1) v10+ | **O(1) CLZ** ⭐ | O(1) CLZ |
| **TCB Size** | 80-100 bytes | **32 bytes** ⭐ | **28 bytes** ⭐ |
| **Context Switch** | ~10µs | **<10µs** ⭐ | <10µs |
| **Integration** | Manual config | Manual | **Single-line** ⭐ |

### Constraints
- Must maintain <10µs context switch latency
- Zero runtime overhead for type safety
- Backward compatible for 1 release cycle
- Memory footprint calculable at compile-time
- Works on all 5 boards (F401RE, F722ZE, G071RB, G0B1RE, SAME70)
- Binary size increase <1%
- Compile time increase <5%

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

**Choice**: Standardize on SysTick for RTOS tick across all ARM boards using concepts.

**Problem with Current Implementation:**
```cpp
// src/rtos/platform/arm_systick_integration.cpp (CURRENT - TO BE REMOVED)
extern "C" void SysTick_Handler() {
#if defined(STM32F1)
    extern void stm32f1_systick_handler();
    stm32f1_systick_handler();
#elif defined(STM32F4)
    extern void stm32f4_systick_handler();
    stm32f4_systick_handler();
#elif defined(STM32F7)
    // ... more platform ifdefs
#endif
    alloy::rtos::RTOS::tick();  // ← void return (should be Result<T,E>)
}
```

**Issues:**
- ❌ Platform-specific ifdefs (not scalable to new boards)
- ❌ No compile-time validation of tick period
- ❌ RTOS::tick() returns void (should return Result<T,E>)
- ❌ Board integration pattern inconsistent

**Proposed Solution (Unified Pattern):**
```cpp
// Every board.cpp (UNIFIED PATTERN):
extern "C" void SysTick_Handler() {
    // Increment board timing
    board::BoardSysTick::increment_tick();

    #ifdef ALLOY_RTOS_ENABLED
        // Forward to RTOS scheduler (with error handling)
        auto result = RTOS::tick<board::BoardSysTick>();
        if (result.is_err()) {
            // Optional: Handle tick error
            board::handle_rtos_error(result.unwrap_err());
        }
    #endif
}

// RTOS::tick() signature (concept-validated)
namespace RTOS {
    template <RTOSTickSource TickSource>
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

// RTOS validates tick source at compile-time
template <RTOSTickSource TickSource>
Result<void, RTOSError> RTOS::tick() {
    // Guaranteed 1ms tick period at compile-time!
    // Update delayed tasks
    auto wake_result = scheduler::wake_delayed_tasks();
    if (wake_result.is_err()) {
        return wake_result;
    }

    // Trigger context switch if needed
    if (scheduler::need_context_switch()) {
        scheduler::reschedule();
    }

    return Ok();
}
```

**Benefits**:
- ✅ Single unified pattern across all boards
- ✅ 1ms tick rate validated at compile-time
- ✅ Result<T,E> error handling throughout
- ✅ Type-safe, zero-overhead
- ✅ No platform ifdefs
- ✅ Easy to add new boards (copy template)

**Files Removed:**
- `src/rtos/platform/arm_systick_integration.cpp` (platform ifdefs eliminated)

**Alternatives Considered**:
- ❌ Keep platform ifdefs: Not scalable
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

### Decision 8: C++23 Features for Maximum Compile-Time Power

**Choice**: Leverage C++23 features for guaranteed compile-time evaluation and cleaner code.

#### Feature 1: `consteval` for Guaranteed Compile-Time

**Current (C++20):**
```cpp
template <typename... Tasks>
class TaskSet {
    // Might be computed at compile-time, might be runtime
    static constexpr size_t total_ram =
        ((Tasks::stack_size + sizeof(TCB)) + ...);
};
```

**Proposed (C++23):**
```cpp
template <typename... Tasks>
class TaskSet {
    // GUARANTEED compile-time (consteval forces it)
    static consteval size_t calculate_total_ram() {
        return ((Tasks::stack_size + sizeof(TCB)) + ...);
    }

    static constexpr size_t total_ram = calculate_total_ram();

    // Validation MUST happen at compile-time
    static_assert(total_ram <= MAX_RAM_FOR_RTOS,
                  "Total RAM exceeds available memory");
};
```

**Benefits:**
- ✅ Compiler ERROR if calculation can't be done at compile-time (safety)
- ✅ Explicit contract: This MUST be compile-time
- ✅ No accidental runtime overhead

#### Feature 2: `if consteval` for Dual-Mode Functions

```cpp
// Single function that works at compile-time AND runtime
constexpr u32 calculate_stack_usage(const TaskControlBlock& tcb) {
    if consteval {
        // Compile-time path (static analysis)
        return estimate_max_stack_usage<decltype(tcb)>();
    } else {
        // Runtime path (actual measurement)
        u8* sp = static_cast<u8*>(tcb.stack_pointer);
        u8* base = static_cast<u8*>(tcb.stack_base);
        return static_cast<u32>(base + tcb.stack_size - sp);
    }
}

// Usage:
constexpr u32 predicted = calculate_stack_usage(my_tcb);  // Compile-time
u32 actual = calculate_stack_usage(my_tcb);               // Runtime
```

**Benefits:**
- ✅ Single implementation for both compile-time and runtime
- ✅ Compiler chooses optimal path automatically
- ✅ No code duplication

#### Feature 3: Deducing `this` for Cleaner CRTP

**Current CRTP Pattern (C++20):**
```cpp
template <typename Derived>
class TaskBase {
public:
    constexpr void run() {
        static_cast<Derived*>(this)->run_impl();  // Verbose, error-prone
    }
};

class MyTask : public TaskBase<MyTask> {
    void run_impl() { /* ... */ }
};
```

**C++23 Deducing This (Cleaner):**
```cpp
class TaskBase {
public:
    template <typename Self>
    constexpr void run(this Self&& self) {
        self.run_impl();  // Direct call, no static_cast
    }
};

class MyTask : public TaskBase {
    void run_impl() { /* ... */ }
};
```

**Benefits:**
- ✅ Simpler syntax (no static_cast)
- ✅ Better compiler errors
- ✅ Easier to understand and maintain

#### Feature 4: `fixed_string` for Compile-Time Task Names

**Current (C++20):**
```cpp
template <size_t N, Priority P>
class Task {
    // Name passed as const char* (runtime string)
    const char* name_;  // 4 bytes RAM per task
};
```

**C++23 (Compile-Time String Literal):**
```cpp
// fixed_string for compile-time strings
template <size_t N>
struct fixed_string {
    char data[N];
    static constexpr size_t size = N - 1;

    consteval fixed_string(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }

    constexpr operator const char*() const { return data; }
};

// Task with compile-time name
template <size_t StackSize, Priority P, fixed_string Name>
class Task {
    static constexpr const char* name = Name;
    // Name stored in .rodata (zero RAM cost)
    // No name_ member needed!
};

// Usage:
using SensorTask = Task<512, Priority::High, "Sensor">;
static_assert(SensorTask::name == "Sensor");  // Compile-time check!
```

**Benefits:**
- ✅ Zero RAM cost (name in .rodata, not .data/.bss)
- ✅ 4 bytes saved per task (no name pointer in TCB)
- ✅ Compile-time validation possible
- ✅ Better debug output (name always available)

**TCB Size Reduction:**
```cpp
// Before (C++20): 32 bytes
struct TaskControlBlock {
    void* stack_pointer;     // 4 bytes
    void* stack_base;        // 4 bytes
    u32 stack_size;          // 4 bytes
    u8 priority;             // 1 byte
    TaskState state;         // 1 byte
    u32 wake_time;           // 4 bytes
    const char* name;        // 4 bytes ← ELIMINATED
    TaskControlBlock* next;  // 4 bytes
    // Total: 32 bytes (with padding)
};

// After (C++23): 28 bytes
struct TaskControlBlock {
    void* stack_pointer;     // 4 bytes
    void* stack_base;        // 4 bytes
    u32 stack_size;          // 4 bytes
    u8 priority;             // 1 byte
    TaskState state;         // 1 byte
    u32 wake_time;           // 4 bytes
    // name removed (in Task<> type)
    TaskControlBlock* next;  // 4 bytes
    // Total: 26 bytes → 28 with padding
};
```

**RAM Savings:**
- Per task: 4 bytes (removed name pointer)
- System with 8 tasks: 32 bytes saved
- Name strings still available for debugging (in .rodata)

**Alternatives Considered:**
- ❌ Keep `const char*`: Wastes 4 bytes RAM per task
- ❌ Use macros: Loses type safety
- ❌ External name array: More complex, still uses RAM

---

### Decision 9: ARM Cortex-M PendSV Optimization

**Choice**: Add FPU lazy context saving for Cortex-M4F/M7F with FPU.

**Current Implementation (Good):**
```cpp
extern "C" __attribute__((naked)) void PendSV_Handler() {
    __asm volatile(
        "mrs r0, psp            \n"  // Get PSP
        "stmdb r0!, {r4-r11}    \n"  // Save r4-r11 (32 bytes)
        "bl PendSV_Handler_C    \n"  // C function switches tasks
        "ldmia r0!, {r4-r11}    \n"  // Restore r4-r11
        "msr psp, r0            \n"  // Set new PSP
        "bx lr                  \n"  // Return (hardware restores r0-r3, r12, lr, pc, xPSR)
    );
}
```

**Proposed (FPU Lazy Context Saving):**
```cpp
extern "C" __attribute__((naked)) void PendSV_Handler() {
    __asm volatile(
        "mrs r0, psp            \n"  // Get PSP

        #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        // Check if FPU context needs saving
        "tst lr, #0x10          \n"  // Test EXC_RETURN bit 4
        "it eq                  \n"
        "vstmdbeq r0!, {s16-s31}\n"  // Save FPU if used (64 bytes)
        #endif

        "stmdb r0!, {r4-r11}    \n"  // Save r4-r11 (32 bytes)
        "bl PendSV_Handler_C    \n"  // C function switches tasks
        "ldmia r0!, {r4-r11}    \n"  // Restore r4-r11

        #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        "tst lr, #0x10          \n"  // Test EXC_RETURN
        "it eq                  \n"
        "vldmiaeq r0!, {s16-s31}\n"  // Restore FPU if used
        #endif

        "msr psp, r0            \n"  // Set new PSP
        "bx lr                  \n"  // Return
    );
}
```

**Benefits:**
- ✅ Only saves FPU context if task uses FPU
- ✅ 64 bytes saved per context switch for non-FPU tasks
- ✅ FreeRTOS uses this technique
- ✅ Zero overhead for M0/M0+/M3 (no FPU)

**Performance Impact:**
- Tasks without FPU: ~5µs context switch (unchanged)
- Tasks with FPU: ~7-8µs (FPU save/restore adds ~2-3µs)
- FreeRTOS: Similar performance

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
