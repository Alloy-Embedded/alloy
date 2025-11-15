# Alloy RTOS API Reference

Complete API reference for Alloy RTOS with C++23 enhancements.

**Version**: 2.0 (with Phases 1-6 features)
**C++ Standard**: C++23 required
**Target**: Embedded systems (Cortex-M, ESP32, etc.)

---

## Table of Contents

1. [Core RTOS](#core-rtos)
2. [Tasks](#tasks)
3. [Queue](#queue)
4. [Mutex](#mutex)
5. [Semaphore](#semaphore)
6. [Task Notification](#task-notification)
7. [Memory Pool](#memory-pool)
8. [Tickless Idle](#tickless-idle)
9. [Error Handling](#error-handling)
10. [Concepts](#concepts)

---

## Core RTOS

### Namespace

```cpp
namespace alloy::rtos
```

### RTOS Functions

#### `RTOS::start()`

Start the RTOS scheduler. **Never returns**.

```cpp
[[noreturn]] void RTOS::start();
```

**Example**:
```cpp
int main() {
    // Create tasks...
    RTOS::start();  // Never returns
}
```

---

#### `RTOS::delay()`

Delay the current task for specified milliseconds.

```cpp
void RTOS::delay(core::u32 ms);
```

**Parameters**:
- `ms`: Delay in milliseconds

**Example**:
```cpp
void task_func() {
    while (1) {
        // Do work
        RTOS::delay(100);  // Delay 100ms
    }
}
```

---

#### `RTOS::yield()`

Yield CPU to another ready task of equal or higher priority.

```cpp
void RTOS::yield();
```

**Example**:
```cpp
void cooperative_task() {
    while (1) {
        // Do some work
        RTOS::yield();  // Let other tasks run
    }
}
```

---

#### `RTOS::current_task()`

Get pointer to current task's TCB.

```cpp
TaskControlBlock* RTOS::current_task();
```

**Returns**: Pointer to current TCB or `nullptr`

---

#### `RTOS::get_tick_count()`

Get system tick count in milliseconds.

```cpp
core::u32 RTOS::get_tick_count();
```

**Returns**: Milliseconds since RTOS start

---

#### `RTOS::tick()` (Internal)

Scheduler tick - called from SysTick ISR.

```cpp
core::Result<void, RTOSError> RTOS::tick();
```

**Returns**: Ok on success, Err on failure

**Example** (in board.cpp):
```cpp
extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();
    #ifdef ALLOY_RTOS_ENABLED
        RTOS::tick().unwrap();
    #endif
}
```

---

## Tasks

### Task Template

```cpp
template <size_t StackSize, Priority Pri, fixed_string Name = "task">
class Task;
```

**Template Parameters**:
- `StackSize`: Stack size in bytes (256-65536, 8-byte aligned)
- `Pri`: Task priority (Priority::Idle to Priority::Critical)
- `Name`: Compile-time task name (optional, default="task")

**Validation**:
- Stack size: 256 ≤ size ≤ 65536, must be 8-byte aligned
- Priority: Must be 0-7 (Idle-Critical)
- Name: 1-31 alphanumeric characters, `_`, or `-`

---

### Constructor (New API)

```cpp
explicit Task(void (*task_func)());
```

**Parameters**:
- `task_func`: Task entry point function

**Example**:
```cpp
void sensor_task_func() {
    while (1) {
        // Task work
        RTOS::delay(100);
    }
}

Task<512, Priority::High, "Sensor"> sensor_task(sensor_task_func);
```

---

### Constructor (Old API - Deprecated)

```cpp
[[deprecated]] explicit Task(void (*task_func)(), const char* name);
```

**Parameters**:
- `task_func`: Task entry point function
- `name`: Runtime task name (uses RAM)

---

### Member Functions

#### `get_tcb()`

Get task control block.

```cpp
TaskControlBlock* get_tcb();
const TaskControlBlock* get_tcb() const;
```

---

#### `name()` (Compile-Time)

Get task name.

```cpp
static constexpr const char* name();
```

**Example**:
```cpp
static_assert(sensor_task.name() == "Sensor");
```

---

#### `stack_size()` (Compile-Time)

Get stack size.

```cpp
static constexpr size_t stack_size();
```

---

#### `priority()` (Compile-Time)

Get priority.

```cpp
static constexpr Priority priority();
```

---

### TaskSet Template

Group tasks for compile-time validation.

```cpp
template <typename... Tasks>
struct TaskSet;
```

**Example**:
```cpp
using MyTasks = TaskSet<
    decltype(sensor_task),
    decltype(display_task),
    decltype(logger_task)
>;

// Validation
static_assert(MyTasks::total_ram() <= 8192);
static_assert(MyTasks::validate());
```

---

### TaskSet Functions

#### `count()`

```cpp
static constexpr size_t count();
```

Number of tasks in set.

---

#### `total_ram()`

```cpp
static consteval size_t total_ram();
```

Total RAM (stack + TCB overhead).

---

#### `total_ram_with_budget<Budget>()`

```cpp
template <size_t Budget>
static consteval size_t total_ram_with_budget();
```

Total RAM with budget check (throws if exceeds).

---

#### `highest_priority()`

```cpp
static consteval core::u8 highest_priority();
```

---

#### `lowest_priority()`

```cpp
static consteval core::u8 lowest_priority();
```

---

#### `validate<RequireUnique>()`

```cpp
template <bool RequireUniquePriorities = false>
static consteval bool validate();
```

Validate task set.

---

#### `validate_advanced<>()`

```cpp
template <bool RequireUniquePriorities = false,
          bool WarnPriorityInversion = true>
static consteval bool validate_advanced();
```

Advanced validation (deadlock, priority inversion).

---

## Queue

### Template

```cpp
template <typename T, size_t Capacity>
class Queue;
```

**Constraints**:
- `T` must satisfy `IPCMessage` concept (trivially copyable, ≤256 bytes)
- `Capacity` must be > 0

---

### Member Functions

#### `send()`

Send message to queue (blocking).

```cpp
core::Result<void, RTOSError> send(const T& message, core::u32 timeout_ms = INFINITE);
```

**Parameters**:
- `message`: Message to send
- `timeout_ms`: Timeout in milliseconds (default: INFINITE)

**Returns**:
- Ok on success
- Err(Timeout) if timeout
- Err(QueueFull) if queue full (shouldn't happen with blocking)

**Example**:
```cpp
Queue<SensorData, 8> queue;

SensorData data{...};
auto result = queue.send(data, 1000);  // 1s timeout
if (result.is_err()) {
    // Handle error
}
```

---

#### `receive()`

Receive message from queue (blocking).

```cpp
core::Result<T, RTOSError> receive(core::u32 timeout_ms = INFINITE);
```

**Parameters**:
- `timeout_ms`: Timeout in milliseconds

**Returns**:
- Ok(message) on success
- Err(Timeout) if timeout
- Err(QueueEmpty) if empty (shouldn't happen with blocking)

**Example**:
```cpp
auto result = queue.receive(1000);  // 1s timeout
if (result.is_ok()) {
    SensorData data = result.unwrap();
    // Use data
}
```

---

#### `try_send()`

Non-blocking send.

```cpp
core::Result<void, RTOSError> try_send(const T& message);
```

**Returns**:
- Ok on success
- Err(QueueFull) if queue full

---

#### `try_receive()`

Non-blocking receive.

```cpp
core::Result<T, RTOSError> try_receive();
```

**Returns**:
- Ok(message) if available
- Err(QueueEmpty) if empty

---

#### `is_empty()`

```cpp
bool is_empty() const;
```

---

#### `is_full()`

```cpp
bool is_full() const;
```

---

#### `available()`

```cpp
size_t available() const;
```

Number of messages in queue.

---

## Mutex

### Class

```cpp
class Mutex;
```

---

### Member Functions

#### `lock()`

Acquire mutex (blocking).

```cpp
core::Result<void, RTOSError> lock(core::u32 timeout_ms = INFINITE);
```

**Returns**:
- Ok on success
- Err(Timeout) if timeout
- Err(Deadlock) if would deadlock

---

#### `unlock()`

Release mutex.

```cpp
core::Result<void, RTOSError> unlock();
```

**Returns**:
- Ok on success
- Err(NotOwner) if not owned by current task

---

#### `try_lock()`

Non-blocking lock.

```cpp
core::Result<void, RTOSError> try_lock();
```

**Returns**:
- Ok on success
- Err(NotAvailable) if already locked

---

### LockGuard (RAII)

Automatic mutex management.

```cpp
class LockGuard;
```

**Example**:
```cpp
Mutex mutex;

{
    LockGuard guard(mutex);
    if (guard.locked()) {
        // Critical section
    }
    // Automatically unlocked here
}
```

---

## Semaphore

### Binary Semaphore

```cpp
class BinarySemaphore;
```

#### Constructor

```cpp
BinarySemaphore(bool initial_available = false);
```

---

#### `give()`

```cpp
core::Result<void, RTOSError> give();
```

---

#### `take()`

```cpp
core::Result<void, RTOSError> take(core::u32 timeout_ms = INFINITE);
```

---

#### `try_take()`

```cpp
core::Result<void, RTOSError> try_take();
```

---

### Counting Semaphore

```cpp
class CountingSemaphore;
```

#### Constructor

```cpp
CountingSemaphore(core::u32 max_count, core::u32 initial_count = 0);
```

---

#### `give()`

```cpp
core::Result<void, RTOSError> give();
```

**Returns**: Err(InvalidState) if would exceed max_count

---

#### `take()`

```cpp
core::Result<void, RTOSError> take(core::u32 timeout_ms = INFINITE);
```

---

#### `try_take()`

```cpp
core::Result<void, RTOSError> try_take();
```

---

#### `count()`

```cpp
core::u32 count() const;
```

Current semaphore count.

---

## Task Notification

### Class

```cpp
class TaskNotification;
```

**Overhead**: 8 bytes per task

---

### Enums

#### NotifyAction

```cpp
enum class NotifyAction {
    SetBits,          // OR operation (event flags)
    Increment,        // Add operation (counting semaphore)
    Overwrite,        // Replace value (simple data)
    OverwriteIfEmpty  // Replace only if empty (overflow detection)
};
```

---

#### NotifyClearMode

```cpp
enum class NotifyClearMode {
    ClearOnEntry,  // Clear before blocking
    ClearOnExit,   // Clear after waking (default)
    NoClear        // Manual clear required
};
```

---

### Static Functions

#### `notify()`

Send notification to task.

```cpp
static core::Result<core::u32, RTOSError> notify(
    TaskControlBlock* tcb,
    core::u32 value,
    NotifyAction action
) noexcept;
```

**Parameters**:
- `tcb`: Target task control block
- `value`: Notification value
- `action`: Notification action

**Returns**: Ok(previous_value) or Err

**Example**:
```cpp
TaskNotification::notify(
    sensor_task_tcb,
    0x01,  // Set bit 0
    NotifyAction::SetBits
);
```

---

#### `notify_from_isr()`

ISR-safe notification.

```cpp
static core::Result<core::u32, RTOSError> notify_from_isr(
    TaskControlBlock* tcb,
    core::u32 value,
    NotifyAction action
) noexcept;
```

**Example** (in ISR):
```cpp
extern "C" void EXTI_IRQHandler() {
    TaskNotification::notify_from_isr(
        task_tcb,
        0x01,
        NotifyAction::SetBits
    );
}
```

---

#### `wait()`

Wait for notification (blocking).

```cpp
static core::Result<core::u32, RTOSError> wait(
    core::u32 timeout_ms,
    NotifyClearMode clear_mode = NotifyClearMode::ClearOnExit
);
```

**Returns**: Ok(notification_value) or Err(Timeout)

**Example**:
```cpp
auto result = TaskNotification::wait(INFINITE);
if (result.is_ok()) {
    core::u32 flags = result.unwrap();
    // Handle flags
}
```

---

#### `try_wait()`

Non-blocking wait.

```cpp
static core::Result<core::u32, RTOSError> try_wait(
    NotifyClearMode clear_mode = NotifyClearMode::ClearOnExit
);
```

---

#### `clear()`

Manually clear notification.

```cpp
static core::Result<core::u32, RTOSError> clear();
```

---

#### `peek()`

Get value without clearing.

```cpp
static core::u32 peek();
```

---

#### `is_pending()`

Check if notification pending.

```cpp
static bool is_pending();
```

---

## Memory Pool

### StaticPool Template

```cpp
template <typename T, size_t Capacity>
class StaticPool;
```

**Constraints**:
- `T` must satisfy `PoolAllocatable` concept
- `Capacity`: 1-1024

---

### Member Functions

#### `allocate()`

Allocate block (O(1), lock-free).

```cpp
core::Result<T*, RTOSError> allocate() noexcept;
```

**Returns**: Ok(pointer) or Err(NoMemory)

**Example**:
```cpp
StaticPool<Message, 16> pool;

auto result = pool.allocate();
if (result.is_ok()) {
    Message* msg = result.unwrap();
    new (msg) Message();  // Placement new
    // Use msg...
    msg->~Message();
    pool.deallocate(msg);
}
```

---

#### `deallocate()`

Deallocate block (O(1), lock-free).

```cpp
core::Result<void, RTOSError> deallocate(T* ptr) noexcept;
```

**Returns**: Ok or Err(InvalidPointer)

---

#### `available()`

```cpp
size_t available() const noexcept;
```

Number of free blocks.

---

#### `capacity()` (Compile-Time)

```cpp
static consteval size_t capacity() noexcept;
```

---

#### `block_size()` (Compile-Time)

```cpp
static consteval size_t block_size() noexcept;
```

---

#### `total_size()` (Compile-Time)

```cpp
static consteval size_t total_size() noexcept;
```

---

### PoolAllocator (RAII)

```cpp
template <typename T>
class PoolAllocator;
```

**Example**:
```cpp
{
    PoolAllocator<Message> msg(pool);
    if (msg.is_valid()) {
        msg->id = 1;
        // Use msg...
    }
    // Automatic deallocation
}
```

---

## Tickless Idle

### Class

```cpp
class TicklessIdle;
```

---

### Enums

#### SleepMode

```cpp
enum class SleepMode {
    Light,    // WFI (~1-5mA, <1µs wake)
    Deep,     // STOP (~10-100µA, ~10µs wake)
    Standby   // Standby (<1µA, ~100µs wake)
};
```

---

### Static Functions

#### `enable()`

```cpp
static core::Result<void, RTOSError> enable() noexcept;
```

---

#### `disable()`

```cpp
static core::Result<void, RTOSError> disable() noexcept;
```

---

#### `configure()`

```cpp
static core::Result<void, RTOSError> configure(
    SleepMode mode,
    core::u32 min_sleep_us
) noexcept;
```

**Parameters**:
- `mode`: Sleep mode
- `min_sleep_us`: Minimum sleep duration (microseconds)

**Example**:
```cpp
TicklessIdle::enable();
TicklessIdle::configure(SleepMode::Deep, 5000);  // Min 5ms
```

---

### User Hooks

#### `tickless_enter_sleep()`

User must implement for platform.

```cpp
extern "C" void tickless_enter_sleep(SleepMode mode, core::u32 duration_us);
```

**Example** (STM32):
```cpp
extern "C" void tickless_enter_sleep(SleepMode mode, u32 duration_us) {
    RTC_SetWakeUpTimer(duration_us);

    switch (mode) {
        case SleepMode::Light:
            __WFI();
            break;
        case SleepMode::Deep:
            HAL_PWR_EnterSTOPMode(...);
            SystemClock_Config();
            break;
    }
}
```

---

## Error Handling

### RTOSError Enum

```cpp
enum class RTOSError : core::u8 {
    Timeout = 1,
    NotOwner,
    Deadlock,
    QueueFull,
    QueueEmpty,
    InvalidPriority,
    StackOverflow,
    InvalidState,
    NoMemory,
    InvalidPointer,
    TickError,
    ContextSwitchError,
    NotInitialized,
    Unknown
};
```

---

### Result<T, E>

```cpp
template <typename T, typename E>
class Result;
```

#### Member Functions

```cpp
bool is_ok() const;
bool is_err() const;

T unwrap();           // Panics on error
T unwrap_or(T default_value);
E unwrap_err();
```

---

## Concepts

### IPCMessage

```cpp
template <typename T>
concept IPCMessage = requires {
    requires std::is_trivially_copyable_v<T>;
    requires !std::is_pointer_v<T>;
    requires sizeof(T) <= 256;
};
```

---

### PoolAllocatable

```cpp
template <typename T>
concept PoolAllocatable =
    std::is_trivially_destructible_v<T> &&
    sizeof(T) <= 4096 &&
    alignof(T) <= 64;
```

---

### ValidTask

```cpp
template <typename T>
concept ValidTask =
    HasTaskMetadata<T> &&
    requires {
        requires T::stack_size() >= 256;
        requires T::stack_size() <= 65536;
        requires (T::stack_size() % 8) == 0;
    };
```

---

## Compile-Time Functions

### Validation

```cpp
// Stack validation
template <size_t StackSize>
consteval size_t validate_stack_size();

// Priority validation
template <core::u8 Pri>
consteval core::u8 validate_priority();

// Task name validation
template <size_t N>
consteval bool is_valid_task_name(const char (&str)[N]);
```

---

### Pool Utilities

```cpp
// Calculate optimal pool capacity
template <typename T, size_t Budget>
consteval size_t optimal_pool_capacity();

// Validate pool fits budget
template <typename T, size_t Capacity, size_t Budget>
consteval bool pool_fits_budget();
```

---

### Power Estimation

```cpp
// Estimate power savings
template <core::u8 IdlePercent,
          core::u32 ActiveCurrentMa,
          core::u32 SleepCurrentUa>
consteval core::u32 estimated_power_savings_uw();
```

---

## Constants

```cpp
constexpr core::u32 INFINITE = 0xFFFFFFFF;  // Infinite timeout
```

---

## Examples

See `examples/rtos/` directory for complete examples:

- `phase1_result_example.cpp` - Error handling
- `phase2_example.cpp` - Compile-time tasks
- `phase3_example.cpp` - Advanced validation
- `phase5_cpp23_example.cpp` - C++23 features
- `phase6_advanced_features.cpp` - TaskNotification, StaticPool, TicklessIdle

---

## Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Task switch | O(1) | <10µs on Cortex-M |
| Queue send/receive | O(1) | 2-5µs |
| TaskNotification | O(1) | <1µs |
| Mutex lock/unlock | O(1) | Priority inheritance |
| Semaphore give/take | O(1) | |
| StaticPool allocate | O(1) | Lock-free |
| StaticPool deallocate | O(1) | Lock-free |

---

## Memory Usage

| Component | Overhead |
|-----------|----------|
| RTOS core | ~60 bytes |
| TCB (per task) | 32 bytes |
| TaskNotification (per task) | 8 bytes |
| Queue<T, N> | N * sizeof(T) + metadata |
| Mutex | 8 bytes |
| Semaphore | 8 bytes |
| StaticPool<T, N> | N * sizeof(T) + N * sizeof(void*) |

---

## Thread Safety

- ✅ **Queue**: Thread-safe
- ✅ **Mutex**: Thread-safe (by design)
- ✅ **Semaphore**: Thread-safe
- ✅ **TaskNotification**: Thread-safe (atomic)
- ✅ **StaticPool**: Thread-safe (lock-free)
- ✅ **TicklessIdle**: ISR-safe

---

## Compatibility

- **C++ Standard**: C++23 required
- **Platforms**: Cortex-M, ESP32, x86-64 (host), ARM64
- **Compilers**: GCC 13+, Clang 16+
- **Build System**: CMake 3.20+

---

## See Also

- [Migration Guide](RTOS_MIGRATION_GUIDE.md)
- [Phase Summaries](PHASE1_COMPLETION_SUMMARY.md, etc.)
- [Examples](../examples/rtos/)
