# Spec: Message Queues

## ADDED Requirements

### Requirement: Type-Safe Message Queue
**ID**: RTOS-QUEUE-001
**Priority**: P0 (Critical)

The system SHALL provide type-safe message queues with compile-time type checking.

#### Scenario: Send and receive typed messages
```cpp
struct SensorData {
    float temperature;
    uint32_t timestamp;
};

Queue<SensorData, 10> queue;

void producer() {
    SensorData data{25.5f, systick::micros()};
    queue.send(data);  // Type-safe
}

void consumer() {
    SensorData data = queue.receive();  // Returns typed data
    REQUIRE(data.temperature == 25.5f);
}
```

---

### Requirement: Blocking Send/Receive
**ID**: RTOS-QUEUE-002
**Priority**: P0 (Critical)

The system SHALL block tasks when queue is full (send) or empty (receive).

#### Scenario: Block on full queue
```cpp
Queue<uint32_t, 2> small_queue;

void sender() {
    small_queue.send(1);  // OK
    small_queue.send(2);  // OK, queue full now
    small_queue.send(3);  // Blocks until space available
}

void receiver() {
    RTOS::delay(100);
    uint32_t val = small_queue.receive();  // Unblocks sender
    REQUIRE(val == 1);
}
```

---

### Requirement: Timeout Support
**ID**: RTOS-QUEUE-003
**Priority**: P1 (High)

The system SHALL support timeouts for send/receive operations.

#### Scenario: Timeout on receive
```cpp
Queue<uint32_t, 10> queue;

// When receiving from empty queue with timeout
bool success = queue.try_receive(data, 100);  // 100ms timeout

// Then returns false after timeout
REQUIRE(!success);
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
