# Design: Alloy RTOS

## Architecture Overview

### Component Architecture
```
┌─────────────────────────────────────────────────────────────┐
│                    User Application                          │
│                                                               │
│  void task1_func() {                                         │
│      while(1) {                                              │
│          Message msg = queue.receive();                      │
│          process(msg);                                       │
│      }                                                        │
│  }                                                            │
│                                                               │
│  Task<512, Priority::High> task1(task1_func);               │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│              src/rtos/rtos.hpp (Main API)                    │
│  - Task<StackSize, Priority> class template                 │
│  - RTOS::start() - begins scheduling                         │
│  - RTOS::delay(ms) - task delay                              │
│  - RTOS::yield() - cooperative yield                         │
└─────────────────┬───────────────────────────────────────────┘
                  │
        ┌─────────┴─────────┬──────────────┬──────────────┐
        │                   │              │              │
┌───────▼────────┐  ┌───────▼──────┐  ┌───▼────┐  ┌─────▼──────┐
│   Scheduler    │  │   IPC        │  │ Memory │  │  Platform  │
│   scheduler.   │  │   queue.hpp  │  │ Static │  │  ARM       │
│   hpp          │  │   semaphore. │  │ pools  │  │  context.  │
│                │  │   hpp        │  │        │  │  hpp       │
│  - Ready queue │  │   mutex.hpp  │  │        │  │  Xtensa    │
│  - Context     │  │   event.hpp  │  │        │  │  context.  │
│    switch      │  │              │  │        │  │  hpp       │
│  - Tick ISR    │  │  Type-safe   │  │        │  │            │
└────────────────┘  └──────────────┘  └────────┘  └────────────┘
```

## 1. Scheduler Design

### 1.1 Task Control Block (TCB)
```cpp
struct TaskControlBlock {
    void* stack_pointer;          // Current SP (updated on context switch)
    void* stack_base;             // Stack bottom (for overflow detection)
    uint32_t stack_size;          // Stack size in bytes
    uint8_t priority;             // 0-7 (7 = highest)
    TaskState state;              // Ready, Running, Blocked, Suspended
    uint32_t wake_time;           // For delayed tasks (micros)
    const char* name;             // For debugging

    // Linked list for same-priority tasks (future: round-robin)
    TaskControlBlock* next;
};

enum class TaskState : uint8_t {
    Ready,      // Ready to run
    Running,    // Currently executing
    Blocked,    // Waiting on IPC
    Suspended,  // Manually suspended
    Delayed     // Sleeping until wake_time
};
```

### 1.2 Ready Queue (Priority Bitmap)
**Concept**: Use bitmap to find highest priority task in O(1)

```cpp
class ReadyQueue {
private:
    // Bitmap: bit N set = tasks at priority N are ready
    uint8_t priority_bitmap_;

    // Array of task lists, one per priority level
    TaskControlBlock* ready_lists_[8];

public:
    // O(1) - Find highest priority with __builtin_clz or similar
    TaskControlBlock* get_highest_priority() {
        if (priority_bitmap_ == 0) {
            return &idle_task;  // No tasks ready, run idle
        }

        // Find highest set bit (ARM: CLZ instruction)
        uint8_t highest = 7 - __builtin_clz(priority_bitmap_);
        return ready_lists_[highest];
    }

    // O(1) - Add task to ready queue
    void make_ready(TaskControlBlock* task) {
        // Set bit for this priority
        priority_bitmap_ |= (1 << task->priority);

        // Add to linked list for this priority
        task->next = ready_lists_[task->priority];
        ready_lists_[task->priority] = task;
    }

    // O(1) - Remove task from ready queue
    void make_not_ready(TaskControlBlock* task) {
        // Remove from linked list
        // ... (omitted for brevity)

        // If no more tasks at this priority, clear bit
        if (ready_lists_[task->priority] == nullptr) {
            priority_bitmap_ &= ~(1 << task->priority);
        }
    }
};
```

**Performance**:
- `get_highest_priority()`: 1-2 cycles (CLZ instruction on ARM)
- `make_ready()`: ~5-10 cycles
- `make_not_ready()`: ~10-20 cycles

### 1.3 Context Switch Mechanism

#### ARM Cortex-M (STM32F1, STM32F4, SAMD21, RP2040)
**Use PendSV for context switching** (industry standard)

```cpp
// In SysTick_Handler (runs every 1ms)
void SysTick_Handler() {
    systick_counter++;

    // Trigger scheduler
    RTOS::tick();

    // If context switch needed, trigger PendSV
    if (RTOS::need_context_switch()) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

// PendSV_Handler performs actual context switch
__attribute__((naked))
void PendSV_Handler() {
    asm volatile (
        // Save context of current task
        "mrs r0, psp               \n"  // Get process stack pointer
        "stmdb r0!, {r4-r11}       \n"  // Push r4-r11 onto task stack
        "bl save_current_sp        \n"  // Save SP to TCB

        // Load context of next task
        "bl get_next_task_sp       \n"  // Get SP of next task
        "ldmia r0!, {r4-r11}       \n"  // Pop r4-r11 from new task stack
        "msr psp, r0               \n"  // Set process stack pointer

        "bx lr                     \n"  // Return (hardware restores r0-r3, r12, lr, pc, xpsr)
    );
}
```

**Context saved by hardware** (on exception entry/exit):
- r0-r3, r12, lr, pc, xPSR (8 registers)

**Context saved by software** (in PendSV):
- r4-r11 (8 registers)

**Total**: 16 registers = 64 bytes per task

**Context switch time**: ~5-10µs @ 72MHz

#### ESP32 (Xtensa)
**Use level-1 interrupt for context switch**

```cpp
// In timer ISR (1ms tick)
void IRAM_ATTR timer_isr(void* arg) {
    systick_counter++;

    RTOS::tick();

    if (RTOS::need_context_switch()) {
        // Trigger software interrupt for context switch
        xt_set_intset(1 << CONTEXT_SWITCH_INTR);
    }
}

// Context switch ISR (Xtensa assembly)
void context_switch_isr() {
    // Save all Xtensa registers (a0-a15, PC, PS, SAR, etc.)
    // ... ~40 registers on Xtensa

    // Switch to next task
    // Restore registers
}
```

**Context switch time**: ~10-20µs @ 240MHz (Xtensa has more registers)

### 1.4 Scheduler Algorithm

```cpp
void RTOS::tick() {
    // 1. Increment system tick counter
    tick_counter++;

    // 2. Wake up delayed tasks
    wake_delayed_tasks(systick::micros());

    // 3. Find highest priority ready task
    TaskControlBlock* next = ready_queue.get_highest_priority();

    // 4. If different from current, request context switch
    if (next != current_task) {
        current_task->state = TaskState::Ready;
        next->state = TaskState::Running;
        current_task = next;
        need_context_switch_ = true;
    }
}

void wake_delayed_tasks(uint32_t current_time) {
    // Iterate through delayed tasks
    for (auto* task : delayed_tasks) {
        if (systick::is_timeout(task->wake_time, current_time)) {
            task->state = TaskState::Ready;
            ready_queue.make_ready(task);
        }
    }
}
```

### 1.5 Task Creation (Compile-Time)

```cpp
template<size_t StackSize, Priority Pri>
class Task {
    static_assert(StackSize >= 256, "Stack too small");
    static_assert(StackSize % 8 == 0, "Stack must be 8-byte aligned");

private:
    alignas(8) uint8_t stack_[StackSize];
    TaskControlBlock tcb_;

public:
    constexpr Task(void (*task_func)(), const char* name = "task")
        : stack_{}, tcb_{}
    {
        // Initialize TCB
        tcb_.stack_base = &stack_[0];
        tcb_.stack_size = StackSize;
        tcb_.priority = static_cast<uint8_t>(Pri);
        tcb_.state = TaskState::Ready;
        tcb_.name = name;

        // Initialize stack with context
        init_stack(task_func);

        // Register with RTOS
        RTOS::register_task(&tcb_);
    }

private:
    void init_stack(void (*task_func)()) {
        // Platform-specific stack initialization
        // ARM: Fill stack as if exception occurred
        uint32_t* sp = reinterpret_cast<uint32_t*>(&stack_[StackSize]);

        *(--sp) = 0x01000000;  // xPSR (Thumb bit set)
        *(--sp) = reinterpret_cast<uint32_t>(task_func);  // PC
        *(--sp) = 0xFFFFFFFD;  // LR (return to thread mode)
        *(--sp) = 0;  // R12
        *(--sp) = 0;  // R3
        *(--sp) = 0;  // R2
        *(--sp) = 0;  // R1
        *(--sp) = 0;  // R0

        // Software-saved registers
        *(--sp) = 0;  // R11
        *(--sp) = 0;  // R10
        *(--sp) = 0;  // R9
        *(--sp) = 0;  // R8
        *(--sp) = 0;  // R7
        *(--sp) = 0;  // R6
        *(--sp) = 0;  // R5
        *(--sp) = 0;  // R4

        tcb_.stack_pointer = sp;
    }
};
```

**Usage**:
```cpp
void sensor_task() {
    while (1) {
        read_sensor();
        RTOS::delay(100);  // 100ms
    }
}

// Compile-time task creation
Task<512, Priority::High> sensor(sensor_task, "Sensor");
Task<256, Priority::Normal> led(led_task, "LED");
Task<128, Priority::Low> idle(idle_task, "Idle");

int main() {
    Board::initialize();
    RTOS::start();  // Never returns
}
```

## 2. Message Queue Design

### 2.1 Type-Safe Queue
```cpp
template<typename T, size_t Capacity>
class Queue {
    static_assert(std::is_trivially_copyable_v<T>,
                  "Queue messages must be trivially copyable");

private:
    T buffer_[Capacity];
    volatile size_t head_{0};
    volatile size_t tail_{0};
    volatile size_t count_{0};

    TaskControlBlock* blocked_senders_{nullptr};
    TaskControlBlock* blocked_receivers_{nullptr};

public:
    // Send message (blocks if full)
    bool send(const T& message, uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while (count_ >= Capacity) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return false;  // Timeout
            }

            // Block current task
            RTOS::block_current_task(&blocked_senders_);
        }

        // Critical section
        disable_interrupts();
        buffer_[tail_] = message;  // Copy message
        tail_ = (tail_ + 1) % Capacity;
        count_++;
        enable_interrupts();

        // Wake up blocked receivers
        RTOS::unblock_tasks(&blocked_receivers_);

        return true;
    }

    // Receive message (blocks if empty)
    T receive(uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while (count_ == 0) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return T{};  // Timeout, return default
            }

            RTOS::block_current_task(&blocked_receivers_);
        }

        // Critical section
        disable_interrupts();
        T message = buffer_[head_];  // Copy message
        head_ = (head_ + 1) % Capacity;
        count_--;
        enable_interrupts();

        // Wake up blocked senders
        RTOS::unblock_tasks(&blocked_senders_);

        return message;
    }

    // Non-blocking variants
    bool try_send(const T& message) {
        return send(message, 0);
    }

    bool try_receive(T& message) {
        if (count_ == 0) return false;
        message = receive(0);
        return true;
    }
};
```

**Usage**:
```cpp
struct SensorData {
    float temperature;
    float humidity;
};

Queue<SensorData, 10> sensor_queue;

void sensor_task() {
    while (1) {
        SensorData data = read_sensor();
        sensor_queue.send(data);  // Blocks if full
        RTOS::delay(1000);
    }
}

void display_task() {
    while (1) {
        SensorData data = sensor_queue.receive();  // Blocks if empty
        display.show(data);
    }
}
```

## 3. Semaphore Design

### 3.1 Binary Semaphore (Signaling)
```cpp
class BinarySemaphore {
private:
    volatile bool available_{false};
    TaskControlBlock* blocked_tasks_{nullptr};

public:
    // Give semaphore (signal)
    void give() {
        disable_interrupts();
        available_ = true;
        enable_interrupts();

        // Wake up one blocked task
        RTOS::unblock_one_task(&blocked_tasks_);
    }

    // Take semaphore (wait)
    bool take(uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while (!available_) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return false;
            }

            RTOS::block_current_task(&blocked_tasks_);
        }

        disable_interrupts();
        available_ = false;
        enable_interrupts();

        return true;
    }
};
```

### 3.2 Counting Semaphore (Resource Pool)
```cpp
class CountingSemaphore {
private:
    volatile uint32_t count_;
    const uint32_t max_count_;
    TaskControlBlock* blocked_tasks_{nullptr};

public:
    constexpr CountingSemaphore(uint32_t initial, uint32_t max)
        : count_(initial), max_count_(max) {}

    void give() {
        disable_interrupts();
        if (count_ < max_count_) {
            count_++;
        }
        enable_interrupts();

        RTOS::unblock_one_task(&blocked_tasks_);
    }

    bool take(uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while (count_ == 0) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return false;
            }

            RTOS::block_current_task(&blocked_tasks_);
        }

        disable_interrupts();
        count_--;
        enable_interrupts();

        return true;
    }
};
```

**Usage**:
```cpp
BinarySemaphore data_ready;
CountingSemaphore buffer_pool(5, 5);  // 5 buffers available

void interrupt_handler() {
    process_data();
    data_ready.give();  // Signal task
}

void task() {
    while (1) {
        data_ready.take();  // Wait for signal
        handle_data();
    }
}
```

## 4. Mutex Design (Priority Inheritance)

```cpp
class Mutex {
private:
    volatile bool locked_{false};
    TaskControlBlock* owner_{nullptr};
    TaskControlBlock* blocked_tasks_{nullptr};
    uint8_t original_priority_;

public:
    void lock() {
        while (locked_) {
            // Priority inheritance: temporarily boost owner's priority
            if (owner_ != nullptr &&
                RTOS::current_task()->priority > owner_->priority) {
                original_priority_ = owner_->priority;
                owner_->priority = RTOS::current_task()->priority;
                RTOS::reschedule();
            }

            RTOS::block_current_task(&blocked_tasks_);
        }

        disable_interrupts();
        locked_ = true;
        owner_ = RTOS::current_task();
        enable_interrupts();
    }

    void unlock() {
        if (owner_ != RTOS::current_task()) {
            // Error: unlocking mutex not owned
            return;
        }

        disable_interrupts();
        locked_ = false;

        // Restore original priority if it was boosted
        if (owner_->priority != original_priority_) {
            owner_->priority = original_priority_;
        }

        owner_ = nullptr;
        enable_interrupts();

        RTOS::unblock_one_task(&blocked_tasks_);
        RTOS::reschedule();
    }
};

// RAII lock guard
class LockGuard {
    Mutex& mutex_;
public:
    LockGuard(Mutex& m) : mutex_(m) { mutex_.lock(); }
    ~LockGuard() { mutex_.unlock(); }
};
```

**Usage**:
```cpp
Mutex spi_mutex;

void task1() {
    LockGuard lock(spi_mutex);
    spi.transfer(data);
    // Automatically unlocks on scope exit
}
```

## 5. Event Flags Design

```cpp
class EventFlags {
private:
    volatile uint32_t flags_{0};
    TaskControlBlock* blocked_tasks_{nullptr};

public:
    // Set flags (OR)
    void set(uint32_t flags) {
        disable_interrupts();
        flags_ |= flags;
        enable_interrupts();

        RTOS::unblock_all_tasks(&blocked_tasks_);
    }

    // Clear flags
    void clear(uint32_t flags) {
        disable_interrupts();
        flags_ &= ~flags;
        enable_interrupts();
    }

    // Wait for ANY flags
    uint32_t wait_any(uint32_t flags, uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while ((flags_ & flags) == 0) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return 0;
            }

            RTOS::block_current_task(&blocked_tasks_);
        }

        return flags_ & flags;
    }

    // Wait for ALL flags
    bool wait_all(uint32_t flags, uint32_t timeout_ms = INFINITE) {
        uint32_t start = systick::micros();

        while ((flags_ & flags) != flags) {
            if (timeout_ms != INFINITE &&
                systick::is_timeout(start, timeout_ms * 1000)) {
                return false;
            }

            RTOS::block_current_task(&blocked_tasks_);
        }

        return true;
    }
};
```

**Usage**:
```cpp
EventFlags events;

constexpr uint32_t BUTTON_PRESSED = (1 << 0);
constexpr uint32_t DATA_READY     = (1 << 1);
constexpr uint32_t TIMEOUT        = (1 << 2);

void task() {
    while (1) {
        uint32_t flags = events.wait_any(BUTTON_PRESSED | DATA_READY);

        if (flags & BUTTON_PRESSED) {
            handle_button();
        }
        if (flags & DATA_READY) {
            handle_data();
        }
    }
}
```

## 6. Memory Footprint

### Per-Task Overhead
- TCB: ~32 bytes
- Stack: User-configured (256-1024 bytes typical)
- Total: ~32 bytes + stack size

### Global RTOS Data
- Ready queue: 8 pointers + 1 byte bitmap = ~36 bytes
- Current task pointer: 4 bytes
- Tick counter: 4 bytes
- Scheduler state: ~16 bytes
- **Total**: ~60 bytes

### IPC Objects
- Queue<T, N>: `sizeof(T) * N + 32` bytes
- Semaphore: ~16 bytes
- Mutex: ~20 bytes
- EventFlags: ~12 bytes

### Example Application
```cpp
Task<512, Priority::High>   task1;     // 544 bytes
Task<512, Priority::Normal> task2;     // 544 bytes
Task<256, Priority::Low>    task3;     // 288 bytes
Queue<uint32_t, 10>         queue;     // 72 bytes
Mutex                       mutex;     // 20 bytes
EventFlags                  events;    // 12 bytes

// RTOS core: 60 bytes
// Total: 1540 bytes RAM
```

**Typical applications**: 1-3 KB RAM for RTOS

## 7. Platform-Specific Notes

### ARM Cortex-M (STM32F1, STM32F4, SAMD21, RP2040)
- Use PendSV for context switching
- Leverage SysTick for tick interrupt
- Stack must be 8-byte aligned
- Use CLZ instruction for O(1) priority search
- Context: 16 registers × 4 bytes = 64 bytes

### ESP32 (Xtensa)
- Use software interrupt for context switch
- More registers to save (~40)
- Stack must be 16-byte aligned
- Context: ~160 bytes
- Can leverage dual-core (future work)

## 8. Safety Features

### Stack Overflow Detection (Debug Builds)
```cpp
#ifdef DEBUG
    constexpr uint32_t STACK_CANARY = 0xDEADBEEF;

    void init_stack() {
        // Place canary at bottom of stack
        *reinterpret_cast<uint32_t*>(tcb_.stack_base) = STACK_CANARY;
    }

    void check_stack() {
        if (*reinterpret_cast<uint32_t*>(tcb_.stack_base) != STACK_CANARY) {
            // Stack overflow detected!
            PANIC("Stack overflow in task %s", tcb_.name);
        }
    }
#endif
```

### Deadlock Detection (Debug Builds)
Track mutex ownership graph, detect cycles.

### Priority Inversion Prevention
Mutexes implement priority inheritance automatically.

## References
- ARM Cortex-M Programming Guide: PendSV usage
- FreeRTOS Kernel: Scheduling algorithms
- µC/OS-II: Classic RTOS design patterns
- Modern C++ Concurrency: Lock-free programming
