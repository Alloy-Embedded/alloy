/// Alloy RTOS Host Demo
///
/// Demonstrates RTOS functionality running on host (PC) platform.
/// Tests core RTOS features: tasks, queues, mutexes, semaphores, event flags.
///
/// This example proves that embedded RTOS code can run on a PC for testing!

#include <iomanip>
#include <iostream>

#include "hal/host/delay.hpp"
#include "hal/host/systick.hpp"

#include "rtos/event.hpp"
#include "rtos/mutex.hpp"
#include "rtos/queue.hpp"
#include "rtos/rtos.hpp"
#include "rtos/semaphore.hpp"

using namespace alloy;
using namespace alloy::rtos;

// ============================================================================
// Shared Resources & IPC Objects
// ============================================================================

/// Message structure for queue communication
struct SensorData {
    core::u32 timestamp;
    core::i16 temperature;
    core::i16 humidity;
};

/// Global IPC objects
Queue<SensorData, 8> sensor_queue;
Mutex console_mutex;
BinarySemaphore data_ready_sem;
EventFlags system_events;

// Event flag bits
constexpr core::u32 EVENT_DATA_READY = (1 << 0);
constexpr core::u32 EVENT_THRESHOLD_EXCEEDED = (1 << 1);
constexpr core::u32 EVENT_ERROR = (1 << 2);

// ============================================================================
// Helper Functions
// ============================================================================

/// Thread-safe console output
template <typename... Args>
void print(const char* task_name, Args&&... args) {
    LockGuard lock(console_mutex);
    std::cout << "[" << std::setw(10) << task_name << " | " << std::setw(6)
              << (systick::micros() / 1000) << "ms] ";
    (std::cout << ... << args) << std::endl;
}

// ============================================================================
// Task 1: Sensor Reader (High Priority)
// ============================================================================
/// Simulates reading sensor data and sending to queue

void sensor_task() {
    print("SENSOR", "Task started");

    core::u16 reading_count = 0;

    while (true) {
        // Simulate sensor reading
        SensorData data;
        data.timestamp = systick::micros();
        data.temperature = 20 + (reading_count % 10);  // Simulate 20-30°C
        data.humidity = 50 + (reading_count % 30);     // Simulate 50-80%

        // Send to queue (blocking if full)
        if (sensor_queue.send(data, 1000)) {  // 1 second timeout
            print("SENSOR", "Sent data #", reading_count, ": temp=", data.temperature,
                  "°C, humidity=", data.humidity, "%");

            // Signal data ready
            data_ready_sem.give();
            system_events.set(EVENT_DATA_READY);

            // Check threshold
            if (data.temperature > 28) {
                system_events.set(EVENT_THRESHOLD_EXCEEDED);
            }
        } else {
            print("SENSOR", "Queue full! Data lost.");
        }

        reading_count++;

        // Read every 500ms
        RTOS::delay(500);
    }
}

// ============================================================================
// Task 2: Data Processor (Normal Priority)
// ============================================================================
/// Processes sensor data from queue

void processor_task() {
    print("PROCESSOR", "Task started");

    while (true) {
        // Wait for data ready semaphore
        if (data_ready_sem.take(2000)) {  // 2 second timeout
            // Receive from queue
            SensorData data;
            if (sensor_queue.receive(data, 1000)) {  // 1 second timeout
                // Simulate processing
                core::u32 processing_time = 50 + (data.temperature % 50);

                print("PROCESSOR", "Processing data (", processing_time, "ms)...");
                RTOS::delay(processing_time);

                print("PROCESSOR", "Processed: temp=", data.temperature,
                      "°C, humidity=", data.humidity, "%");
            } else {
                print("PROCESSOR", "Failed to receive data from queue");
            }
        } else {
            print("PROCESSOR", "Timeout waiting for data");
        }
    }
}

// ============================================================================
// Task 3: Event Monitor (Normal Priority)
// ============================================================================
/// Monitors system events

void monitor_task() {
    print("MONITOR", "Task started");

    while (true) {
        // Wait for any event (2 second timeout)
        core::u32 events =
            system_events.wait_any(EVENT_DATA_READY | EVENT_THRESHOLD_EXCEEDED | EVENT_ERROR,
                                   false,  // Don't clear flags
                                   2000    // 2 second timeout
            );

        if (events != 0) {
            if (events & EVENT_DATA_READY) {
                print("MONITOR", "Event: DATA_READY");
                system_events.clear(EVENT_DATA_READY);
            }

            if (events & EVENT_THRESHOLD_EXCEEDED) {
                print("MONITOR", "⚠️  ALERT: Temperature threshold exceeded!");
                system_events.clear(EVENT_THRESHOLD_EXCEEDED);
            }

            if (events & EVENT_ERROR) {
                print("MONITOR", "❌ ERROR event detected!");
                system_events.clear(EVENT_ERROR);
            }
        } else {
            print("MONITOR", "No events (timeout)");
        }
    }
}

// ============================================================================
// Task 4: Heartbeat (Low Priority)
// ============================================================================
/// Periodic heartbeat task

void heartbeat_task() {
    print("HEARTBEAT", "Task started");

    core::u32 beat_count = 0;

    while (true) {
        print("HEARTBEAT", "💓 Beat #", beat_count, " - System OK");
        beat_count++;

        // Heartbeat every 2 seconds
        RTOS::delay(2000);
    }
}

// ============================================================================
// Task 5: Statistics (Low Priority)
// ============================================================================
/// Displays RTOS statistics periodically

void stats_task() {
    print("STATS", "Task started");

    while (true) {
        // Wait 5 seconds
        RTOS::delay(5000);

        {
            LockGuard lock(console_mutex);
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "  RTOS Statistics (" << (systick::micros() / 1000) << "ms uptime)"
                      << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            std::cout << "  Queue occupancy: " << sensor_queue.count() << " / "
                      << sensor_queue.capacity() << std::endl;
            std::cout << "  Tick count: " << RTOS::get_tick_count() << " ms" << std::endl;
            std::cout << std::string(60, '=') << "\n" << std::endl;
        }
    }
}

// ============================================================================
// Task Declarations
// ============================================================================

// Task priorities: 0 (lowest) to 7 (highest)
Task<2048, Priority::High> task_sensor(sensor_task, "Sensor");
Task<2048, Priority::Normal> task_processor(processor_task, "Processor");
Task<2048, Priority::Normal> task_monitor(monitor_task, "Monitor");
Task<2048, Priority::Low> task_heartbeat(heartbeat_task, "Heartbeat");
Task<2048, Priority::Lowest> task_stats(stats_task, "Stats");

// ============================================================================
// Main Entry Point
// ============================================================================

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║            Alloy RTOS - Host Platform Demo                  ║
║                                                              ║
║  Testing real-time operating system on PC!                  ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;

    // Initialize SysTick (required for RTOS timing)
    auto result = hal::host::SystemTick::init();
    if (!result.is_ok()) {
        std::cerr << "Failed to initialize SysTick!" << std::endl;
        return 1;
    }

    std::cout << "✓ SysTick initialized" << std::endl;
    std::cout << "✓ 5 tasks created:" << std::endl;
    std::cout << "    - Sensor      (Priority: High)" << std::endl;
    std::cout << "    - Processor   (Priority: Normal)" << std::endl;
    std::cout << "    - Monitor     (Priority: Normal)" << std::endl;
    std::cout << "    - Heartbeat   (Priority: Low)" << std::endl;
    std::cout << "    - Statistics  (Priority: Lowest)" << std::endl;
    std::cout << "\nStarting RTOS scheduler...\n" << std::endl;

    // Start RTOS (never returns)
    RTOS::start();

    return 0;  // Never reached
}
