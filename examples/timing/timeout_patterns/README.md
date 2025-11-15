# Timeout Patterns - Non-Blocking Timing Example

Demonstrates professional timeout handling patterns for responsive embedded applications.

## Overview

This example shows how to implement non-blocking timeout logic using SysTick:
- **Blocking timeout with retry** - Polling with fallback
- **Non-blocking state machine** - Responsive while waiting
- **Multiple concurrent timeouts** - Managing different time windows
- **Timeout with fallback** - Graceful degradation on timeout

## Key Patterns Demonstrated

### 1. Blocking Timeout with Retry
```cpp
for (int retry = 0; retry < MAX_RETRIES; retry++) {
    u32 start = SysTickTimer::millis<board::BoardSysTick>();

    while (!operation_complete()) {
        if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, TIMEOUT_MS)) {
            break;  // Timeout - retry
        }
        // Continue waiting
    }

    if (operation_complete()) break;  // Success
}
```

### 2. Non-Blocking State Machine
```cpp
enum class State { WAITING, SUCCESS, TIMEOUT };
State state = State::WAITING;
u32 start = SysTickTimer::millis<board::BoardSysTick>();

while (state == State::WAITING) {
    if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, TIMEOUT_MS)) {
        state = State::TIMEOUT;
        break;
    }

    // Do other work (animate LED, check sensors, etc.)
    update_display();

    if (operation_complete()) {
        state = State::SUCCESS;
    }
}
```

### 3. Multiple Concurrent Timeouts
```cpp
u32 overall_timeout_start = SysTickTimer::millis<board::BoardSysTick>();
u32 periodic_action_start = overall_timeout_start;

while (!SysTickTimer::is_timeout_ms<board::BoardSysTick>(overall_timeout_start, 5000)) {
    // Check periodic action timeout
    if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(periodic_action_start, 200)) {
        do_periodic_action();
        periodic_action_start = SysTickTimer::millis<board::BoardSysTick>();
    }
}
```

### 4. Timeout with Fallback
```cpp
// Try primary method
bool success = try_primary_method(PRIMARY_TIMEOUT_MS);

if (!success) {
    // Fall back to alternative
    success = try_fallback_method(FALLBACK_TIMEOUT_MS);
}

if (!success) {
    // Enter safe/degraded mode
    enter_safe_mode();
}
```

## Build and Run

```bash
make nucleo-f401re-timeout-patterns-build
make nucleo-f401re-timeout-patterns-flash
```

## Expected LED Behavior

The example cycles through 4 timeout patterns:

1. **Retry Pattern** (3s): LED flashes 3 times with pauses, then stays on briefly
2. **Non-Blocking** (5s): LED blinks rapidly while "waiting", then 3 quick flashes
3. **Multiple Timeouts** (5s): Fast LED blink for entire duration
4. **Fallback** (6s): Various blink patterns indicating primary timeout → fallback attempt

## Learning Objectives

✅ Using `is_timeout_ms()` for non-blocking waits
✅ Implementing state machines with timeouts
✅ Managing multiple independent timeouts
✅ Retry logic and fallback patterns
✅ Responsive embedded code design

## Real-World Applications

- **Sensor reading with timeout** - Don't hang if sensor fails
- **Network requests** - Timeout and retry on no response
- **User input** - Timeout waiting for button press
- **Peripheral initialization** - Fallback if hardware doesn't respond
- **Watchdog patterns** - Periodic resets within time window

## Next Steps

- **Performance Example**: Learn to measure function execution time
- **SysTick Demo**: Explore different tick rates and configurations
