# Tasks: Add Alloy RTOS

## Phase 1: Core Scheduler (Days 1-4)

### Task 1.1: Create RTOS Core Interface
**Estimated**: 4 hours
**Dependencies**: `add-systick-timer-hal` (must be complete)
**Deliverable**: `src/rtos/rtos.hpp`

Create main RTOS interface:
- [x] Task<StackSize, Priority> class template
- [x] TaskControlBlock struct
- [x] TaskState enum
- [x] Priority enum (8 levels)
- [x] RTOS::start(), delay(), yield() functions
- [x] Concept definitions for type safety
- [x] Documentation

**Files**: New `src/rtos/rtos.hpp` ✅ COMPLETE

---

### Task 1.2: Implement Ready Queue
**Estimated**: 3 hours
**Dependencies**: Task 1.1
**Deliverable**: `src/rtos/scheduler.hpp`

Implement O(1) priority-based ready queue:
- [x] Priority bitmap (uint8_t)
- [x] Array of task lists per priority
- [x] get_highest_priority() using CLZ
- [x] make_ready() / make_not_ready()
- [ ] Unit tests for ready queue (deferred)

**Files**: New `src/rtos/scheduler.hpp` ✅ COMPLETE

---

### Task 1.3: ARM Cortex-M Context Switch (STM32F1/F4)
**Estimated**: 6 hours
**Dependencies**: Task 1.2
**Deliverable**: `src/rtos/platform/arm_context.hpp`

Implement PendSV-based context switching:
- [x] PendSV_Handler in assembly
- [x] save_context() / restore_context()
- [x] Initialize task stack frames
- [x] SysTick_Handler integration
- [ ] Test context switch on STM32F4 (requires hardware)

**Files**: New `src/rtos/platform/arm_context.hpp` ✅ COMPLETE

---

### Task 1.4: Scheduler Core Logic
**Estimated**: 4 hours
**Dependencies**: Task 1.3
**Deliverable**: Updated `src/rtos/scheduler.hpp`

Implement scheduler algorithm:
- [x] RTOS::tick() called from SysTick
- [x] wake_delayed_tasks()
- [x] Select next task
- [x] Trigger context switch if needed
- [x] RTOS::start() - initialize and run
- [x] RTOS::delay(ms) - task delay
- [x] RTOS::yield() - cooperative yield

**Files**: Modified `src/rtos/scheduler.hpp` ✅ COMPLETE

---

### Task 1.5: ESP32 Context Switch (Xtensa)
**Estimated**: 8 hours
**Dependencies**: Task 1.4 (can parallelize with 1.3)
**Deliverable**: `src/rtos/platform/xtensa_context.hpp`

Implement Xtensa context switching:
- [x] Save/restore all Xtensa registers (a0-a15, etc.)
- [x] Software interrupt for context switch
- [x] Timer ISR integration (esp_timer)
- [ ] Test on ESP32 (requires hardware)

**Files**: New `src/rtos/platform/xtensa_context.hpp` ✅ COMPLETE

---

### Task 1.6: Integrate RTOS into All Boards
**Estimated**: 3 hours
**Dependencies**: Tasks 1.3, 1.5
**Deliverable**: Updated board files

Add RTOS support to all 5 boards:
- [ ] Include RTOS headers
- [ ] Initialize RTOS after SysTick
- [ ] Document RTOS availability

**Files**:
- Modified: `boards/stm32f103c8/board.hpp`
- Modified: `boards/stm32f407vg/board.hpp`
- Modified: `boards/esp32_devkit/board.hpp`
- Modified: `boards/raspberry_pi_pico/board.hpp`
- Modified: `boards/arduino_zero/board.hpp`

---

## Phase 2: Message Queues (Days 5-6)

### Task 2.1: Implement Message Queue
**Estimated**: 5 hours
**Dependencies**: Phase 1
**Deliverable**: `src/rtos/queue.hpp`

Implement type-safe message queue:
- [ ] Queue<T, Capacity> template
- [ ] Circular buffer implementation
- [ ] send() / receive() with blocking
- [ ] try_send() / try_receive() non-blocking
- [ ] Timeout support
- [ ] Task blocking/unblocking
- [ ] Unit tests

**Files**: New `src/rtos/queue.hpp`

---

### Task 2.2: Queue Example Application
**Estimated**: 2 hours
**Dependencies**: Task 2.1
**Deliverable**: `examples/rtos_queue/main.cpp`

Create queue demo:
- [ ] Producer task (sends sensor data)
- [ ] Consumer task (receives and displays)
- [ ] Demonstrate blocking behavior
- [ ] Show timeout handling

**Files**: New `examples/rtos_queue/`

---

## Phase 3: Semaphores & Mutexes (Days 7-9)

### Task 3.1: Implement Binary Semaphore
**Estimated**: 3 hours
**Dependencies**: Phase 1
**Deliverable**: `src/rtos/semaphore.hpp`

- [ ] BinarySemaphore class
- [ ] give() / take() with blocking
- [ ] ISR-safe give()
- [ ] Timeout support
- [ ] Unit tests

**Files**: New `src/rtos/semaphore.hpp`

---

### Task 3.2: Implement Counting Semaphore
**Estimated**: 2 hours
**Dependencies**: Task 3.1
**Deliverable**: Updated `src/rtos/semaphore.hpp`

- [ ] CountingSemaphore class
- [ ] Resource pool management
- [ ] Test with buffer pool example

**Files**: Modified `src/rtos/semaphore.hpp`

---

### Task 3.3: Implement Mutex with Priority Inheritance
**Estimated**: 6 hours
**Dependencies**: Phase 1
**Deliverable**: `src/rtos/mutex.hpp`

Implement priority-inheriting mutex:
- [ ] Mutex class
- [ ] lock() / unlock()
- [ ] Priority inheritance algorithm
- [ ] Priority restoration
- [ ] LockGuard RAII wrapper
- [ ] Test priority inversion prevention

**Files**: New `src/rtos/mutex.hpp`

---

### Task 3.4: Semaphore/Mutex Examples
**Estimated**: 3 hours
**Dependencies**: Tasks 3.1-3.3
**Deliverable**: Example applications

Create demos:
- [ ] ISR-to-task signaling (semaphore)
- [ ] Resource pool management (counting semaphore)
- [ ] Shared SPI bus (mutex)
- [ ] Priority inheritance demo

**Files**: New `examples/rtos_sync/`

---

## Phase 4: Event Flags (Day 10)

### Task 4.1: Implement Event Flags
**Estimated**: 4 hours
**Dependencies**: Phase 1
**Deliverable**: `src/rtos/events.hpp`

- [ ] EventFlags class (32-bit mask)
- [ ] set() / clear() operations
- [ ] wait_any() / wait_all()
- [ ] Timeout support
- [ ] ISR-safe set()
- [ ] Unit tests

**Files**: New `src/rtos/events.hpp`

---

### Task 4.2: Event Flags Example
**Estimated**: 2 hours
**Dependencies**: Task 4.1
**Deliverable**: `examples/rtos_events/main.cpp`

- [ ] Multi-source event handling
- [ ] Startup synchronization demo
- [ ] Wait for multiple conditions

**Files**: New `examples/rtos_events/`

---

## Phase 5: Testing & Validation (Days 11-13)

### Task 5.1: Unit Tests
**Estimated**: 8 hours
**Dependencies**: All implementation tasks
**Deliverable**: `tests/rtos/`

Create comprehensive unit tests:
- [ ] Scheduler tests (priority, context switch)
- [ ] Queue tests (type safety, blocking, overflow)
- [ ] Semaphore tests (binary, counting)
- [ ] Mutex tests (priority inheritance)
- [ ] Event tests (wait_any, wait_all)
- [ ] Mock timer for deterministic tests

**Files**: New `tests/rtos/*_test.cpp`

---

### Task 5.2: Integration Tests on Real Hardware
**Estimated**: 12 hours
**Dependencies**: All implementation
**Deliverable**: Validation report

Test on all 5 platforms:
- [ ] STM32F1: Verify context switch, measure latency
- [ ] STM32F4: Verify priority scheduling, stress test
- [ ] ESP32: Verify Xtensa context switch
- [ ] RP2040: Full feature test
- [ ] SAMD21: Memory footprint check
- [ ] Run for extended periods (>1 hour)
- [ ] Verify no stack overflows
- [ ] Measure context switch latency (<10µs)
- [ ] Document results

**Files**: New `docs/RTOS_VALIDATION.md`

---

### Task 5.3: Stress Testing
**Estimated**: 6 hours
**Dependencies**: Task 5.2
**Deliverable**: Stress test results

- [ ] 10+ tasks with different priorities
- [ ] Heavy queue traffic (1000+ msgs/sec)
- [ ] Mutex contention (multiple tasks competing)
- [ ] Priority inversion scenarios
- [ ] Long-running stability (24+ hours)
- [ ] Memory leak detection

**Files**: New `examples/rtos_stress_test/`

---

## Phase 6: Documentation & Examples (Days 14-15)

### Task 6.1: API Documentation
**Estimated**: 6 hours
**Dependencies**: All implementation
**Deliverable**: `docs/rtos/API.md`

Write comprehensive docs:
- [ ] Task creation and management
- [ ] Scheduler behavior
- [ ] Queue API with examples
- [ ] Semaphore API with examples
- [ ] Mutex API with examples
- [ ] Event flags API with examples
- [ ] Best practices
- [ ] Common pitfalls

**Files**: New `docs/rtos/API.md`

---

### Task 6.2: Tutorial & Getting Started
**Estimated**: 4 hours
**Dependencies**: Task 6.1
**Deliverable**: `docs/rtos/TUTORIAL.md`

Create beginner-friendly tutorial:
- [ ] First RTOS application (blink with tasks)
- [ ] Producer-consumer with queues
- [ ] ISR signaling with semaphores
- [ ] Shared resource with mutex
- [ ] Event-driven architecture
- [ ] Debugging tips
- [ ] Performance tuning

**Files**: New `docs/rtos/TUTORIAL.md`

---

### Task 6.3: Architecture Document
**Estimated**: 3 hours
**Dependencies**: All tasks
**Deliverable**: `docs/rtos/ARCHITECTURE.md`

Document internals:
- [ ] Scheduler algorithm
- [ ] Context switch mechanism
- [ ] IPC implementation details
- [ ] Memory layout
- [ ] Priority inheritance algorithm
- [ ] Platform-specific notes

**Files**: New `docs/rtos/ARCHITECTURE.md`

---

### Task 6.4: Complete Example Applications
**Estimated**: 6 hours
**Dependencies**: All implementation
**Deliverable**: Real-world examples

Create production-quality examples:
- [ ] Data logger (sensors + SD card + display)
- [ ] Motor controller (PID + UART commands)
- [ ] Multi-sensor dashboard (I2C + SPI + UART)
- [ ] IoT device (WiFi + MQTT + sensors)

**Files**: New `examples/rtos_*_complete/`

---

## Summary

**Total Estimated Time**: ~100 hours (12-15 working days, ~2.5-3 weeks)

**Phases**:
1. Core Scheduler (Days 1-4): 28 hours
2. Message Queues (Days 5-6): 7 hours
3. Semaphores & Mutexes (Days 7-9): 14 hours
4. Event Flags (Day 10): 6 hours
5. Testing & Validation (Days 11-13): 26 hours
6. Documentation & Examples (Days 14-15): 19 hours

**Critical Path**:
1. SysTick Timer HAL (prerequisite, from previous spec)
2. Scheduler Core → Context Switch → IPC → Testing → Docs

**Parallelization Opportunities**:
- ARM and Xtensa context switch (Tasks 1.3 and 1.5)
- IPC mechanisms (Queues, Semaphores, Mutexes, Events) can be developed in parallel after scheduler
- Examples can be written in parallel with testing

**Risk Mitigation**:
- Start with simplest platform (STM32F4) to validate design
- Implement and test scheduler before IPC
- Unit test each component before integration
- Use oscilloscope to measure context switch latency
- Run stress tests for extended periods

**Deliverables Checklist**:
- [ ] Core scheduler for 5 platforms
- [ ] Message queues (type-safe)
- [ ] Binary and counting semaphores
- [ ] Mutexes with priority inheritance
- [ ] Event flags (32-bit)
- [ ] 10+ example applications
- [ ] Comprehensive unit tests
- [ ] Hardware validation report
- [ ] Complete documentation (API, Tutorial, Architecture)
- [ ] Build system integration
