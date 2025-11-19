# Spec: RTOS Scheduler

## ADDED Requirements

### Requirement: Priority-Based Preemptive Scheduling
**ID**: RTOS-SCHED-001
**Priority**: P0 (Critical)

The system SHALL implement a priority-based preemptive scheduler with 8 priority levels (0=lowest, 7=highest).

#### Scenario: Higher priority task preempts lower priority
```cpp
// Given two tasks with different priorities
Task<512, Priority::High> high_task(high_func);
Task<512, Priority::Low> low_task(low_func);

void low_func() {
    while(1) {
        // Low priority work
        process_background();
    }
}

void high_func() {
    RTOS::delay(1000);  // Sleep 1 second
    critical_work();     // This MUST run immediately when woken
}

// When RTOS starts
RTOS::start();

// Then high_task preempts low_task immediately when waking up
// And high_task completes before low_task resumes
```

---

### Requirement: Context Switching
**ID**: RTOS-SCHED-002
**Priority**: P0 (Critical)

The system SHALL perform context switching with latency <10µs on ARM Cortex-M platforms.

#### Scenario: Fast context switch on ARM
```cpp
// Given RTOS running on STM32F4 @ 168MHz
RTOS::start();

// When context switch occurs (measured with oscilloscope)
// Then context switch latency < 10µs
// And all registers are saved/restored correctly
```

---

### Requirement: Compile-Time Task Configuration
**ID**: RTOS-SCHED-003
**Priority**: P0 (Critical)

The system SHALL define all tasks at compile time with stack size and priority validation.

#### Scenario: Task creation with compile-time validation
```cpp
// Given task with valid stack size
Task<512, Priority::High> task1(task_func);  // OK

// When task with invalid stack size
// Task<100, Priority::High> task2(task_func);  // Compile error: Stack too small

// Then static_assert fails at compile time
// And clear error message shown
```

---

### Requirement: Task Delay
**ID**: RTOS-SCHED-004
**Priority**: P0 (Critical)

The system SHALL provide accurate task delays using SysTick timer.

#### Scenario: Accurate 100ms delay
```cpp
void task_func() {
    while(1) {
        uint32_t start = alloy::systick::micros();

        RTOS::delay(100);  // 100ms delay

        uint32_t elapsed = alloy::systick::micros_since(start);

        // Then elapsed within ±5% of 100ms
        REQUIRE(elapsed >= 95000);
        REQUIRE(elapsed <= 105000);
    }
}
```

---

### Requirement: Idle Task
**ID**: RTOS-SCHED-005
**Priority**: P0 (Critical)

The system SHALL provide an idle task that runs when no other tasks are ready.

#### Scenario: Idle task runs when all tasks blocked
```cpp
Task<128, Priority::Lowest> idle(idle_func);

void idle_func() {
    while(1) {
        // Put CPU to sleep (WFI instruction)
        __WFI();
    }
}

// When all user tasks blocked
// Then idle task runs
// And CPU enters low-power mode
```

## MODIFIED Requirements
None - This is a new feature.

## REMOVED Requirements
None - This is a new feature.
