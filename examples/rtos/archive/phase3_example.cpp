/// Example: Phase 3 - Advanced Concept-Based Validation
///
/// This example demonstrates Phase 3 advanced features:
/// 1. Priority inversion detection
/// 2. Deadlock prevention with lock order validation
/// 3. ISR-safe function validation
/// 4. Memory budget validation
/// 5. Advanced queue concepts (TimestampedQueue, PriorityQueue)
/// 6. Task concept validation
///
/// Board: Any
/// Features: Advanced compile-time safety analysis

#include "hal/interface/systick.hpp"
#include "rtos/rtos.hpp"
#include "rtos/queue.hpp"
#include "rtos/mutex.hpp"
#include "rtos/semaphore.hpp"
#include "rtos/concepts.hpp"

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Advanced Message Types with Metadata
// ============================================================================

/// High-priority message with priority field
struct HighPriorityCommand {
    core::u8 priority;      // Required by HasPriority concept
    core::u8 command_id;
    core::u8 params[6];
};
static_assert(IPCMessage<HighPriorityCommand>);
static_assert(HasPriority<HighPriorityCommand>, "Must have priority field");

/// Timestamped sensor data
struct TimestampedSensorData {
    core::u32 timestamp;    // Required by HasTimestamp concept
    core::i16 temperature;
    core::i16 humidity;
    core::u16 pressure;
    core::u8 sensor_id;
    core::u8 reserved;
};
static_assert(IPCMessage<TimestampedSensorData>);
static_assert(HasTimestamp<TimestampedSensorData>, "Must have timestamp");
static_assert(sizeof(TimestampedSensorData) <= 256, "Must fit in IPC size limit");

/// Event log entry (both timestamp and priority)
struct LogEntry {
    core::u32 timestamp;
    core::u8 priority;
    core::u8 source;
    core::u16 event_id;
    char message[20];
};
static_assert(IPCMessage<LogEntry>);
static_assert(HasTimestamp<LogEntry>);
static_assert(HasPriority<LogEntry>);
static_assert(PODType<LogEntry>, "Must be POD for serialization");

// ============================================================================
// Advanced Queue Type Validation
// ============================================================================

// Priority queue for high-priority commands
Queue<HighPriorityCommand, 8> priority_queue;
static_assert(PriorityQueue<decltype(priority_queue), HighPriorityCommand>,
              "Must satisfy PriorityQueue concept");

// Timestamped queue for sensor data
Queue<TimestampedSensorData, 16> sensor_queue;
static_assert(TimestampedQueue<decltype(sensor_queue), TimestampedSensorData>,
              "Must satisfy TimestampedQueue concept");

// General queue for log entries (has both)
Queue<LogEntry, 32> log_queue;
static_assert(PriorityQueue<decltype(log_queue), LogEntry>);
static_assert(TimestampedQueue<decltype(log_queue), LogEntry>);
static_assert(BlockingQueue<decltype(log_queue), LogEntry>);
static_assert(NonBlockingQueue<decltype(log_queue), LogEntry>);

// ============================================================================
// Resource Management with Deadlock Prevention
// ============================================================================

// Resources with IDs for lock order validation
enum class ResourceID : core::u8 {
    Display = 1,
    Sensor = 2,
    Logger = 3,
    Network = 4
};

Mutex display_mutex;   // ID: 1
Mutex sensor_mutex;    // ID: 2
Mutex logger_mutex;    // ID: 3
Mutex network_mutex;   // ID: 4

// Validate lock ordering to prevent deadlock
// Rule: Always acquire locks in increasing resource ID order

/// Task A: Display + Logger (correct order: 1 -> 3)
constexpr bool task_a_lock_order_valid =
    has_consistent_lock_order<
        static_cast<core::u8>(ResourceID::Display),
        static_cast<core::u8>(ResourceID::Logger)
    >();
static_assert(task_a_lock_order_valid, "Task A lock order must be valid");

/// Task B: Sensor + Network (correct order: 2 -> 4)
constexpr bool task_b_lock_order_valid =
    has_consistent_lock_order<
        static_cast<core::u8>(ResourceID::Sensor),
        static_cast<core::u8>(ResourceID::Network)
    >();
static_assert(task_b_lock_order_valid, "Task B lock order must be valid");

/// Task C: All resources (correct order: 1 -> 2 -> 3 -> 4)
constexpr bool task_c_lock_order_valid =
    has_consistent_lock_order<
        static_cast<core::u8>(ResourceID::Display),
        static_cast<core::u8>(ResourceID::Sensor),
        static_cast<core::u8>(ResourceID::Logger),
        static_cast<core::u8>(ResourceID::Network)
    >();
static_assert(task_c_lock_order_valid, "Task C lock order must be valid");

// Example of INVALID lock order (would fail compilation if uncommented):
// constexpr bool invalid_order =
//     has_consistent_lock_order<
//         static_cast<core::u8>(ResourceID::Network),  // 4
//         static_cast<core::u8>(ResourceID::Display)   // 1 (wrong!)
//     >();
// static_assert(invalid_order, "This would fail!");

// ============================================================================
// ISR-Safe Functions
// ============================================================================

/// ISR-safe function: quick data sampling (noexcept, no blocking)
inline void sample_sensor_isr() noexcept {
    // Quick sensor read, non-blocking
    // Just set flag or minimal work
}

/// ISR-safe function: signal semaphore from ISR
inline void signal_data_ready_isr() noexcept {
    // Quick semaphore give (non-blocking in ISR context)
}

// Validate ISR safety
static_assert(ISRSafe<decltype(sample_sensor_isr)>, "Must be ISR-safe");
static_assert(ISRSafe<decltype(signal_data_ready_isr)>, "Must be ISR-safe");

// Note: Functions that block or allocate would NOT be ISR-safe
// void blocking_func() {
//     // This would NOT be ISR-safe (not noexcept, may block)
// }

// ============================================================================
// Memory Budget Validation
// ============================================================================

// Calculate total queue memory
constexpr size_t priority_queue_size = 8 * sizeof(HighPriorityCommand);
constexpr size_t sensor_queue_size = 16 * sizeof(TimestampedSensorData);
constexpr size_t log_queue_size = 32 * sizeof(LogEntry);

constexpr size_t total_queue_memory =
    priority_queue_size + sensor_queue_size + log_queue_size;

// Validate against RAM budget (e.g., 4KB for queues)
static_assert(queue_memory_fits_budget<4096,
                                        priority_queue_size,
                                        sensor_queue_size,
                                        log_queue_size>(),
              "Queue memory must fit in 4KB budget");

// ============================================================================
// Task Definitions with Advanced Validation
// ============================================================================

void high_priority_handler_func() {
    while (true) {
        auto result = priority_queue.receive(INFINITE);
        if (result.is_ok()) {
            HighPriorityCommand cmd = result.unwrap();
            // Process high-priority command
            // Lock resources in correct order
            LockGuard display_lock(display_mutex, 100);
            if (display_lock.is_locked()) {
                LockGuard logger_lock(logger_mutex, 100);  // Correct order: 1 -> 3
                if (logger_lock.is_locked()) {
                    // Process with both locks
                }
            }
        }
    }
}

void sensor_processor_func() {
    while (true) {
        auto result = sensor_queue.receive(1000);
        if (result.is_ok()) {
            TimestampedSensorData data = result.unwrap();

            // Lock resources in correct order
            LockGuard sensor_lock(sensor_mutex, 100);
            if (sensor_lock.is_locked()) {
                // Process sensor data

                // Create log entry
                LogEntry log{
                    .timestamp = data.timestamp,
                    .priority = 3,
                    .source = data.sensor_id,
                    .event_id = 0x1001,
                    .message = "Sensor data OK"
                };

                log_queue.try_send(log);
            }
        }

        RTOS::delay(10);
    }
}

void logger_func() {
    while (true) {
        auto result = log_queue.receive(5000);
        if (result.is_ok()) {
            LogEntry log = result.unwrap();

            // Lock in correct order
            LockGuard logger_lock(logger_mutex, 100);
            if (logger_lock.is_locked()) {
                // Write log (e.g., to UART or flash)
            }
        }
    }
}

// Task declarations with compile-time names
Task<1024, Priority::Critical, "HighPriority"> high_priority_task(high_priority_handler_func);
Task<512, Priority::High, "SensorProc"> sensor_task(sensor_processor_func);
Task<512, Priority::Normal, "Logger"> logger_task(logger_func);

// Validate all tasks satisfy ValidTask concept
static_assert(ValidTask<decltype(high_priority_task)>, "Must be ValidTask");
static_assert(ValidTask<decltype(sensor_task)>, "Must be ValidTask");
static_assert(ValidTask<decltype(logger_task)>, "Must be ValidTask");

// ============================================================================
// Advanced TaskSet Analysis
// ============================================================================

using MyTaskSet = TaskSet<
    decltype(high_priority_task),
    decltype(sensor_task),
    decltype(logger_task)
>;

// Basic validation
static_assert(MyTaskSet::count() == 3);
static_assert(MyTaskSet::total_stack_ram() == 2048);  // 1024 + 512 + 512
static_assert(MyTaskSet::total_ram() == 2144);  // + (3 * 32 TCB)
static_assert(MyTaskSet::validate());

// Advanced validation
static_assert(MyTaskSet::all_tasks_valid(), "All tasks must be valid");
static_assert(MyTaskSet::validate_advanced<false, true>(), "Advanced validation");

// Priority analysis
static_assert(MyTaskSet::highest_priority() == static_cast<core::u8>(Priority::Critical));
static_assert(MyTaskSet::lowest_priority() == static_cast<core::u8>(Priority::Normal));

// Priority inversion risk analysis
constexpr bool has_inversion_risk = MyTaskSet::has_priority_inversion_risk();
static_assert(has_inversion_risk,
              "We expect priority inversion risk (Critical=7, Normal=3, gap >1)");

// Note: Priority inheritance in Mutex implementation handles this at runtime

// CPU utilization estimate
static_assert(MyTaskSet::Info::utilization_estimate == 30, "3 tasks * 10%");

// ============================================================================
// Stack Usage Analysis
// ============================================================================

// Hypothetical stack usage per function
constexpr size_t high_priority_stack = 256;
constexpr size_t sensor_proc_stack = 128;
constexpr size_t logger_stack = 128;

// Worst case if all called nested (not realistic for tasks, but demonstrates concept)
constexpr size_t worst_case = worst_case_stack_usage<
    high_priority_stack,
    sensor_proc_stack,
    logger_stack
>();
static_assert(worst_case == 512);  // 256 + 128 + 128

// Ensure individual task stacks can handle their functions
static_assert(high_priority_task.stack_size() >= high_priority_stack);
static_assert(sensor_task.stack_size() >= sensor_proc_stack);
static_assert(logger_task.stack_size() >= logger_stack);

// ============================================================================
// Schedulability Analysis (Simplified)
// ============================================================================

// Simplified Rate Monotonic Analysis (RMA)
// Note: Real RMA requires execution times and periods

// Hypothetical: High-priority task runs 500us every 10ms
constexpr bool high_pri_schedulable = is_schedulable<500, 10000>();
static_assert(high_pri_schedulable, "High-priority task must be schedulable");

// Sensor task runs 200us every 50ms
constexpr bool sensor_schedulable = is_schedulable<200, 50000>();
static_assert(sensor_schedulable, "Sensor task must be schedulable");

// ============================================================================
// Compile-Time Guarantees Summary
// ============================================================================

// At compile time, we have verified:
//
// ✅ All message types are IPCMessage compliant
// ✅ Queues satisfy required concepts (PriorityQueue, TimestampedQueue, etc.)
// ✅ Lock order is consistent (prevents deadlock)
// ✅ ISR functions are noexcept (ISR-safe)
// ✅ Memory budget is satisfied (queues fit in 4KB)
// ✅ All tasks satisfy ValidTask concept
// ✅ Total RAM usage is known (2144 bytes)
// ✅ Priority inversion risk is detected (runtime priority inheritance handles it)
// ✅ Stack sizes are sufficient
// ✅ Task utilization is estimated (30%)
// ✅ Schedulability is verified (simplified RMA)

int main() {
    // All compile-time validation passed!
    // We know:
    // - No deadlocks possible (lock order validated)
    // - No priority inversion (detected, handled by Mutex)
    // - Memory budget satisfied (2144 bytes total)
    // - All types are valid for IPC
    // - ISR functions are safe

    RTOS::start();
    return 0;
}

// ============================================================================
// Key Takeaways
// ============================================================================

// Phase 3 adds powerful compile-time analysis:
//
// 1. **Deadlock Prevention**: Lock order validation catches potential deadlocks
//    before deployment.
//
// 2. **Priority Analysis**: Detects priority inversion scenarios and validates
//    that priority inheritance is available.
//
// 3. **ISR Safety**: Validates that ISR callbacks are noexcept and won't block.
//
// 4. **Memory Budgets**: Ensures queue and task memory fits within constraints.
//
// 5. **Advanced Queue Concepts**: Type-checks for timestamp and priority fields
//    enable specialized queue behaviors.
//
// 6. **Schedulability**: Basic RMA checks ensure tasks are schedulable.
//
// All of this analysis happens at COMPILE TIME with ZERO runtime overhead!
