# RTOS Type-Safety with Concepts and Result<T,E> Specification

## ADDED Requirements

### Requirement: IPCMessage Concept for Queue Types

All message types used in RTOS queues SHALL satisfy the IPCMessage concept for compile-time type safety.

**Rationale**: Prevents use of unsafe types (non-trivially-copyable, too large) in IPC mechanisms.

#### Scenario: Valid message type compiles

- **GIVEN** a struct that is trivially copyable and reasonably sized
- **WHEN** used as queue message type
- **THEN** compilation SHALL succeed
- **AND** queue SHALL be instantiated without errors

```cpp
struct SensorData {
    uint32_t timestamp;
    int16_t temperature;
    int16_t humidity;
};
static_assert(std::is_trivially_copyable_v<SensorData>);

Queue<SensorData, 8> sensor_queue;  // ✓ Compiles
```

#### Scenario: Invalid message type fails at compile-time

- **GIVEN** a type with non-trivial destructor or copy constructor
- **WHEN** used as queue message type
- **THEN** compilation SHALL fail with clear error message
- **AND** message SHALL explain IPCMessage requirements

```cpp
struct BadMessage {
    std::string data;  // Non-trivial destructor
};

Queue<BadMessage, 8> bad_queue;
// Error: "Queue message type must satisfy IPCMessage concept"
// Note: BadMessage has non-trivial destructor (std::string)
// IPC messages must be trivially copyable for safe memcpy
```

#### Scenario: Message size validation

- **GIVEN** a type larger than 256 bytes
- **WHEN** used as queue message type
- **THEN** compilation SHALL fail with size limit error
- **AND** suggestion to use pointer or reference SHALL be provided

```cpp
struct LargeMessage {
    uint8_t data[512];  // Too large
};

Queue<LargeMessage, 8> large_queue;
// Error: "Message type too large (512 bytes > 256 bytes max)"
// Suggestion: Use pointer to dynamically allocated message or StaticPool
```

---

### Requirement: Producer/Consumer Concept Validation

Queues SHALL enforce compile-time validation that tasks can produce/consume messages correctly.

**Rationale**: Catches producer/consumer type mismatches before deployment.

#### Scenario: Producer task validation

- **GIVEN** a task that sends messages to a queue
- **WHEN** task type is checked against QueueProducer concept
- **THEN** compilation SHALL verify task has send_to(queue) method
- **AND** method SHALL return Result<void, RTOSError>
- **AND** mismatch SHALL fail at compile-time

```cpp
struct SensorTask {
    static void run() {
        SensorData data = read_sensor();
        sensor_queue.send(data, 1000).unwrap();
    }
};

static_assert(QueueProducer<SensorTask, decltype(sensor_queue)>,
              "SensorTask must be valid producer for sensor_queue");
```

#### Scenario: Consumer task validation

- **GIVEN** a task that receives messages from a queue
- **WHEN** task type is checked against QueueConsumer concept
- **THEN** compilation SHALL verify task has receive_from(queue) method
- **AND** method SHALL return Result<MessageType, RTOSError>
- **AND** type mismatch SHALL fail at compile-time

```cpp
struct DisplayTask {
    static void run() {
        while (1) {
            auto result = sensor_queue.receive(1000);
            if (result.is_ok()) {
                display(result.unwrap());
            }
        }
    }
};

static_assert(QueueConsumer<DisplayTask, decltype(sensor_queue)>,
              "DisplayTask must be valid consumer for sensor_queue");
```

#### Scenario: Type mismatch detection

- **GIVEN** producer sending TypeA and consumer expecting TypeB
- **WHEN** concepts are applied
- **THEN** compilation SHALL fail
- **AND** error SHALL clearly indicate type mismatch
- **AND** both types SHALL be shown in error message

```cpp
Queue<SensorData, 8> sensor_queue;
Queue<DisplayData, 8> display_queue;

// Trying to send wrong type
SensorData data = ...;
display_queue.send(data, 1000);
// Error: Cannot convert SensorData to DisplayData
// Queue<DisplayData, 8> expects DisplayData messages
```

---

### Requirement: Result<T, RTOSError> for All RTOS APIs

All RTOS blocking APIs SHALL return Result<T, RTOSError> instead of bool for consistent error handling.

**Rationale**: Provides detailed error information and integrates with HAL error handling patterns.

#### Scenario: Mutex lock returns Result

- **GIVEN** a mutex lock operation
- **WHEN** lock() is called
- **THEN** it SHALL return Result<void, RTOSError>
- **AND** Ok(void) SHALL indicate successful lock
- **AND** Err(RTOSError::Timeout) SHALL indicate timeout
- **AND** Err(RTOSError::Deadlock) SHALL indicate potential deadlock

```cpp
Mutex resource_mutex;

auto result = resource_mutex.lock(1000);  // 1 second timeout
if (result.is_ok()) {
    access_resource();
    resource_mutex.unlock();
} else {
    match (result.unwrap_err()) {
        RTOSError::Timeout => log("Lock timeout"),
        RTOSError::Deadlock => log("Potential deadlock detected"),
        _ => log("Unknown error")
    }
}
```

#### Scenario: Queue send returns Result

- **GIVEN** a queue send operation
- **WHEN** send() is called
- **THEN** it SHALL return Result<void, RTOSError>
- **AND** Ok(void) SHALL indicate message sent
- **AND** Err(RTOSError::QueueFull) SHALL indicate timeout on full queue

```cpp
Queue<SensorData, 8> queue;
SensorData data = read_sensor();

queue.send(data, 100)
    .and_then([](auto) {
        log("Message sent successfully");
        return Ok();
    })
    .or_else([](RTOSError err) {
        if (err == RTOSError::QueueFull) {
            log("Queue full - data dropped");
        }
        return Err(err);
    });
```

#### Scenario: Queue receive returns Result with value

- **GIVEN** a queue receive operation
- **WHEN** receive() is called
- **THEN** it SHALL return Result<T, RTOSError>
- **AND** Ok(T) SHALL contain received message
- **AND** Err(RTOSError::QueueEmpty) SHALL indicate timeout

```cpp
auto result = queue.receive(1000);

result.match(
    [](SensorData data) {
        process_sensor_data(data);
    },
    [](RTOSError err) {
        log_error("Receive failed", err);
    }
);
```

---

### Requirement: Monadic Error Composition

Result<T,E> SHALL support monadic operations (and_then, or_else, map) for elegant error handling chains.

**Rationale**: Enables functional error handling patterns without exceptions.

#### Scenario: and_then chains successful operations

- **GIVEN** multiple operations that can fail
- **WHEN** chained with and_then
- **THEN** execution SHALL stop at first error
- **AND** all operations SHALL only run if previous succeeded

```cpp
sensor_queue.receive(1000)
    .and_then([](SensorData data) {
        return validate_sensor_data(data);  // Result<SensorData, RTOSError>
    })
    .and_then([](SensorData validated) {
        return display_queue.send(validated, 100);
    })
    .or_else([](RTOSError err) {
        log_error("Sensor pipeline failed", err);
        return Err(err);
    });
```

#### Scenario: map transforms Ok value

- **GIVEN** Result<T, E> with Ok value
- **WHEN** map() is applied with transformation function
- **THEN** Ok value SHALL be transformed
- **AND** Err value SHALL pass through unchanged

```cpp
queue.receive(1000)
    .map([](SensorData data) {
        return data.temperature;  // Extract temperature only
    })
    .match(
        [](int16_t temp) { display_temperature(temp); },
        [](RTOSError err) { display_error(err); }
    );
```

#### Scenario: or_else handles errors

- **GIVEN** Result<T, E> with Err value
- **WHEN** or_else() is applied
- **THEN** error handler SHALL be called
- **AND** handler can recover (return Ok) or propagate (return Err)

```cpp
queue.send(data, 100)
    .or_else([](RTOSError err) {
        if (err == RTOSError::QueueFull) {
            // Retry once
            return queue.send(data, 100);
        }
        return Err(err);  // Give up
    });
```

---

### Requirement: Multi-Mutex Deadlock-Free Lock

RTOS SHALL provide ScopedMultiLock for acquiring multiple mutexes without deadlock risk.

**Rationale**: Prevents classic deadlock scenarios when multiple resources must be locked.

#### Scenario: Lock two mutexes deadlock-free

- **GIVEN** two mutexes A and B
- **WHEN** ScopedMultiLock is used
- **THEN** mutexes SHALL be locked in memory address order
- **AND** order SHALL be consistent regardless of parameter order
- **AND** RAII SHALL guarantee unlock in reverse order

```cpp
Mutex uart_mutex;
Mutex i2c_mutex;

void task1() {
    ScopedMultiLock lock(uart_mutex, i2c_mutex);
    // Locks in address order: min(addr(uart), addr(i2c)), max(...)
    use_uart_and_i2c();
    // Automatic unlock in reverse order
}

void task2() {
    ScopedMultiLock lock(i2c_mutex, uart_mutex);  // Different parameter order
    // Locks in SAME address order as task1 - no deadlock possible!
    use_uart_and_i2c();
}
```

#### Scenario: Lock three or four mutexes

- **GIVEN** 3 or 4 mutexes
- **WHEN** ScopedMultiLock is instantiated
- **THEN** all SHALL be locked in ascending address order
- **AND** variadic templates SHALL handle any count (up to 4)

```cpp
ScopedMultiLock lock(uart_mutex, i2c_mutex, spi_mutex, can_mutex);
// All locked in deterministic order
// No deadlock possible with any lock order in other threads
```

#### Scenario: Compile-time lock count validation

- **GIVEN** attempt to lock 0 mutexes
- **WHEN** ScopedMultiLock is instantiated
- **THEN** compilation SHALL fail
- **AND** error SHALL indicate at least one mutex required

```cpp
ScopedMultiLock lock();
// Error: "At least one mutex required"
```

---

### Requirement: Task Notifications (Lightweight IPC)

RTOS SHALL provide task notifications as a lightweight alternative to queues for simple signaling.

**Rationale**: Reduces memory footprint and latency for single-producer, single-consumer scenarios.

#### Scenario: Notify task from ISR

- **GIVEN** interrupt service routine needs to wake task
- **WHEN** task.notify(value) is called from ISR
- **THEN** notification SHALL be set atomically
- **AND** task SHALL wake if blocked on wait_notification()
- **AND** ISR SHALL return immediately (no blocking)

```cpp
void GPIO_IRQHandler() {
    uint32_t sensor_value = read_gpio_value();
    high_priority_task.notify(sensor_value);  // Wake task with value
}
```

#### Scenario: Wait for notification in task

- **GIVEN** task waiting for notification
- **WHEN** wait_notification(timeout) is called
- **THEN** task SHALL block until notification received or timeout
- **AND** notification value SHALL be returned in Result<u32, RTOSError>
- **AND** notification SHALL be cleared after read

```cpp
void task_func() {
    while (1) {
        auto result = current_task.wait_notification(1000);
        result.match(
            [](uint32_t value) {
                process_value(value);
            },
            [](RTOSError err) {
                if (err == RTOSError::Timeout) {
                    handle_timeout();
                }
            }
        );
    }
}
```

#### Scenario: Memory footprint comparison

- **GIVEN** task notification and Queue<uint32_t, 1>
- **WHEN** memory usage is measured
- **THEN** notification SHALL use ~8 bytes
- **AND** queue SHALL use ~20 bytes
- **AND** notification SHALL be preferred for simple cases

```cpp
// TaskNotification: 8 bytes (value + pending flag)
// Queue<uint32_t, 1>: 20 bytes (buffer + head/tail/count + wait lists)
// Savings: 60% smaller
```

---

### Requirement: Static Memory Pool

RTOS SHALL provide StaticPool<T, N> for type-safe dynamic allocation within bounded memory.

**Rationale**: Enables safe object allocation without heap fragmentation or unbounded memory usage.

#### Scenario: Allocate object from pool

- **GIVEN** StaticPool<SensorData, 16> pool
- **WHEN** allocate() is called
- **THEN** pointer to new object SHALL be returned in Result<T*, RTOSError>
- **AND** construction SHALL happen in-place
- **AND** pool SHALL track allocation

```cpp
StaticPool<SensorData, 16> sensor_pool;

auto result = sensor_pool.allocate(timestamp, temp, humidity);
if (result.is_ok()) {
    SensorData* data = result.unwrap();
    process(data);
    sensor_pool.deallocate(data);
}
```

#### Scenario: Pool exhaustion

- **GIVEN** pool with all blocks allocated
- **WHEN** allocate() is called
- **THEN** Result SHALL be Err(RTOSError::NoMemory)
- **AND** no allocation SHALL occur
- **AND** pool state SHALL be unchanged

```cpp
for (int i = 0; i < 20; i++) {  // Pool capacity is 16
    auto result = pool.allocate(i);
    if (result.is_err()) {
        assert(i == 16);  // Failed at 17th allocation
        assert(result.unwrap_err() == RTOSError::NoMemory);
    }
}
```

#### Scenario: Pointer validation on deallocate

- **GIVEN** pointer not from pool
- **WHEN** deallocate(ptr) is called
- **THEN** Result SHALL be Err(RTOSError::InvalidPointer)
- **AND** no deallocation SHALL occur
- **AND** pool SHALL remain valid

```cpp
SensorData external_data;
auto result = pool.deallocate(&external_data);
assert(result.is_err());
assert(result.unwrap_err() == RTOSError::InvalidPointer);
```

---

### Requirement: Backward Compatibility for Result<T,E>

RTOS SHALL provide helper methods to ease migration from bool returns to Result<T,E>.

**Rationale**: Smooth transition for existing codebases with deprecation path.

#### Scenario: unwrap_or() for bool-like behavior

- **GIVEN** code expecting bool return
- **WHEN** unwrap_or(false) is used
- **THEN** Ok → true, Err → false SHALL be returned
- **AND** deprecation warning SHALL suggest proper Result handling

```cpp
// Old code pattern
if (mutex.lock()) {  // Deprecated
    // ...
}

// Temporary migration (discouraged)
if (mutex.lock().unwrap_or(false)) {  // Works but loses error info
    // ...
}

// Preferred new code
if (mutex.lock().is_ok()) {
    // ...
}
```

#### Scenario: Migration guide examples

- **GIVEN** migration guide documentation
- **WHEN** developer reads bool→Result section
- **THEN** guide SHALL show before/after examples
- **AND** guide SHALL explain benefits of Result
- **AND** guide SHALL show common patterns (match, and_then, or_else)
