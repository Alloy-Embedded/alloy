# Alloy RTOS Migration Guide

This guide helps you migrate from traditional RTOS patterns to Alloy RTOS with C++23 enhancements.

---

## Table of Contents

1. [Overview](#overview)
2. [Error Handling Migration](#error-handling-migration)
3. [Task Creation Migration](#task-creation-migration)
4. [Queue Migration](#queue-migration)
5. [Mutex Migration](#mutex-migration)
6. [Semaphore Migration](#semaphore-migration)
7. [Notifications vs Queues](#notifications-vs-queues)
8. [Memory Allocation Migration](#memory-allocation-migration)
9. [Power Management](#power-management)
10. [Performance Tips](#performance-tips)

---

## Overview

### What Changed

Alloy RTOS introduced several major improvements across 6 phases:

| Phase | Feature | Migration Impact |
|-------|---------|------------------|
| **1** | Result<T,E> | **HIGH** - All APIs changed |
| **2** | Compile-time tasks | **MEDIUM** - New API, old still works |
| **3** | Advanced concepts | **LOW** - Optional validation |
| **4** | Unified SysTick | **LOW** - Transparent |
| **5** | C++23 enhancements | **LOW** - Better errors only |
| **6** | Advanced features | **LOW** - New features, optional |

### Migration Strategy

**Recommended approach**: Incremental migration

1. ✅ Start with error handling (Phase 1)
2. ✅ Migrate to compile-time tasks (Phase 2) - optional
3. ✅ Add validation (Phase 3) - optional
4. ✅ Use new features (Phase 6) where beneficial

---

## Error Handling Migration

### Before (Boolean Returns)

```cpp
// Old API - boolean error handling
bool success = mutex.lock();
if (!success) {
    // Error, but what kind?
    return false;
}

// Do work...
mutex.unlock();
```

**Problems**:
- ❌ No error information (why did it fail?)
- ❌ Easy to ignore errors
- ❌ Inconsistent with HAL layer

### After (Result<T,E>)

```cpp
// New API - type-safe error handling
auto result = mutex.lock();
if (result.is_err()) {
    RTOSError error = result.unwrap_err();
    // Handle specific error
    switch (error) {
        case RTOSError::Timeout:
            // Handle timeout
            break;
        case RTOSError::Deadlock:
            // Handle deadlock
            break;
        default:
            // Other error
            break;
    }
    return core::Err(error);
}

// Do work...
mutex.unlock();
```

**Benefits**:
- ✅ Specific error information
- ✅ Type-safe (compiler enforces checking)
- ✅ Consistent with HAL layer
- ✅ Better error propagation

### Quick Migration Pattern

```cpp
// Pattern 1: Ignore error (not recommended)
mutex.lock().unwrap();  // Panics on error

// Pattern 2: Check error
if (auto result = mutex.lock(); result.is_ok()) {
    // Success
} else {
    // Handle error
}

// Pattern 3: Propagate error
auto result = mutex.lock();
if (result.is_err()) {
    return result;  // Propagate error up
}

// Pattern 4: Early return with ?-like macro
#define TRY(expr) \
    ({ auto _result = (expr); \
       if (_result.is_err()) return _result; \
       _result.unwrap(); })

// Usage
TRY(mutex.lock());
// Work...
TRY(mutex.unlock());
```

---

## Task Creation Migration

### Before (Runtime Names)

```cpp
// Old API - runtime string (uses RAM)
Task<512, Priority::High> sensor_task(sensor_func, "Sensor");
```

**Problems**:
- ❌ Task name stored in RAM (wastes memory)
- ❌ No compile-time validation
- ❌ Can't calculate RAM usage at compile-time

### After (Compile-Time Names)

```cpp
// New API - compile-time string (zero RAM)
Task<512, Priority::High, "Sensor"> sensor_task(sensor_func);
```

**Benefits**:
- ✅ Zero RAM for task names (stored in .rodata)
- ✅ Compile-time validation
- ✅ Compile-time RAM calculation
- ✅ Type-safe

### Backward Compatibility

The old API still works (deprecated):

```cpp
// Still works, but shows deprecation warning
Task<512, Priority::High> sensor_task(sensor_func, "Sensor");
```

### TaskSet Validation (New)

```cpp
// Group tasks for compile-time validation
Task<512, Priority::High, "Sensor"> sensor_task(sensor_func);
Task<1024, Priority::Normal, "Display"> display_task(display_func);
Task<256, Priority::Low, "Logger"> logger_task(logger_func);

using MyTasks = TaskSet<
    decltype(sensor_task),
    decltype(display_task),
    decltype(logger_task)
>;

// Compile-time validation
static_assert(MyTasks::total_ram() == 1888);  // 1792 stack + 96 TCB
static_assert(MyTasks::validate());
static_assert(MyTasks::all_tasks_valid());

// RAM budget check (C++23)
static_assert(MyTasks::total_ram_with_budget<4096>() == 1888);
```

---

## Queue Migration

### Before (Output Parameters)

```cpp
// Old API - output parameter
Queue<SensorData, 8> queue;

SensorData data;
bool success = queue.receive(data, 1000);  // 1s timeout
if (success) {
    // Use data
}
```

**Problems**:
- ❌ Output parameter (not idiomatic C++)
- ❌ No error information
- ❌ Can't return directly

### After (Direct Return)

```cpp
// New API - direct return with Result<>
Queue<SensorData, 8> queue;

auto result = queue.receive(1000);  // 1s timeout
if (result.is_ok()) {
    SensorData data = result.unwrap();
    // Use data
} else {
    RTOSError error = result.unwrap_err();
    if (error == RTOSError::Timeout) {
        // Handle timeout
    }
}
```

**Benefits**:
- ✅ Idiomatic C++ (direct return)
- ✅ Specific error information
- ✅ Type-safe
- ✅ More concise

### Send Migration

```cpp
// Before
bool success = queue.send(data, 1000);

// After
auto result = queue.send(data, 1000);
if (result.is_err()) {
    // Handle error (QueueFull, Timeout, etc.)
}
```

---

## Mutex Migration

### Before

```cpp
// Old API
Mutex mutex;

if (mutex.lock()) {
    // Critical section
    mutex.unlock();
}
```

### After

```cpp
// New API
Mutex mutex;

if (auto result = mutex.lock(); result.is_ok()) {
    // Critical section
    mutex.unlock();
}
```

### RAII Pattern (Recommended)

```cpp
// Use LockGuard for automatic unlock
{
    LockGuard guard(mutex);
    if (guard.locked()) {
        // Critical section
    }
    // Automatically unlocked here
}
```

---

## Semaphore Migration

### Before

```cpp
// Old API
BinarySemaphore sem;

if (sem.take()) {
    // Do work
    sem.give();
}
```

### After

```cpp
// New API
BinarySemaphore sem;

if (auto result = sem.take(); result.is_ok()) {
    // Do work
    sem.give();
}
```

### Counting Semaphore

```cpp
CountingSemaphore sem(5);  // Max count = 5

// Producer
sem.give().unwrap();

// Consumer
if (auto result = sem.take(1000); result.is_ok()) {
    // Resource acquired
}
```

---

## Notifications vs Queues

### When to Use Each

**Use TaskNotification when:**
- ✅ Single consumer (one task)
- ✅ Simple events or flags
- ✅ ISR → Task communication
- ✅ Performance critical (<1µs)
- ✅ Memory constrained (8 bytes)

**Use Queue when:**
- ✅ Multiple consumers
- ✅ Large data (>4 bytes)
- ✅ Message ordering important
- ✅ Buffering needed

### Migration: Queue → Notification

**Before (Queue for simple event)**:

```cpp
Queue<uint32_t, 4> event_queue;

// ISR
extern "C" void EXTI_IRQHandler() {
    event_queue.try_send(0x01);  // 32+ bytes overhead
}

// Task
void sensor_task() {
    auto result = event_queue.receive(INFINITE);
    if (result.is_ok()) {
        uint32_t event = result.unwrap();
        // Handle event
    }
}
```

**After (Notification)**:

```cpp
TaskControlBlock* sensor_task_tcb;

// ISR
extern "C" void EXTI_IRQHandler() {
    TaskNotification::notify_from_isr(
        sensor_task_tcb,
        0x01,  // Event flag
        NotifyAction::SetBits
    );  // Only 8 bytes overhead
}

// Task
void sensor_task() {
    auto result = TaskNotification::wait(INFINITE);
    if (result.is_ok()) {
        uint32_t flags = result.unwrap();
        // Handle flags
    }
}
```

**Savings**: 4x less memory, 10x faster

### Notification Modes

```cpp
// Event flags (OR bits)
TaskNotification::notify(task, 0x01, NotifyAction::SetBits);
TaskNotification::notify(task, 0x02, NotifyAction::SetBits);
// Result: value = 0x03

// Counting semaphore (increment)
TaskNotification::notify(task, 1, NotifyAction::Increment);
TaskNotification::notify(task, 1, NotifyAction::Increment);
// Result: value = 2

// Simple data (overwrite)
TaskNotification::notify(task, 0x1234, NotifyAction::Overwrite);
TaskNotification::notify(task, 0x5678, NotifyAction::Overwrite);
// Result: value = 0x5678 (last wins)

// Overflow detection
auto result = TaskNotification::notify(task, 0x99, NotifyAction::OverwriteIfEmpty);
if (result.is_err()) {
    // Notification already pending
}
```

---

## Memory Allocation Migration

### Before (malloc/free or static arrays)

```cpp
// Option 1: malloc/free (not recommended in embedded)
Message* msg = (Message*)malloc(sizeof(Message));
if (msg) {
    // Use msg
    free(msg);
}

// Option 2: Static array (wastes memory)
Message message_buffer[16];  // Always allocated
```

**Problems**:
- ❌ malloc: fragmentation, non-deterministic
- ❌ Static: wastes memory when not used
- ❌ No RAII

### After (StaticPool)

```cpp
// Fixed-size pool (O(1), no fragmentation)
StaticPool<Message, 16> message_pool;

// Manual allocation
auto result = message_pool.allocate();
if (result.is_ok()) {
    Message* msg = result.unwrap();
    new (msg) Message();  // Placement new

    // Use msg...

    msg->~Message();  // Manual destructor
    message_pool.deallocate(msg);
}

// RAII (recommended)
{
    PoolAllocator<Message> msg(message_pool);
    if (msg.is_valid()) {
        msg->id = 1;
        // Use msg...
    }
    // Automatically deallocated here
}
```

**Benefits**:
- ✅ O(1) allocation (vs malloc's O(log n))
- ✅ Zero fragmentation
- ✅ Lock-free thread-safe
- ✅ RAII available
- ✅ Compile-time budgeting

### Pool Sizing

```cpp
// Calculate optimal capacity for budget
constexpr size_t capacity = optimal_pool_capacity<Message, 1024>();
StaticPool<Message, capacity> pool;

// Validate pool fits budget
static_assert(pool_fits_budget<Message, 16, 2048>());
```

---

## Power Management

### Before (No power management)

```cpp
// Idle task just loops
void idle_task() {
    while (1) {
        // Burn power doing nothing
    }
}
```

### After (TicklessIdle)

```cpp
// Enable tickless idle
int main() {
    // Configure tickless idle
    TicklessIdle::enable();
    TicklessIdle::configure(SleepMode::Deep, 5000);  // Min 5ms sleep

    // Start RTOS (will automatically sleep when idle)
    RTOS::start();
}

// Platform-specific hook (user implements)
extern "C" void tickless_enter_sleep(SleepMode mode, uint32_t duration_us) {
    // Configure wake-up timer
    RTC_SetWakeUpTimer(duration_us);

    // Enter sleep mode
    switch (mode) {
        case SleepMode::Light:
            __WFI();  // Wait for interrupt
            break;
        case SleepMode::Deep:
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            SystemClock_Config();  // Restore clocks
            break;
        case SleepMode::Standby:
            HAL_PWR_EnterSTANDBYMode();
            break;
    }
}
```

**Power Savings**: Up to 80% reduction

---

## Performance Tips

### 1. Use Notifications for Simple Events

```cpp
// ❌ Slow: Queue for simple event
Queue<uint32_t, 4> queue;

// ✅ Fast: Notification
TaskNotification::notify(task, event, NotifyAction::SetBits);
```

**Speedup**: 10x faster

### 2. Use StaticPool for Allocations

```cpp
// ❌ Slow: malloc/free
Message* msg = (Message*)malloc(sizeof(Message));

// ✅ Fast: StaticPool
auto result = pool.allocate();
```

**Speedup**: Deterministic O(1)

### 3. Use LockGuard for Mutexes

```cpp
// ❌ Error-prone: Manual unlock
mutex.lock();
// ... might forget unlock or throw exception
mutex.unlock();

// ✅ Safe: Automatic unlock
{
    LockGuard guard(mutex);
    // Automatically unlocked on scope exit
}
```

### 4. Use Compile-Time Validation

```cpp
// ✅ Catch errors at compile-time
using MyTasks = TaskSet<...>;
static_assert(MyTasks::total_ram_with_budget<8192>());
static_assert(MyTasks::validate_advanced());
```

### 5. Enable TicklessIdle for Battery

```cpp
// ✅ Save power when idle
TicklessIdle::enable();
TicklessIdle::configure(SleepMode::Deep, 1000);
```

**Savings**: Up to 80% power

---

## Common Migration Patterns

### Pattern 1: Error Handling

```cpp
// Before
bool result = operation();
if (!result) {
    return false;
}

// After
auto result = operation();
if (result.is_err()) {
    return result;  // Propagate error
}
```

### Pattern 2: Queue Operations

```cpp
// Before
Data data;
if (queue.receive(data, timeout)) {
    // Use data
}

// After
if (auto result = queue.receive(timeout); result.is_ok()) {
    Data data = result.unwrap();
    // Use data
}
```

### Pattern 3: Task Creation

```cpp
// Before
Task<512, Priority::High> task(func, "Task");

// After
Task<512, Priority::High, "Task"> task(func);
```

### Pattern 4: Memory Allocation

```cpp
// Before
Message* msg = (Message*)malloc(sizeof(Message));
// ... use ...
free(msg);

// After
PoolAllocator<Message> msg(pool);
if (msg.is_valid()) {
    // Use msg...
}  // Automatic deallocation
```

---

## Checklist

Use this checklist to track your migration:

### Phase 1: Error Handling
- [ ] Replace all boolean returns with Result<T,E>
- [ ] Handle errors properly (not just unwrap())
- [ ] Update error handling in all tasks
- [ ] Test error paths

### Phase 2: Compile-Time Tasks
- [ ] Migrate task names to compile-time strings
- [ ] Create TaskSet for validation
- [ ] Verify RAM calculations

### Phase 3: Advanced Validation (Optional)
- [ ] Add static_asserts for validation
- [ ] Check for priority inversion
- [ ] Validate lock ordering

### Phase 6: New Features (Optional)
- [ ] Replace queues with notifications where appropriate
- [ ] Migrate malloc/free to StaticPool
- [ ] Enable TicklessIdle for power savings

### Testing
- [ ] Test on all target boards
- [ ] Verify performance improvements
- [ ] Measure power consumption
- [ ] Run stress tests

---

## Need Help?

- **Documentation**: See `docs/` directory
- **Examples**: See `examples/rtos/`
- **Issues**: File on GitHub
- **Questions**: Check RTOS_API_REFERENCE.md

---

## Summary

**Key Changes**:
1. ✅ Error handling: `bool` → `Result<T,E>`
2. ✅ Task names: Runtime string → Compile-time string
3. ✅ Queue API: Output parameter → Direct return
4. ✅ New features: TaskNotification, StaticPool, TicklessIdle

**Benefits**:
- ✅ Type-safe error handling
- ✅ Zero-RAM task names
- ✅ Compile-time validation
- ✅ 10x faster notifications
- ✅ O(1) memory allocation
- ✅ 80% power savings

**Migration Effort**:
- Phase 1 (Error handling): **High** - Required for all code
- Phase 2-3 (Validation): **Low** - Optional improvements
- Phase 6 (Features): **Low** - Optional new features

Recommended timeline: 1-2 weeks for complete migration
