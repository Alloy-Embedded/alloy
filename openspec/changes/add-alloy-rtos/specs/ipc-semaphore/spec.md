# Spec: Semaphores

## ADDED Requirements

### Requirement: Binary Semaphore
**ID**: RTOS-SEM-001
**Priority**: P0 (Critical)

The system SHALL provide binary semaphores for task synchronization.

#### Scenario: Signal from ISR to task
```cpp
BinarySemaphore data_ready;

void uart_isr() {
    process_rx_data();
    data_ready.give();  // Signal task from ISR
}

void uart_task() {
    while(1) {
        data_ready.take();  // Wait for signal
        handle_uart_data();
    }
}
```

---

### Requirement: Counting Semaphore
**ID**: RTOS-SEM-002
**Priority**: P0 (Critical)

The system SHALL provide counting semaphores for resource management.

#### Scenario: Manage pool of buffers
```cpp
CountingSemaphore buffers(5, 5);  // 5 buffers available

void allocate_buffer() {
    buffers.take();  // Blocks if no buffers available
    Buffer* buf = get_buffer();
    use_buffer(buf);
    free_buffer(buf);
    buffers.give();  // Return buffer to pool
}
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
