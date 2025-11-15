# Timeout Patterns Example

## Overview

This example demonstrates **real-world timeout patterns** for embedded systems. Learn how to implement robust, non-blocking timeouts for communication, sensor polling, and multi-operation tracking.

## Features

- ✅ **Blocking timeout with retry** - Classic polling with fallback
- ✅ **Non-blocking timeout** - State machine pattern for concurrent operations
- ✅ **Multiple concurrent timeouts** - Track independent operations
- ✅ **Production-ready patterns** - Copy-paste into your projects

## Hardware Requirements

- Any supported board
- On-board LED
- USB connection for console output (optional)

## Patterns Demonstrated

### Pattern 1: Blocking Timeout with Retry

**Use case**: Polling unreliable hardware (I2C sensor, UART response, SPI transaction)

```cpp
bool wait_for_sensor_with_retry(Sensor& sensor, u32 timeout_ms, u32 max_retries) {
    for (u32 retry = 0; retry < max_retries; retry++) {
        u32 start = SysTickTimer::millis<board::BoardSysTick>();

        while (true) {
            if (sensor.is_ready()) return true;

            if (SysTickTimer::is_timeout_ms<board::BoardSysTick>(start, timeout_ms)) {
                break;  // Try next retry
            }

            delay_ms(10);  // Avoid busy-wait
        }
    }
    return false;  // All retries exhausted
}
```

**When to use**:
- ✅ Simple polling scenarios
- ✅ Infrequent operations
- ✅ Can afford to block

**Trade-offs**:
- ⚠️ Blocks CPU (can't do other work)
- ⚠️ Fixed timeout (not adaptive)
- ✅ Easy to understand and debug

---

### Pattern 2: Non-Blocking Timeout (State Machine)

**Use case**: Background operations while waiting (sensor + display + communication)

```cpp
enum class State { Idle, Waiting, Processing, Error };

void state_machine_tick() {
    static State state = State::Idle;
    static u32 timeout_start = 0;

    switch (state) {
        case State::Idle:
            timeout_start = millis();
            state = State::Waiting;
            break;

        case State::Waiting:
            if (sensor.is_ready()) {
                state = State::Processing;
            } else if (is_timeout_ms(timeout_start, 1000)) {
                state = State::Error;
            } else {
                do_other_work();  // Non-blocking!
            }
            break;

        // ... other states
    }
}
```

**When to use**:
- ✅ Need to do multiple things concurrently
- ✅ Long timeouts (>100ms)
- ✅ Real-time responsiveness required

**Trade-offs**:
- ✅ Allows concurrent work
- ⚠️ More complex (state machine)
- ⚠️ Requires periodic calling

---

### Pattern 3: Multiple Concurrent Timeouts

**Use case**: Communication protocol with multiple transaction types (command + data)

```cpp
// Track multiple independent operations
u32 cmd_timeout_start = millis();
u32 data_timeout_start = millis();
bool cmd_complete = false;
bool data_complete = false;

while (!cmd_complete || !data_complete) {
    // Check command timeout (50ms)
    if (!cmd_complete && is_timeout_ms(cmd_timeout_start, 50)) {
        handle_cmd_timeout();
        cmd_complete = true;
    }

    // Check data timeout (200ms)
    if (!data_complete && is_timeout_ms(data_timeout_start, 200)) {
        handle_data_timeout();
        data_complete = true;
    }

    // Poll operations...
}
```

**When to use**:
- ✅ Multiple operations with different timeouts
- ✅ Pipelined communication
- ✅ Complex protocols

---

## Expected Output

```
╔══════════════════════════════════════════════════════╗
║   Timeout Patterns Example - Non-Blocking Timeouts  ║
╚══════════════════════════════════════════════════════╝

Board: Nucleo-F401RE
System Clock: 84 MHz

--- Pattern 1: Blocking Timeout with Retry ---
Timeout: 100 ms, Max retries: 3
  Attempt 1/3... TIMEOUT
  Attempt 2/3... SUCCESS (after 15 attempts)
  Result: SUCCESS

--- Pattern 2: Non-Blocking Timeout ---
Demonstrates state machine pattern with timeout.
Starting non-blocking operation...
  [State: Idle] Starting sensor poll...
  [State: WaitingForSensor] (doing other work: 100 iterations)
  [State: WaitingForSensor] (doing other work: 200 iterations)
  [State: WaitingForSensor] Sensor ready! Processing...
  [State: ProcessingData] Data processed successfully!
Non-blocking demo complete. Work iterations: 2547

--- Pattern 3: Multiple Concurrent Timeouts ---
Tracking two operations with different timeouts.
Operation 1: 50ms timeout
Operation 2: 200ms timeout

  [10 ms] Operation 1: waiting... (10/50 ms)
  [10 ms] Operation 2: waiting... (10/200 ms)
  [20 ms] Operation 1: waiting... (20/50 ms)
  [20 ms] Operation 2: waiting... (20/200 ms)
  [50 ms] Operation 1: TIMEOUT (expected ~50ms)
  [50 ms] Operation 2: waiting... (50/200 ms)
  [200 ms] Operation 2: TIMEOUT (expected ~200ms)

  ✓ Both operations completed (or timed out).

╔══════════════════════════════════════════════════════╗
║              All patterns demonstrated!             ║
╚══════════════════════════════════════════════════════╝
```

## Building

```bash
# Configure
cmake -B build -DALLOY_BOARD=nucleo_f401re

# Build
cmake --build build --target timeout_patterns

# Flash
cmake --build build --target flash_timeout_patterns
```

## Real-World Applications

### I2C Sensor Polling
```cpp
// Pattern 1: Retry with timeout
bool read_temperature(float& temp) {
    for (int retry = 0; retry < 3; retry++) {
        if (i2c_start_conversion()) {
            u32 start = millis();
            while (!i2c_conversion_ready()) {
                if (is_timeout_ms(start, 100)) break;
                delay_ms(5);
            }
            if (i2c_conversion_ready()) {
                temp = i2c_read_result();
                return true;
            }
        }
        delay_ms(50);  // Before retry
    }
    return false;
}
```

### UART Communication
```cpp
// Pattern 3: Multiple timeouts
bool uart_send_receive(const u8* tx_data, u8* rx_data, size_t len) {
    u32 tx_start = millis();
    u32 rx_start = 0;
    bool tx_done = false;
    bool rx_done = false;

    while (!tx_done || !rx_done) {
        // TX with 100ms timeout
        if (!tx_done) {
            if (uart_tx_ready()) {
                uart_write(tx_data, len);
                tx_done = true;
                rx_start = millis();  // Start RX timer
            } else if (is_timeout_ms(tx_start, 100)) {
                return false;
            }
        }

        // RX with 500ms timeout
        if (tx_done && !rx_done) {
            if (uart_rx_available() >= len) {
                uart_read(rx_data, len);
                rx_done = true;
            } else if (is_timeout_ms(rx_start, 500)) {
                return false;
            }
        }
    }

    return true;
}
```

### State Machine with Timeout
```cpp
// Pattern 2: Non-blocking
enum class CommState { Idle, Sending, WaitingAck, Done, Error };

void communication_task() {
    static CommState state = CommState::Idle;
    static u32 timeout_start = 0;

    switch (state) {
        case CommState::Sending:
            if (uart_tx_complete()) {
                timeout_start = millis();
                state = CommState::WaitingAck;
            }
            break;

        case CommState::WaitingAck:
            if (received_ack()) {
                state = CommState::Done;
            } else if (is_timeout_ms(timeout_start, 1000)) {
                state = CommState::Error;
            }
            break;

        // ... handle other states
    }
}
```

## Performance Tips

### Minimize Polling Overhead
```cpp
// ❌ Bad: Busy-wait (100% CPU)
while (!sensor.is_ready()) {
    // Burning CPU cycles!
}

// ✅ Good: Small delay between polls
while (!sensor.is_ready()) {
    delay_ms(10);  // Reduces CPU usage to ~1%
}
```

### Choose Appropriate Timeout Values
```cpp
// I2C typically completes in <10ms, timeout at 100ms (10x safety margin)
const u32 I2C_TIMEOUT_MS = 100;

// UART response typically <50ms, timeout at 500ms
const u32 UART_TIMEOUT_MS = 500;

// Flash erase can take seconds
const u32 FLASH_ERASE_TIMEOUT_MS = 5000;
```

## Common Pitfalls

### ❌ Using equality check instead of elapsed time
```cpp
// WRONG: Can miss timeout if interrupt occurs
if (millis() == timeout_time) {  // Fragile!
    handle_timeout();
}

// CORRECT: Use elapsed time
if (is_timeout_ms(start_time, TIMEOUT_MS)) {
    handle_timeout();
}
```

### ❌ Not handling wraparound
```cpp
// WRONG: Fails after millis() wraps (49 days on 32-bit)
if (millis() - start_time > TIMEOUT_MS) {  // Bug when wrapping!
    handle_timeout();
}

// CORRECT: Use provided helper (handles wraparound)
if (is_timeout_ms(start_time, TIMEOUT_MS)) {
    handle_timeout();
}
```

### ❌ Forgetting to reset timeout on retry
```cpp
// WRONG: Timeout never resets
u32 start = millis();
for (int retry = 0; retry < 3; retry++) {
    if (is_timeout_ms(start, 100)) {  // First retry always times out!
        break;
    }
}

// CORRECT: Reset timeout each retry
for (int retry = 0; retry < 3; retry++) {
    u32 start = millis();  // New timeout for each retry
    // ...
}
```

## Learning Objectives

After studying this example:

1. ✅ Understand when to use blocking vs non-blocking timeouts
2. ✅ Know how to implement retry logic correctly
3. ✅ Can build state machines with timeout handling
4. ✅ Can track multiple concurrent operations
5. ✅ Avoid common timeout pitfalls

## Next Steps

- **performance**: Learn to measure execution time
- **systick_demo**: Understand tick resolution trade-offs
- Apply these patterns in your own projects!

## References

- [Timeout Patterns in Embedded Systems](https://interrupt.memfault.com/blog/cortex-m-rtos-context-switching)
- [State Machine Design](https://barrgroup.com/embedded-systems/how-to/state-machines)
