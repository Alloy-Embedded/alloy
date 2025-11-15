# RTOS Core with Compile-Time Type Safety Specification

## Overview

This spec defines the core RTOS functionality with maximum compile-time validation using C++23 features. The design prioritizes:
- **Zero-overhead abstractions** - All compile-time features have zero runtime cost
- **Guaranteed compile-time** - Using `consteval` to enforce compile-time evaluation
- **Type safety** - C++20 concepts for validation
- **Minimal RAM footprint** - TCB reduced from 32 bytes to 28 bytes using `fixed_string`

### Key Improvements Over Current Implementation

| Feature | Current (C++20) | Proposed (C++23) | Benefit |
|---------|----------------|------------------|---------|
| **Task Registration** | Constructor (runtime) | `TaskSet<>` variadic template | Compile-time validation |
| **RAM Calculation** | Manual | `consteval` guaranteed | Must be compile-time |
| **Task Names** | `const char*` (4 bytes RAM) | `fixed_string` (0 bytes RAM) | 4 bytes saved per task |
| **TCB Size** | 32 bytes | 28 bytes | 12.5% smaller |
| **Error Handling** | `bool` | `Result<T,E>` | Consistent with HAL |

## ADDED Requirements

### Requirement: Variadic Task Registration with TaskSet

The RTOS SHALL support compile-time task registration using variadic templates, allowing all tasks to be defined as types and registered in a single TaskSet.

**Rationale**: Enables compile-time validation of task configuration and total memory footprint calculation.

#### Scenario: Define tasks as types

- **GIVEN** an application requiring 3 tasks
- **WHEN** tasks are defined using type aliases
- **THEN** each task SHALL be a unique type with stack size, priority, and name
- **AND** tasks SHALL be usable as template parameters

```cpp
using SensorTask = Task<512, Priority::High, "Sensor">;
using DisplayTask = Task<512, Priority::Normal, "Display">;
using LogTask = Task<256, Priority::Low, "Log">;
```

#### Scenario: Register all tasks with TaskSet

- **GIVEN** task types defined
- **WHEN** TaskSet is instantiated with all task types
- **THEN** TaskSet SHALL validate all task configurations at compile-time
- **AND** total RAM SHALL be calculated automatically
- **AND** compilation SHALL fail if any validation fails

```cpp
using AllTasks = TaskSet<SensorTask, DisplayTask, LogTask>;
// Total RAM: 1,376 bytes (calculated at compile-time)
```

#### Scenario: Start RTOS with type-safe entry point

- **GIVEN** TaskSet defined with all application tasks
- **WHEN** RTOS::start<TaskSet>() is called
- **THEN** scheduler SHALL initialize all tasks
- **AND** highest priority task SHALL run first
- **AND** function SHALL never return

```cpp
int main() {
    board::init();
    RTOS::start<AllTasks>();  // Type-safe, never returns
}
```

---

### Requirement: Compile-Time Task Validation

TaskSet SHALL validate all task configurations at compile-time using static_assert with clear error messages.

**Rationale**: Catches configuration errors before deployment, eliminating entire class of runtime bugs.

#### Scenario: Stack size validation

- **GIVEN** a task with stack size < 256 bytes
- **WHEN** TaskSet is instantiated
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate minimum stack size
- **AND** suggested fix SHALL be provided

```cpp
using BadTask = Task<128, Priority::Normal, "Bad">;  // 128 < 256
using Tasks = TaskSet<BadTask>;
// Error: "Stack size must be at least 256 bytes"
```

#### Scenario: Stack alignment validation

- **GIVEN** a task with stack size not 8-byte aligned
- **WHEN** TaskSet is instantiated
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate alignment requirement

```cpp
using MisalignedTask = Task<513, Priority::Normal, "Misaligned">;  // Not 8-byte aligned
using Tasks = TaskSet<MisalignedTask>;
// Error: "Stack size must be 8-byte aligned (multiple of 8)"
```

#### Scenario: Priority range validation

- **GIVEN** a task with priority outside 0-7 range
- **WHEN** TaskSet is instantiated
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate valid priority range

```cpp
using InvalidPriTask = Task<512, static_cast<Priority>(9), "Invalid">;
using Tasks = TaskSet<InvalidPriTask>;
// Error: "Priority must be between Idle (0) and Critical (7)"
```

#### Scenario: Total RAM budget validation

- **GIVEN** tasks whose total RAM exceeds MAX_RAM_FOR_RTOS
- **WHEN** TaskSet is instantiated
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL show total RAM and maximum allowed
- **AND** per-task breakdown SHALL be provided

```cpp
static_assert(TaskSet<Task1, Task2, Task3, Task4, Task5>::total_ram <= MAX_RAM,
              "Total task RAM (2048 bytes) exceeds available memory (1536 bytes)");
```

#### Scenario: Minimum task count validation

- **GIVEN** TaskSet with zero tasks
- **WHEN** TaskSet is instantiated
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL indicate at least one task required

---

### Requirement: Optional Priority Uniqueness Check

TaskSet SHALL optionally enforce unique priorities when STRICT_MODE is enabled at compile-time.

**Rationale**: Some applications require strict priority ordering, others need flexibility for round-robin.

#### Scenario: Unique priorities in strict mode

- **GIVEN** STRICT_MODE enabled
- **WHEN** two tasks have the same priority
- **THEN** compilation SHALL fail with static_assert
- **AND** error message SHALL list conflicting tasks

```cpp
#define ALLOY_RTOS_STRICT_MODE
using Task1 = Task<512, Priority::Normal, "Task1">;
using Task2 = Task<512, Priority::Normal, "Task2">;  // Same priority
using Tasks = TaskSet<Task1, Task2>;
// Error: "Tasks 'Task1' and 'Task2' have same priority in strict mode"
```

#### Scenario: Shared priorities allowed in normal mode

- **GIVEN** STRICT_MODE disabled
- **WHEN** two tasks have the same priority
- **THEN** compilation SHALL succeed
- **AND** tasks SHALL use FIFO ordering at same priority

```cpp
// No STRICT_MODE
using Task1 = Task<512, Priority::Normal, "Task1">;
using Task2 = Task<512, Priority::Normal, "Task2">;
using Tasks = TaskSet<Task1, Task2>;  // Compiles OK
```

---

### Requirement: Compile-Time Memory Footprint Calculation

TaskSet SHALL calculate total RAM footprint at compile-time, including all TCBs, stacks, and RTOS overhead.

**Rationale**: Enables static memory analysis and prevents deployment on systems with insufficient RAM.

#### Scenario: Calculate total RAM for tasks

- **GIVEN** TaskSet with N tasks
- **WHEN** total_ram constant is evaluated
- **THEN** it SHALL equal sum of (stack_size + sizeof(TCB)) for all tasks
- **AND** calculation SHALL be constexpr (compile-time only)

```cpp
using Tasks = TaskSet<
    Task<512, Priority::High, "T1">,
    Task<512, Priority::Normal, "T2">,
    Task<256, Priority::Low, "T3">
>;

static_assert(Tasks::total_ram == (512+32) + (512+32) + (256+32),
              "RAM calculation mismatch");
// Total: 1,376 bytes
```

#### Scenario: Include RTOS core overhead

- **GIVEN** TaskSet instantiation
- **WHEN** total_system_ram is calculated
- **THEN** it SHALL include task RAM + RTOS core overhead
- **AND** RTOS core overhead SHALL be documented (scheduler + queues)

```cpp
static constexpr size_t RTOS_CORE_OVERHEAD = 60;  // Scheduler + state
static constexpr size_t total_system_ram =
    Tasks::total_ram + RTOS_CORE_OVERHEAD;
```

#### Scenario: Report memory breakdown

- **GIVEN** TaskSet with multiple tasks
- **WHEN** compiling with verbose mode
- **THEN** compiler SHALL output per-task memory usage
- **AND** total RAM SHALL be displayed
- **AND** available RAM SHALL be shown

```
Task 'Sensor': 544 bytes (512 stack + 32 TCB)
Task 'Display': 544 bytes (512 stack + 32 TCB)
Task 'Log': 288 bytes (256 stack + 32 TCB)
Total task RAM: 1,376 bytes
RTOS core: 60 bytes
Grand total: 1,436 bytes
Available: 6,144 bytes (RAM size: 8KB)
```

---

### Requirement: Backward Compatibility Shim

The RTOS SHALL support the old Task constructor API for one release cycle with deprecation warnings.

**Rationale**: Provides smooth migration path for existing projects.

#### Scenario: Old API still works with warning

- **GIVEN** existing code using Task constructors
- **WHEN** code is compiled
- **THEN** it SHALL compile successfully
- **AND** deprecation warning SHALL be emitted
- **AND** warning SHALL reference migration guide

```cpp
// Old API (deprecated)
Task<512, Priority::High> task1(func1, "Task1");
// Warning: "Task constructor deprecated. Use TaskSet registration.
//           See migration guide: docs/rtos_migration.md"

RTOS::start();  // Old start (also deprecated)
```

#### Scenario: New API preferred

- **GIVEN** migration to new API
- **WHEN** using TaskSet registration
- **THEN** compilation SHALL succeed with zero warnings
- **AND** code SHALL use type-safe entry point

```cpp
// New API (recommended)
using Task1 = Task<512, Priority::High, "Task1">;
using AllTasks = TaskSet<Task1>;
RTOS::start<AllTasks>();  // No warnings
```

#### Scenario: Migration script converts code

- **GIVEN** project using old Task constructor API
- **WHEN** migration script is executed
- **THEN** script SHALL convert to TaskSet registration
- **AND** script SHALL update RTOS::start() calls
- **AND** script SHALL preserve task names and priorities
- **AND** converted code SHALL compile without warnings

```bash
./scripts/migrate_rtos_api.py src/
# Converted 5 tasks to TaskSet registration
# Updated 1 RTOS::start() call
# Success: Project migrated to new API
```

---

### Requirement: Task Name String Literal

Tasks SHALL use compile-time string literals for names, enabling better debug output and traceability.

**Rationale**: Allows task names in stack traces, logs, and debugger without runtime overhead.

#### Scenario: Task name as template parameter

- **GIVEN** task defined with string literal name
- **WHEN** task is created
- **THEN** name SHALL be stored as compile-time constant
- **AND** name SHALL be accessible via Task::name
- **AND** no RAM SHALL be used for name storage (stored in .rodata)

```cpp
using SensorTask = Task<512, Priority::High, "Sensor">;
static_assert(SensorTask::name == "Sensor");  // Compile-time check
```

#### Scenario: Task name in debug output

- **GIVEN** task running in debugger
- **WHEN** task list is displayed
- **THEN** task name SHALL appear in debugger
- **AND** stack trace SHALL show task name
- **AND** no runtime string manipulation SHALL occur

```
Current tasks:
  [High]   Sensor   (RUNNING)
  [Normal] Display  (READY)
  [Low]    Log      (BLOCKED on mutex)
```

#### Scenario: Task name in error messages

- **GIVEN** task encounters error (e.g., stack overflow)
- **WHEN** error is logged
- **THEN** task name SHALL be included in error message
- **AND** message SHALL help identify problematic task

```
ERROR: Stack overflow detected in task 'Sensor'
  Priority: High (7)
  Stack: 512 bytes
  Usage: 516 bytes (overflow!)
```

---

### Requirement: C++23 consteval for Guaranteed Compile-Time RAM Calculation

TaskSet SHALL use `consteval` to guarantee that total RAM calculation happens at compile-time with zero runtime overhead.

**Rationale**: `constexpr` can sometimes be evaluated at runtime. `consteval` forces compile-time evaluation.

#### Scenario: RAM calculation must be compile-time

- **GIVEN** TaskSet with multiple tasks
- **WHEN** total_ram is accessed
- **THEN** calculation SHALL be done at compile-time (consteval)
- **AND** compiler SHALL error if runtime evaluation attempted
- **AND** binary SHALL contain zero calculation code

```cpp
template <typename... Tasks>
class TaskSet {
    // GUARANTEED compile-time (compiler error if not possible)
    static consteval size_t calculate_total_ram() {
        return ((Tasks::stack_size + sizeof(TaskControlBlock)) + ...);
    }

    static constexpr size_t total_ram = calculate_total_ram();

    // This WILL compile (consteval succeeds):
    static_assert(total_ram <= 8192, "RAM budget exceeded");
};
```

#### Scenario: Compile error if not compile-time evaluable

- **GIVEN** attempt to use runtime value in consteval function
- **WHEN** code is compiled
- **THEN** compiler SHALL produce error
- **AND** error SHALL indicate consteval requirement

```cpp
void runtime_function(size_t stack_size) {
    // This FAILS to compile:
    // consteval auto ram = calculate_total_ram();  // Error: non-constant expression
}
```

---

### Requirement: C++23 fixed_string for Zero-RAM Task Names

Tasks SHALL use `fixed_string` template parameter for names, storing them in .rodata instead of RAM.

**Rationale**: Task names should be available for debugging without consuming RAM.

#### Scenario: Task name stored in .rodata

- **GIVEN** task defined with string literal name
- **WHEN** task is created
- **THEN** name SHALL be stored in .rodata section (not .data/.bss)
- **AND** no RAM SHALL be consumed for name storage
- **AND** name SHALL be accessible via Task::name constexpr

```cpp
template <size_t N>
struct fixed_string {
    char data[N];
    static constexpr size_t size = N - 1;

    consteval fixed_string(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }

    constexpr operator const char*() const { return data; }
};

// Task with compile-time name (zero RAM cost)
template <size_t StackSize, Priority Pri, fixed_string Name>
class Task {
    static constexpr const char* name = Name;  // In .rodata
    // No name member variable needed!
};

// Usage:
using SensorTask = Task<512, Priority::High, "Sensor">;
static_assert(SensorTask::name == "Sensor");  // Compile-time check
```

#### Scenario: TCB size reduced by 4 bytes

- **GIVEN** TaskControlBlock structure
- **WHEN** using fixed_string for names
- **THEN** TCB SHALL NOT contain name pointer
- **AND** TCB size SHALL be 28 bytes (down from 32 bytes)
- **AND** name SHALL still be accessible for debugging

```cpp
// Before (C++20): 32 bytes
struct TaskControlBlock {
    void* stack_pointer;     // 4 bytes
    void* stack_base;        // 4 bytes
    u32 stack_size;          // 4 bytes
    u8 priority;             // 1 byte
    TaskState state;         // 1 byte
    u32 wake_time;           // 4 bytes
    const char* name;        // 4 bytes ← REMOVED
    TaskControlBlock* next;  // 4 bytes
    // Total: 32 bytes
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

#### Scenario: RAM savings for multi-task system

- **GIVEN** system with 8 tasks
- **WHEN** using fixed_string names
- **THEN** 32 bytes total RAM saved (4 bytes × 8 tasks)
- **AND** task names still available in debugger
- **AND** stack traces still show task names

```
// RAM savings calculation:
// - 8 tasks × 4 bytes per name pointer = 32 bytes saved
// - Name strings moved from .data to .rodata (no RAM cost)
// - Total: 32 bytes RAM saved for 8-task system
```

---

### Requirement: C++23 if consteval for Dual-Mode Functions

RTOS SHALL provide functions that work both at compile-time and runtime using `if consteval`.

**Rationale**: Some operations (like stack usage calculation) are useful both at compile-time (estimation) and runtime (actual measurement).

#### Scenario: Stack usage function works at compile-time and runtime

- **GIVEN** function to calculate stack usage
- **WHEN** called at compile-time
- **THEN** SHALL use static analysis estimation
- **WHEN** called at runtime
- **THEN** SHALL use actual stack pointer measurement

```cpp
constexpr u32 calculate_stack_usage(const TaskControlBlock& tcb) {
    if consteval {
        // Compile-time path: estimate max usage
        return estimate_max_stack_usage<decltype(tcb)>();
    } else {
        // Runtime path: measure actual usage
        u8* sp = static_cast<u8*>(tcb.stack_pointer);
        u8* base = static_cast<u8*>(tcb.stack_base);
        return static_cast<u32>(base + tcb.stack_size - sp);
    }
}

// Compile-time usage:
constexpr u32 predicted = calculate_stack_usage(sensor_task.get_tcb());

// Runtime usage:
u32 actual = calculate_stack_usage(sensor_task.get_tcb());
```

#### Scenario: Compiler optimizes correctly

- **GIVEN** if consteval function
- **WHEN** called at compile-time
- **THEN** only compile-time branch SHALL be compiled
- **AND** runtime branch SHALL be eliminated
- **WHEN** called at runtime
- **THEN** only runtime branch SHALL be in binary
- **AND** compile-time branch SHALL be eliminated

---

### Requirement: Zero Runtime Overhead for Task Registration

TaskSet SHALL have zero runtime overhead compared to manual task registration, with all validation at compile-time.

**Rationale**: Maintains performance characteristics while adding type safety.

#### Scenario: Identical assembly output

- **GIVEN** TaskSet registration and manual registration
- **WHEN** both are compiled with optimizations
- **THEN** generated assembly SHALL be identical
- **AND** no additional instructions SHALL be added
- **AND** binary size SHALL be unchanged

```asm
; Manual registration (old)
call register_task
call register_task
call register_task

; TaskSet registration (new)
call register_task
call register_task
call register_task
; Identical! Zero overhead.
```

#### Scenario: Compile-time constant folding

- **GIVEN** TaskSet with compile-time known configuration
- **WHEN** compiler optimizes
- **THEN** total RAM calculation SHALL be constant-folded
- **AND** validation SHALL be eliminated (static_assert only)
- **AND** no runtime checks SHALL exist in binary

#### Scenario: Startup time unchanged

- **GIVEN** application using TaskSet
- **WHEN** RTOS::start() is called
- **THEN** startup time SHALL be identical to manual registration
- **AND** no additional initialization SHALL occur
- **AND** first task SHALL run at same time
