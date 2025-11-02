/// RTOS Queue Example - Producer/Consumer Pattern
///
/// Demonstrates message queues for inter-task communication:
/// - Producer task: Reads "sensor" data and sends to queue
/// - Consumer task: Receives data from queue and displays on LED
/// - Idle task: Runs when no other tasks are ready
///
/// This shows how tasks can safely communicate using type-safe queues.
///
/// Hardware: STM32F103 (Bluepill)
/// - LED on PC13 blinks at rate determined by queue messages

#include "stm32f103c8/board.hpp"
#include "rtos/rtos.hpp"
#include "rtos/queue.hpp"
#include "core/types.hpp"

using namespace alloy;
using namespace alloy::rtos;

/// Message structure for sensor data
struct SensorData {
    core::u32 timestamp;      ///< Time when data was read (microseconds)
    core::i16 value;          ///< Sensor value (simulated)
    core::u8  sequence;       ///< Sequence number
    core::u8  padding;        ///< Padding for alignment
};

// Create message queue with capacity for 4 messages
Queue<SensorData, 4> sensor_queue;

// Shared sequence counter
static core::u8 sequence_counter = 0;

/// Producer Task: Simulates sensor readings
///
/// Reads "sensor" data every 200ms and sends to queue.
/// If queue is full, blocks until space is available.
void producer_task_func() {
    while (1) {
        // Simulate sensor reading
        SensorData data;
        data.timestamp = systick::micros();
        data.value = static_cast<core::i16>((sequence_counter * 100) % 1000);
        data.sequence = sequence_counter++;
        data.padding = 0;

        // Send to queue (block if full, timeout after 1 second)
        bool sent = sensor_queue.send(data, 1000);

        if (sent) {
            // Success - blink LED briefly to indicate send
            Board::Led::on();
            RTOS::delay(10);
            Board::Led::off();
        }

        // Produce data every 200ms
        RTOS::delay(200);
    }
}

/// Consumer Task: Processes sensor data
///
/// Receives sensor data from queue and "processes" it.
/// Processing time varies based on value (simulates variable workload).
void consumer_task_func() {
    while (1) {
        SensorData data;

        // Receive from queue (block if empty, timeout after 500ms)
        bool received = sensor_queue.receive(data, 500);

        if (received) {
            // Process data - blink pattern based on sequence number
            // Even sequences: 1 long blink
            // Odd sequences: 2 short blinks
            if (data.sequence % 2 == 0) {
                Board::Led::on();
                RTOS::delay(300);
                Board::Led::off();
            } else {
                Board::Led::on();
                RTOS::delay(100);
                Board::Led::off();
                RTOS::delay(50);
                Board::Led::on();
                RTOS::delay(100);
                Board::Led::off();
            }

            // Variable processing time (50-150ms)
            core::u32 process_time = 50 + (data.value % 100);
            RTOS::delay(process_time);
        } else {
            // Timeout - no data received
            // Indicate with 3 quick blinks
            for (int i = 0; i < 3; i++) {
                Board::Led::on();
                RTOS::delay(50);
                Board::Led::off();
                RTOS::delay(50);
            }
        }
    }
}

/// Idle task (runs when no other tasks ready)
void idle_task_func() {
    while (1) {
        // Enter low-power mode (wait for interrupt)
        __asm volatile("wfi");
    }
}

// Create tasks with different priorities
Task<512, Priority::Normal>  producer_task(producer_task_func, "Producer");
Task<512, Priority::High>    consumer_task(consumer_task_func, "Consumer");
Task<256, Priority::Idle>    idle_task(idle_task_func, "Idle");

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
