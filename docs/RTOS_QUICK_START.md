# Alloy RTOS Quick Start Guide

Get started with Alloy RTOS in 10 minutes!

---

## Prerequisites

- **C++ Compiler**: GCC 13+ or Clang 16+ with C++23 support
- **CMake**: 3.20 or later
- **Board**: Cortex-M, ESP32, or host (x86-64)

---

## Step 1: Hello RTOS (Blinky)

### Create Main File

```cpp
#include "rtos/rtos.hpp"
#include "hal/gpio.hpp"

using namespace alloy;
using namespace alloy::rtos;

// LED blink task
void led_task_func() {
    auto led = hal::GPIO::get_pin(board::LED_PIN);
    led.set_mode(hal::PinMode::Output);

    while (1) {
        led.toggle();
        RTOS::delay(500);  // Blink every 500ms
    }
}

// Create task: 256 bytes stack, High priority, "LED" name
Task<256, Priority::High, "LED"> led_task(led_task_func);

int main() {
    // Initialize board
    board::Board::initialize();

    // Start RTOS (never returns)
    RTOS::start();
}
```

### Build and Run

```bash
cmake -B build -DCMAKE_CXX_STANDARD=23
cmake --build build
```

**Result**: LED blinks every 500ms!

---

## Step 2: Multiple Tasks

### Add Second Task

```cpp
// UART print task
void uart_task_func() {
    while (1) {
        printf("Hello from UART task!\n");
        RTOS::delay(1000);  // Print every 1s
    }
}

// Tasks with different priorities
Task<256, Priority::High, "LED"> led_task(led_task_func);
Task<512, Priority::Normal, "UART"> uart_task(uart_task_func);

int main() {
    board::Board::initialize();
    RTOS::start();  // Both tasks run
}
```

---

## Step 3: Inter-Task Communication (Queue)

### Producer-Consumer Pattern

```cpp
#include "rtos/queue.hpp"

// Message structure
struct SensorData {
    uint32_t timestamp;
    int16_t temperature;  // °C * 100
};

// Global queue
Queue<SensorData, 8> sensor_queue;

// Producer task (reads sensor)
void sensor_task_func() {
    while (1) {
        SensorData data{
            .timestamp = hal::SysTick::micros(),
            .temperature = 2350  // 23.50°C
        };

        // Send to queue
        auto result = sensor_queue.send(data, 1000);
        if (result.is_err()) {
            printf("Queue send failed!\n");
        }

        RTOS::delay(100);
    }
}

// Consumer task (processes data)
void display_task_func() {
    while (1) {
        // Receive from queue
        auto result = sensor_queue.receive(INFINITE);
        if (result.is_ok()) {
            SensorData data = result.unwrap();
            printf("Temp: %d.%02d°C\n",
                   data.temperature / 100,
                   data.temperature % 100);
        }
    }
}

Task<512, Priority::High, "Sensor"> sensor_task(sensor_task_func);
Task<512, Priority::Normal, "Display"> display_task(display_task_func);
```

---

## Step 4: Synchronization (Mutex)

### Protect Shared Resource

```cpp
#include "rtos/mutex.hpp"

// Shared resource
uint32_t shared_counter = 0;
Mutex counter_mutex;

void task1_func() {
    while (1) {
        // Use RAII LockGuard (automatic unlock)
        {
            LockGuard guard(counter_mutex);
            if (guard.locked()) {
                shared_counter++;  // Safe access
            }
        }  // Automatically unlocked here

        RTOS::delay(10);
    }
}

void task2_func() {
    while (1) {
        {
            LockGuard guard(counter_mutex);
            if (guard.locked()) {
                printf("Counter: %lu\n", shared_counter);
            }
        }

        RTOS::delay(1000);
    }
}

Task<256, Priority::Normal, "Task1"> task1(task1_func);
Task<256, Priority::Low, "Task2"> task2(task2_func);
```

---

## Step 5: Fast Notifications (ISR → Task)

### GPIO Interrupt Example

```cpp
#include "rtos/task_notification.hpp"

TaskControlBlock* button_task_tcb;

// ISR-safe notification
extern "C" void EXTI0_IRQHandler() {
    // Notify task that button was pressed
    TaskNotification::notify_from_isr(
        button_task_tcb,
        0x01,  // Event flag
        NotifyAction::SetBits
    );
}

// Button handler task
void button_task_func() {
    while (1) {
        // Wait for notification from ISR
        auto result = TaskNotification::wait(INFINITE);
        if (result.is_ok()) {
            uint32_t flags = result.unwrap();

            if (flags & 0x01) {
                printf("Button pressed!\n");
            }
        }
    }
}

Task<256, Priority::High, "Button"> button_task(button_task_func);

int main() {
    board::Board::initialize();

    // Store TCB for ISR
    button_task_tcb = button_task.get_tcb();

    // Enable GPIO interrupt
    // ...

    RTOS::start();
}
```

**Benefit**: 10x faster than Queue for simple events!

---

## Step 6: Memory Pool

### Dynamic Allocation (Bounded)

```cpp
#include "rtos/memory_pool.hpp"

// Message structure
struct Message {
    uint32_t id;
    uint8_t data[64];
};

// Pool of 16 messages
StaticPool<Message, 16> message_pool;

void sender_task_func() {
    uint32_t id = 0;

    while (1) {
        // Allocate from pool (RAII)
        {
            PoolAllocator<Message> msg(message_pool);

            if (msg.is_valid()) {
                msg->id = id++;
                // Fill data...

                printf("Sent message %lu\n", msg->id);
            } else {
                printf("Pool exhausted!\n");
            }

            // Automatically deallocated here
        }

        RTOS::delay(100);
    }
}

Task<512, Priority::Normal, "Sender"> sender_task(sender_task_func);
```

**Benefit**: 20x faster than malloc, zero fragmentation!

---

## Step 7: Power Management

### Enable Tickless Idle

```cpp
#include "rtos/tickless_idle.hpp"

// Platform-specific sleep hook
extern "C" void tickless_enter_sleep(SleepMode mode, uint32_t duration_us) {
    switch (mode) {
        case SleepMode::Light:
            __WFI();  // Wait for interrupt
            break;

        case SleepMode::Deep:
            // Enter STOP mode
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
            // Restore clocks
            SystemClock_Config();
            break;
    }
}

int main() {
    board::Board::initialize();

    // Enable power management
    TicklessIdle::enable();
    TicklessIdle::configure(SleepMode::Deep, 5000);  // Min 5ms sleep

    RTOS::start();  // Automatically sleeps when idle
}
```

**Benefit**: Up to 80% power savings!

---

## Step 8: Compile-Time Validation

### TaskSet Validation

```cpp
// Define all tasks
Task<512, Priority::High, "Sensor"> sensor_task(sensor_func);
Task<1024, Priority::Normal, "Display"> display_task(display_func);
Task<256, Priority::Low, "Logger"> logger_task(logger_func);

// Group into TaskSet
using MyTasks = TaskSet<
    decltype(sensor_task),
    decltype(display_task),
    decltype(logger_task)
>;

// Compile-time validation
static_assert(MyTasks::total_ram() == 1888);  // Stack + TCB
static_assert(MyTasks::validate());            // Basic validation
static_assert(MyTasks::all_tasks_valid());     // Concept validation

// RAM budget check (C++23)
static_assert(MyTasks::total_ram_with_budget<8192>() == 1888,
              "Tasks must fit in 8KB RAM");

int main() {
    board::Board::initialize();

    // All validation passed at compile-time!
    printf("System RAM: %zu bytes\n", MyTasks::total_ram());

    RTOS::start();
}
```

**Benefit**: Catch configuration errors at compile-time!

---

## Common Patterns

### Pattern 1: Periodic Task

```cpp
void periodic_task_func() {
    while (1) {
        // Do work
        process_data();

        // Wait for next period
        RTOS::delay(100);  // 100ms period
    }
}

Task<512, Priority::Normal, "Periodic"> periodic_task(periodic_task_func);
```

---

### Pattern 2: Event-Driven Task

```cpp
void event_task_func() {
    while (1) {
        // Wait for event
        auto result = TaskNotification::wait(INFINITE);
        if (result.is_ok()) {
            uint32_t event = result.unwrap();
            // Handle event
            handle_event(event);
        }
    }
}

Task<512, Priority::High, "Event"> event_task(event_task_func);
```

---

### Pattern 3: Producer-Consumer

```cpp
Queue<Data, 8> queue;

void producer_func() {
    while (1) {
        Data data = read_sensor();
        queue.send(data, 1000).unwrap();
        RTOS::delay(100);
    }
}

void consumer_func() {
    while (1) {
        auto result = queue.receive(INFINITE);
        if (result.is_ok()) {
            process_data(result.unwrap());
        }
    }
}

Task<512, Priority::High, "Producer"> producer(producer_func);
Task<512, Priority::Normal, "Consumer"> consumer(consumer_func);
```

---

### Pattern 4: Protected Resource

```cpp
Mutex resource_mutex;
Resource shared_resource;

void accessor_task_func() {
    while (1) {
        {
            LockGuard guard(resource_mutex);
            if (guard.locked()) {
                // Safe access
                shared_resource.update();
            }
        }  // Auto unlock

        RTOS::delay(50);
    }
}

Task<256, Priority::Normal, "Accessor"> accessor_task(accessor_task_func);
```

---

## Debugging Tips

### 1. Check Task Stack Usage

```cpp
void monitor_task_func() {
    while (1) {
        uint32_t usage = sensor_task.get_stack_usage();
        printf("Stack usage: %lu bytes\n", usage);

        RTOS::delay(5000);
    }
}
```

---

### 2. Monitor Queue State

```cpp
if (queue.is_full()) {
    printf("Warning: Queue full!\n");
}

printf("Queue: %zu/%zu messages\n",
       queue.available(),
       queue.capacity());
```

---

### 3. Check Pool Availability

```cpp
size_t available = pool.available();
if (available < 2) {
    printf("Warning: Pool almost full!\n");
}
```

---

### 4. Power Statistics

```cpp
auto& stats = get_power_stats();
printf("Sleep time: %lu µs\n", stats.total_sleep_time_us);
printf("Sleep efficiency: %u%%\n", stats.efficiency_percent());
```

---

## Next Steps

### Learn More

- **API Reference**: See [RTOS_API_REFERENCE.md](RTOS_API_REFERENCE.md)
- **Migration Guide**: See [RTOS_MIGRATION_GUIDE.md](RTOS_MIGRATION_GUIDE.md)
- **Performance**: See [RTOS_PERFORMANCE_BENCHMARKS.md](RTOS_PERFORMANCE_BENCHMARKS.md)
- **Examples**: See `examples/rtos/` directory

### Advanced Features

1. **Advanced Validation** (Phase 3):
   - Priority inversion detection
   - Deadlock prevention
   - Schedulability analysis

2. **C++23 Enhancements** (Phase 5):
   - Enhanced consteval
   - if consteval dual-mode
   - Compile-time utilities

3. **Advanced Features** (Phase 6):
   - TaskNotification modes
   - StaticPool optimization
   - TicklessIdle tuning

---

## Troubleshooting

### Problem: Task doesn't run

**Solution**: Check priority and RTOS::start() was called

```cpp
printf("Task priority: %u\n", static_cast<uint8_t>(task.priority()));
```

---

### Problem: Stack overflow

**Solution**: Increase stack size

```cpp
// Before
Task<256, ...> task(...);  // Too small

// After
Task<512, ...> task(...);  // Larger stack
```

---

### Problem: Queue timeout

**Solution**: Check queue capacity and sender/receiver

```cpp
// Increase capacity
Queue<Data, 16> queue;  // Was 8

// Or increase timeout
queue.receive(5000);  // 5 seconds
```

---

### Problem: Mutex deadlock

**Solution**: Use consistent lock ordering

```cpp
// Always lock in same order
{
    LockGuard g1(mutex1);  // Always first
    LockGuard g2(mutex2);  // Always second
}
```

---

## Checklist

Quick startup checklist:

- [ ] Board initialized (`Board::initialize()`)
- [ ] At least one task created
- [ ] RTOS started (`RTOS::start()`)
- [ ] SysTick configured (usually automatic)
- [ ] Correct stack sizes (≥256 bytes, 8-byte aligned)
- [ ] Priorities valid (0-7)
- [ ] Compile with C++23 (`-std=c++23`)

---

## Example Project Structure

```
my_project/
├── CMakeLists.txt
├── src/
│   ├── main.cpp           # RTOS startup
│   ├── tasks/
│   │   ├── sensor.cpp     # Sensor task
│   │   ├── display.cpp    # Display task
│   │   └── logger.cpp     # Logger task
│   └── board/
│       └── board.cpp      # Board init + SysTick
├── include/
│   └── config.hpp         # System configuration
└── README.md
```

---

## Minimal CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_rtos_project CXX)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Alloy library
add_subdirectory(alloy)

# Your executable
add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE alloy-hal)
```

---

## Summary

**You've learned**:

1. ✅ Create tasks with compile-time names
2. ✅ Use queues for inter-task communication
3. ✅ Protect shared resources with mutexes
4. ✅ Fast ISR→Task notifications
5. ✅ Memory pools for dynamic allocation
6. ✅ Power management with tickless idle
7. ✅ Compile-time validation

**Time to hello-world**: ~10 minutes
**Time to full system**: ~1 hour

**Next**: Explore examples in `examples/rtos/` and read the [API Reference](RTOS_API_REFERENCE.md)!

---

## Need Help?

- **Documentation**: Check `docs/` directory
- **Examples**: See `examples/rtos/`
- **Issues**: File on GitHub
- **Community**: Discord/Forum

Happy coding with Alloy RTOS! 🚀
