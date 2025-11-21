/// Example: Phase 2 - Compile-Time TaskSet and Type Safety
///
/// This example demonstrates the new Phase 2 features:
/// 1. fixed_string for zero-RAM task names
/// 2. TaskSet for compile-time RAM calculation
/// 3. C++20 concepts for type safety
/// 4. Compile-time validation with static_assert
///
/// Board: Any (Nucleo F401RE, F722ZE, G071RB, etc.)
/// Features: RTOS with compile-time guarantees

// Note: This is a demonstration example showing Phase 2 features.
// To compile, you need to include board-specific headers.
// The static_asserts below will work in any context once headers are included.

#include "hal/interface/systick.hpp"
#include "rtos/rtos.hpp"
#include "rtos/queue.hpp"
#include "rtos/mutex.hpp"
#include "rtos/semaphore.hpp"
#include "rtos/concepts.hpp"

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Compile-Time Type Safety with Concepts
// ============================================================================

// ✅ Valid IPC Message (trivially copyable, small, not a pointer)
struct SensorData {
    core::u32 timestamp;
    core::i16 temperature;
    core::i16 humidity;
    core::u16 pressure;
};
static_assert(IPCMessage<SensorData>, "SensorData must be IPCMessage");

// ✅ Valid IPC Message
struct Command {
    core::u8 id;
    core::u8 param[7];
};
static_assert(IPCMessage<Command>, "Command must be IPCMessage");

// ❌ Invalid IPC Message (too large - would fail if uncommented)
// struct LargeMessage {
//     core::u8 data[512];  // > 256 bytes limit
// };
// static_assert(IPCMessage<LargeMessage>, "This would fail!");

// ❌ Invalid IPC Message (pointer - would fail if uncommented)
// using BadMessage = core::u32*;
// static_assert(IPCMessage<BadMessage>, "This would fail!");

// ============================================================================
// Global RTOS Objects
// ============================================================================

// Queue with compile-time type safety
Queue<SensorData, 8> sensor_queue;
Queue<Command, 4> command_queue;

// Mutex and semaphore (validated by concepts)
Mutex display_mutex;
BinarySemaphore data_ready_sem;
CountingSemaphore<5> buffer_pool_sem(5);

// Verify concepts at compile time
static_assert(Lockable<Mutex>, "Mutex must be Lockable");
static_assert(Semaphore<BinarySemaphore>, "BinarySemaphore must be Semaphore");
static_assert(QueueProducer<Queue<SensorData, 8>, SensorData>, "Must be producer");
static_assert(QueueConsumer<Queue<SensorData, 8>, SensorData>, "Must be consumer");

// ============================================================================
// Task Functions
// ============================================================================

/// Sensor task: Reads sensor and sends data to queue
void sensor_task_func() {
    core::u32 counter = 0;

    while (true) {
        // Simulate sensor reading
        SensorData data{
            .timestamp = RTOS::get_tick_count(),
            .temperature = static_cast<core::i16>(2500 + (counter % 100)),
            .humidity = static_cast<core::i16>(6000 + (counter % 200)),
            .pressure = static_cast<core::u16>(101325 + (counter % 500))
        };

        // Send to queue (type-safe with Result)
        auto result = sensor_queue.send(data, 1000);
        if (result.is_ok()) {
            // Successfully sent
            data_ready_sem.give().unwrap();
        } else {
            // Handle error (queue full, etc.)
            // In real code: log error
        }

        counter++;
        RTOS::delay(100);  // 100ms sampling rate
    }
}

/// Display task: Receives sensor data and displays
void display_task_func() {
    while (true) {
        // Wait for data ready signal
        if (data_ready_sem.take(INFINITE).is_ok()) {
            // Receive data from queue
            auto result = sensor_queue.receive(1000);
            if (result.is_ok()) {
                SensorData data = result.unwrap();

                // Lock display for thread-safe output
                LockGuard lock(display_mutex, 100);
                if (lock.is_locked()) {
                    // Display data (in real code: use UART/LCD)
                    // uart_printf("Temp: %d.%d°C, Humidity: %d.%d%%\n",
                    //             data.temperature / 100, data.temperature % 100,
                    //             data.humidity / 100, data.humidity % 100);
                }
            }
        }
    }
}

/// Command processor task
void command_task_func() {
    while (true) {
        // Try to receive command (non-blocking)
        auto result = command_queue.try_receive();
        if (result.is_ok()) {
            Command cmd = result.unwrap();

            // Process command
            LockGuard lock(display_mutex, 100);
            if (lock.is_locked()) {
                // Process based on cmd.id
                // In real code: execute command
            }
        }

        RTOS::delay(50);  // Check every 50ms
    }
}

/// Logger task: Low-priority background task
void logger_task_func() {
    while (true) {
        // Get buffer from pool
        if (buffer_pool_sem.take(1000).is_ok()) {
            // Use buffer for logging
            // ... log data ...

            // Return buffer to pool
            buffer_pool_sem.give().unwrap();
        }

        RTOS::delay(500);  // Log every 500ms
    }
}

// ============================================================================
// Task Declaration with fixed_string (Zero RAM for names)
// ============================================================================

// New API: Task names are compile-time strings (zero RAM cost)
Task<512, Priority::High, "Sensor"> sensor_task(sensor_task_func);
Task<1024, Priority::Normal, "Display"> display_task(display_task_func);
Task<512, Priority::Normal, "Command"> command_task(command_task_func);
Task<256, Priority::Low, "Logger"> logger_task(logger_task_func);

// ============================================================================
// Compile-Time TaskSet Validation
// ============================================================================

// Create TaskSet for compile-time validation
using MyTaskSet = TaskSet<
    decltype(sensor_task),
    decltype(display_task),
    decltype(command_task),
    decltype(logger_task)
>;

// Compile-time assertions (all evaluated at compile time!)
static_assert(MyTaskSet::count() == 4, "Must have 4 tasks");

// Calculate total RAM at compile time
static_assert(MyTaskSet::total_stack_ram() == 2304,  // 512 + 1024 + 512 + 256
              "Stack RAM must be 2304 bytes");

static_assert(MyTaskSet::total_ram() == 2432,  // 2304 + (4 * 32 TCB)
              "Total RAM must be 2432 bytes");

// Priority validation
static_assert(MyTaskSet::highest_priority() == static_cast<core::u8>(Priority::High),
              "Highest priority must be High");
static_assert(MyTaskSet::lowest_priority() == static_cast<core::u8>(Priority::Low),
              "Lowest priority must be Low");

// Note: We allow duplicate priorities (Display and Command both Normal)
// so this would fail:
// static_assert(MyTaskSet::has_unique_priorities(), "Would fail!");

// Validate all stacks are valid size
static_assert(MyTaskSet::has_valid_stacks(), "All stacks must be valid");

// Comprehensive validation
static_assert(MyTaskSet::validate<false>(), "Task set must validate");

// Print task set info at compile time (visible in error messages if fails)
using TaskInfo = MyTaskSet::Info;
static_assert(TaskInfo::task_count == 4);
static_assert(TaskInfo::total_ram_bytes == 2432);
static_assert(TaskInfo::total_stack_bytes == 2304);
static_assert(TaskInfo::max_priority == 4);  // High = 4
static_assert(TaskInfo::min_priority == 2);  // Low = 2
static_assert(!TaskInfo::unique_priorities);  // We have duplicates

// ============================================================================
// Compile-Time Name Access
// ============================================================================

// Task names are accessible at compile time (stored in .rodata, not RAM)
static_assert(sensor_task.name()[0] == 'S', "Sensor task name check");
static_assert(display_task.name()[0] == 'D', "Display task name check");

// Stack sizes are accessible at compile time
static_assert(sensor_task.stack_size() == 512);
static_assert(display_task.stack_size() == 1024);
static_assert(command_task.stack_size() == 512);
static_assert(logger_task.stack_size() == 256);

// Priorities are accessible at compile time
static_assert(sensor_task.priority() == Priority::High);
static_assert(display_task.priority() == Priority::Normal);
static_assert(logger_task.priority() == Priority::Low);

// ============================================================================
// Main Function
// ============================================================================

int main() {
    // Note: In real application, initialize board here
    // board::Board::initialize();

    // At this point, all compile-time validation has passed!
    // We know:
    // - Total RAM usage: 2432 bytes (calculated at compile time)
    // - All tasks have valid stack sizes
    // - All message types are IPCMessage compliant
    // - All synchronization primitives satisfy their concepts

    // Start RTOS scheduler (never returns)
    RTOS::start();

    return 0;  // Never reached
}

// ============================================================================
// Compile-Time Guarantees Summary
// ============================================================================

// This example demonstrates that we can verify at compile time:
//
// ✅ Total RAM usage (2432 bytes)
// ✅ Task count (4 tasks)
// ✅ Priority range (Low=2 to High=4)
// ✅ Stack sizes are valid (256-65536 bytes, 8-byte aligned)
// ✅ Task names are compile-time strings (zero RAM)
// ✅ Queue message types are IPCMessage compliant
// ✅ Mutex satisfies Lockable concept
// ✅ Semaphores satisfy Semaphore concept
// ✅ Queues satisfy Producer/Consumer concepts
//
// All of this validation happens at compile time with ZERO runtime overhead!
//
// Benefits:
// - Catch configuration errors at compile time (not runtime)
// - Know exact RAM usage before deploying
// - Type-safe IPC with clear error messages
// - Self-documenting code with concepts
// - Zero runtime cost for all validation
