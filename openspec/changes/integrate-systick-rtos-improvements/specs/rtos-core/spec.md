# RTOS Core with Compile-Time Type Safety Specification

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
