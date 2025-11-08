/// RTOS Event Flags Example - Multi-Sensor Coordination
///
/// Demonstrates event flags for coordinating multiple sensors/events:
/// - 3 "sensor" tasks set individual event flags
/// - wait_any(): Processes whichever sensor is ready first
/// - wait_all(): Processes only when all sensors are ready
///
/// Hardware: STM32F103 (Bluepill)
/// - LED patterns indicate which events are active

#include "rtos/event.hpp"
#include "rtos/rtos.hpp"

#include "core/types.hpp"

#include "stm32f103c8/board.hpp"

using namespace alloy;
using namespace alloy::rtos;

// Define event flags (one bit per event)
constexpr core::u32 EVENT_SENSOR1_READY = (1 << 0);
constexpr core::u32 EVENT_SENSOR2_READY = (1 << 1);
constexpr core::u32 EVENT_SENSOR3_READY = (1 << 2);
constexpr core::u32 EVENT_ALL_SENSORS =
    EVENT_SENSOR1_READY | EVENT_SENSOR2_READY | EVENT_SENSOR3_READY;

/// Event flags for sensor coordination
EventFlags sensor_events;

/// Sensor 1 task - simulates fast sensor (100ms)
///
/// Sets EVENT_SENSOR1_READY every 100ms
void sensor1_task_func() {
    while (1) {
        // Simulate sensor reading
        RTOS::delay(100);

        // Signal sensor 1 ready
        sensor_events.set(EVENT_SENSOR1_READY);

        // Quick blink to indicate sensor 1 active
        Board::Led::on();
        RTOS::delay(20);
        Board::Led::off();
    }
}

/// Sensor 2 task - simulates medium sensor (200ms)
///
/// Sets EVENT_SENSOR2_READY every 200ms
void sensor2_task_func() {
    while (1) {
        // Simulate sensor reading
        RTOS::delay(200);

        // Signal sensor 2 ready
        sensor_events.set(EVENT_SENSOR2_READY);

        // Double blink to indicate sensor 2 active
        for (int i = 0; i < 2; i++) {
            Board::Led::on();
            RTOS::delay(20);
            Board::Led::off();
            RTOS::delay(20);
        }
    }
}

/// Sensor 3 task - simulates slow sensor (300ms)
///
/// Sets EVENT_SENSOR3_READY every 300ms
void sensor3_task_func() {
    while (1) {
        // Simulate sensor reading
        RTOS::delay(300);

        // Signal sensor 3 ready
        sensor_events.set(EVENT_SENSOR3_READY);

        // Triple blink to indicate sensor 3 active
        for (int i = 0; i < 3; i++) {
            Board::Led::on();
            RTOS::delay(20);
            Board::Led::off();
            RTOS::delay(20);
        }
    }
}

/// Quick processing task - wait_any()
///
/// Processes whichever sensor is ready first.
/// Demonstrates wait_any() - returns as soon as ANY event is set.
void quick_process_task_func() {
    while (1) {
        // Wait for ANY sensor (up to 500ms)
        core::u32 events = sensor_events.wait_any(EVENT_ALL_SENSORS, 500, true);

        if (events != 0) {
            // At least one sensor ready - show which one(s)
            int blink_count = 0;
            if (events & EVENT_SENSOR1_READY)
                blink_count++;
            if (events & EVENT_SENSOR2_READY)
                blink_count++;
            if (events & EVENT_SENSOR3_READY)
                blink_count++;

            // Fast blinks = quick processing
            for (int i = 0; i < blink_count; i++) {
                Board::Led::on();
                RTOS::delay(15);
                Board::Led::off();
                RTOS::delay(15);
            }

            // Process data (simulated)
            RTOS::delay(30);
        } else {
            // Timeout - no sensors ready
            Board::Led::on();
            RTOS::delay(5);
            Board::Led::off();
        }

        RTOS::delay(50);
    }
}

/// Fusion processing task - wait_all()
///
/// Waits for ALL sensors to be ready before processing.
/// Demonstrates wait_all() - only returns when ALL events are set.
void fusion_process_task_func() {
    while (1) {
        // Wait for ALL sensors ready (up to 1 second)
        core::u32 events = sensor_events.wait_all(EVENT_ALL_SENSORS, 1000, true);

        if (events == EVENT_ALL_SENSORS) {
            // All sensors ready - perform fusion
            // Long blink indicates fusion processing
            Board::Led::on();
            RTOS::delay(150);
            Board::Led::off();

            // Simulate fusion processing
            RTOS::delay(50);
        } else {
            // Timeout - not all sensors ready
            // 4 quick blinks = timeout
            for (int i = 0; i < 4; i++) {
                Board::Led::on();
                RTOS::delay(10);
                Board::Led::off();
                RTOS::delay(10);
            }
        }

        RTOS::delay(100);
    }
}

/// Idle task
void idle_task_func() {
    while (1) {
        __asm volatile("wfi");
    }
}

// Create tasks
Task<256, Priority::Normal> sensor1_task(sensor1_task_func, "Sensor1");
Task<256, Priority::Normal> sensor2_task(sensor2_task_func, "Sensor2");
Task<256, Priority::Normal> sensor3_task(sensor3_task_func, "Sensor3");
Task<512, Priority::High> quick_task(quick_process_task_func, "QuickProc");
Task<512, Priority::High> fusion_task(fusion_process_task_func, "FusionProc");
Task<256, Priority::Idle> idle_task(idle_task_func, "Idle");

int main() {
    // Initialize board (includes SysTick)
    Board::initialize();

    // Initialize LED
    Board::Led::init();

    // Start RTOS (never returns)
    RTOS::start();

    return 0;
}

// Weak symbols for startup code
extern "C" {
void SystemInit() {
    // Running on default HSI (8MHz)
}
}
