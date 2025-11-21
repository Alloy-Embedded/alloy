/// Phase 5 Example: C++23 Enhanced RTOS Features
///
/// This example demonstrates the C++23 enhancements added in Phase 5:
/// - consteval for guaranteed compile-time validation
/// - if consteval for dual-mode functions
/// - Enhanced error reporting with custom messages
/// - Compile-time string validation
/// - Utility functions (log2, array_max, array_min)
///
/// Compile with:
/// ```bash
/// cmake -B build -DCMAKE_CXX_STANDARD=23
/// cmake --build build --target phase5_example
/// ```

#include "rtos/rtos.hpp"
#include "rtos/queue.hpp"
#include "rtos/mutex.hpp"
#include "rtos/semaphore.hpp"
#include "hal/interface/systick.hpp"

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Example 1: Enhanced Task Validation with C++23
// ============================================================================

/// Sensor data message
struct SensorData {
    core::u32 timestamp;
    core::i16 temperature;   // °C * 100
    core::i16 humidity;      // % * 100
    core::u16 pressure;      // hPa
};

static_assert(IPCMessage<SensorData>, "SensorData must be IPCMessage");

/// Sensor task function
void sensor_task_func() {
    static Queue<SensorData, 8> sensor_queue;

    while (1) {
        // Simulate sensor reading
        SensorData data{
            .timestamp = hal::SysTick::micros(),
            .temperature = 2350,  // 23.50°C
            .humidity = 6500,     // 65.00%
            .pressure = 1013      // 1013 hPa
        };

        // Send to queue (non-blocking)
        auto result = sensor_queue.try_send(data);
        if (result.is_ok()) {
            // Success
        }

        RTOS::delay(100);  // Read every 100ms
    }
}

/// Display task function
void display_task_func() {
    while (1) {
        // Update display
        RTOS::delay(500);  // Update every 500ms
    }
}

/// Logger task function
void logger_task_func() {
    while (1) {
        // Log data
        RTOS::delay(1000);  // Log every 1s
    }
}

// ============================================================================
// Example 2: C++23 Enhanced Task Definitions
// ============================================================================

// Task definitions with compile-time validation
Task<512, Priority::High, "Sensor"> sensor_task(sensor_task_func);
Task<1024, Priority::Normal, "Display"> display_task(display_task_func);
Task<256, Priority::Low, "Logger"> logger_task(logger_task_func);

// ============================================================================
// Example 3: TaskSet with C++23 Budget Validation
// ============================================================================

using MyTasks = TaskSet<
    decltype(sensor_task),
    decltype(display_task),
    decltype(logger_task)
>;

// Compile-time assertions using C++23 enhanced features
static_assert(MyTasks::count() == 3, "Should have 3 tasks");

// Stack RAM calculation (C++23: uses array_max internally)
static_assert(MyTasks::total_stack_ram() == 1792,  // 512 + 1024 + 256
              "Total stack RAM should be 1792 bytes");

// Total RAM (stack + TCB overhead: 32 bytes per task)
static_assert(MyTasks::total_ram() == 1888,  // 1792 + 96
              "Total RAM should be 1888 bytes");

// C++23: RAM budget validation with enhanced error reporting
// This will produce a compile-time error if budget is exceeded
static_assert(MyTasks::total_ram_with_budget<4096>() == 1888,
              "RAM calculation with budget check");

// Priority analysis (C++23: uses array_max/array_min)
static_assert(MyTasks::highest_priority() == static_cast<core::u8>(Priority::High),
              "Highest priority should be High (4)");
static_assert(MyTasks::lowest_priority() == static_cast<core::u8>(Priority::Low),
              "Lowest priority should be Low (2)");

// Advanced validation
static_assert(MyTasks::validate<false>(), "Task set should be valid");
static_assert(MyTasks::all_tasks_valid(), "All tasks should satisfy ValidTask concept");

// ============================================================================
// Example 4: C++23 Compile-Time Validation Examples
// ============================================================================

// Valid task name check (C++23: consteval)
constexpr char valid_name1[] = "MySensor";
constexpr char valid_name2[] = "Task_123";
constexpr char valid_name3[] = "High-Priority";
static_assert(is_valid_task_name(valid_name1), "Should be valid");
static_assert(is_valid_task_name(valid_name2), "Should be valid");
static_assert(is_valid_task_name(valid_name3), "Should be valid");

// Invalid task names (would fail at compile time if uncommented)
// constexpr char invalid_name1[] = "";  // Too short
// static_assert(is_valid_task_name(invalid_name1));  // ❌ Compile error

// constexpr char invalid_name2[] = "Task@123";  // Invalid char '@'
// static_assert(is_valid_task_name(invalid_name2));  // ❌ Compile error

// Stack size validation (C++23: enhanced error messages)
static_assert(validate_stack_size<512>() == 512, "512 bytes is valid");
static_assert(validate_stack_size<1024>() == 1024, "1024 bytes is valid");

// These would fail at compile time with descriptive error messages:
// static_assert(validate_stack_size<128>() == 128);  // ❌ "Stack size must be at least 256 bytes"
// static_assert(validate_stack_size<100000>() == 100000);  // ❌ "Stack size must not exceed 65536 bytes"
// static_assert(validate_stack_size<513>() == 513);  // ❌ "Stack size must be 8-byte aligned"

// Priority validation (C++23: enhanced error messages)
static_assert(validate_priority<0>() == 0, "Priority 0 is valid");
static_assert(validate_priority<7>() == 7, "Priority 7 is valid");

// This would fail at compile time:
// static_assert(validate_priority<8>() == 8);  // ❌ "Priority must be between 0 and 7"

// ============================================================================
// Example 5: C++23 Utility Functions
// ============================================================================

// Power of 2 check (C++23: consteval)
static_assert(is_power_of_2<1>(), "1 is power of 2");
static_assert(is_power_of_2<2>(), "2 is power of 2");
static_assert(is_power_of_2<256>(), "256 is power of 2");
static_assert(is_power_of_2<1024>(), "1024 is power of 2");
static_assert(!is_power_of_2<3>(), "3 is not power of 2");
static_assert(!is_power_of_2<100>(), "100 is not power of 2");

// Log2 calculation (C++23: consteval)
static_assert(log2_constexpr<1>() == 0, "log2(1) = 0");
static_assert(log2_constexpr<2>() == 1, "log2(2) = 1");
static_assert(log2_constexpr<256>() == 8, "log2(256) = 8");
static_assert(log2_constexpr<1024>() == 10, "log2(1024) = 10");

// Array max/min (C++23: consteval)
constexpr core::u8 priorities[] = {2, 4, 1, 3};
static_assert(array_max(priorities) == 4, "Max priority is 4");
static_assert(array_min(priorities) == 1, "Min priority is 1");

constexpr size_t stack_sizes[] = {256, 512, 1024, 2048};
static_assert(array_max(stack_sizes) == 2048, "Max stack is 2048");
static_assert(array_min(stack_sizes) == 256, "Min stack is 256");

// ============================================================================
// Example 6: C++23 if consteval Demonstration
// ============================================================================

// Dual-mode RAM calculation
// When called in consteval context, uses fold expressions
// When called at runtime, uses traditional loop
template <size_t... Sizes>
void demonstrate_dual_mode() {
    // Compile-time evaluation (guaranteed by constexpr context)
    constexpr size_t compile_time_result = calculate_total_ram_dual<Sizes...>();

    // Runtime evaluation (same function, different code path)
    volatile size_t runtime_result = calculate_total_ram_dual<Sizes...>();

    // Both should produce the same result
    // But compile-time path is optimized to fold expression
    // Runtime path uses traditional loop for flexibility
}

// ============================================================================
// Example 7: C++23 Enhanced Error Reporting
// ============================================================================

// RAM budget check with detailed error
template <size_t Budget>
consteval void check_system_ram_budget() {
    constexpr size_t sensor_stack = 512;
    constexpr size_t display_stack = 1024;
    constexpr size_t logger_stack = 256;
    constexpr size_t total_stack = sensor_stack + display_stack + logger_stack;

    constexpr size_t tcb_overhead = 3 * 32;  // 3 tasks * 32 bytes
    constexpr size_t total_ram = total_stack + tcb_overhead;

    // C++23: compile_time_check with custom error message
    compile_time_check(total_ram <= Budget,
                      "System RAM exceeds budget - reduce stack sizes or increase budget");
}

// Verify our system fits in 4KB RAM
static consteval void verify_system_requirements() {
    check_system_ram_budget<4096>();  // ✅ Passes
}

// This would fail at compile time with descriptive error:
// static consteval void verify_tight_budget() {
//     check_system_ram_budget<1500>();  // ❌ "System RAM exceeds budget..."
// }

// ============================================================================
// Example 8: Advanced C++23 Patterns
// ============================================================================

/// Compile-time task configuration validator
template <typename... Tasks>
consteval bool validate_task_configuration() {
    using TS = TaskSet<Tasks...>;

    // Check 1: At least one task
    if (TS::count() == 0) {
        throw "Task configuration must have at least one task";
    }

    // Check 2: RAM budget (example: 8KB limit)
    if (TS::total_ram() > 8192) {
        throw "Task configuration exceeds 8KB RAM budget";
    }

    // Check 3: Priority range
    if (TS::highest_priority() > 7) {
        throw "Invalid priority detected (must be 0-7)";
    }

    // Check 4: All tasks valid
    if (!TS::all_tasks_valid()) {
        throw "One or more tasks failed ValidTask concept check";
    }

    return true;
}

// Validate our configuration
static_assert(validate_task_configuration<
    decltype(sensor_task),
    decltype(display_task),
    decltype(logger_task)
>(), "Task configuration validation");

// ============================================================================
// Example 9: C++23 Performance Analysis
// ============================================================================

/// Compile-time performance metrics
struct SystemMetrics {
    static consteval size_t total_ram() {
        return MyTasks::total_ram();
    }

    static consteval core::u8 task_count() {
        return MyTasks::count();
    }

    static consteval size_t average_stack_size() {
        return MyTasks::total_stack_ram() / MyTasks::count();
    }

    static consteval bool has_priority_gaps() {
        constexpr core::u8 high = MyTasks::highest_priority();
        constexpr core::u8 low = MyTasks::lowest_priority();
        return (high - low) > MyTasks::count();
    }
};

// Compile-time metrics analysis
static_assert(SystemMetrics::total_ram() == 1888, "Total system RAM");
static_assert(SystemMetrics::task_count() == 3, "Task count");
static_assert(SystemMetrics::average_stack_size() == 597,  // 1792 / 3 = 597.33
              "Average stack size per task");

// ============================================================================
// Main Function
// ============================================================================

int main() {
    // Initialize board
    // board::Board::initialize();

    // All tasks are already created and registered
    // TaskSet validation occurred at compile time

    // Print compile-time info (for demonstration)
    // In real embedded system, you might log this to UART

    // Start RTOS scheduler (never returns)
    // RTOS::start();

    return 0;
}

// ============================================================================
// Summary of C++23 Features Demonstrated
// ============================================================================

/*
1. consteval Functions:
   - validate_stack_size<>()
   - validate_priority<>()
   - is_valid_task_name()
   - log2_constexpr<>()
   - array_max(), array_min()

2. if consteval:
   - calculate_total_ram_dual<>() - dual compile-time/runtime paths

3. Enhanced Error Reporting:
   - compile_time_check() - throw with custom messages
   - validate_task_configuration<>() - comprehensive checks

4. Compile-Time Guarantees:
   - Task template validation
   - TaskSet RAM budget checking
   - Priority range validation
   - Task name validation

5. Zero Runtime Overhead:
   - All validation at compile time
   - No runtime checks needed
   - Perfect for embedded systems

6. Improved Developer Experience:
   - Better error messages
   - Earlier error detection (compile vs runtime)
   - Self-documenting code through concepts

Key Benefits:
✅ Stronger compile-time safety
✅ Better error messages
✅ Zero runtime overhead
✅ More expressive code
✅ Easier to maintain
✅ Perfect for resource-constrained embedded systems
*/
