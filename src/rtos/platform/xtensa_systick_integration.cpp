/// ESP32/Xtensa SysTick Integration with RTOS
///
/// Integrates ESP32 timer with RTOS scheduler.
/// Uses esp_timer for periodic tick generation.

#ifdef ESP32

#include "rtos/rtos.hpp"
#include "rtos/platform/xtensa_context.hpp"
#include "esp_timer.h"
#include "esp_attr.h"

namespace alloy::rtos {

namespace xtensa {

// Global timer handle
static esp_timer_handle_t g_rtos_timer = nullptr;
static bool g_timer_initialized = false;

// Timer callback - called every 1ms
static void IRAM_ATTR rtos_timer_callback(void* arg) {
    // Call RTOS scheduler tick
    RTOS::tick();

    // Trigger context switch if needed
    if (RTOS::need_context_switch()) {
        // On ESP32, we can use portYIELD_FROM_ISR for context switch
        // but for pure RTOS implementation, we set flag and handle later
        g_scheduler.need_context_switch = false;

        // Note: Full context switch would happen here
        // For now, we rely on cooperative scheduling in ESP32 environment
    }
}

void init_rtos_timer() {
    if (g_timer_initialized) return;

    // Create periodic timer for 1ms tick
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = rtos_timer_callback;
    timer_args.arg = nullptr;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "alloy_rtos_tick";
    timer_args.skip_unhandled_events = false;

    esp_timer_create(&timer_args, &g_rtos_timer);

    // Start periodic timer (1000 microseconds = 1ms)
    esp_timer_start_periodic(g_rtos_timer, 1000);

    g_timer_initialized = true;
}

void stop_rtos_timer() {
    if (g_timer_initialized && g_rtos_timer != nullptr) {
        esp_timer_stop(g_rtos_timer);
        esp_timer_delete(g_rtos_timer);
        g_rtos_timer = nullptr;
        g_timer_initialized = false;
    }
}

} // namespace xtensa

// Override RTOS::start() for ESP32
// This is called instead of the generic implementation
namespace RTOS {

[[noreturn]] void start_esp32() {
    // Initialize scheduler
    scheduler::init();

    // Initialize ESP32 timer for RTOS tick
    xtensa::init_rtos_timer();

    // Get first task
    g_scheduler.current_task = g_scheduler.ready_queue.get_highest_priority();

    if (g_scheduler.current_task == nullptr) {
        // No tasks - should not happen
        while (1) {
            // Wait
            vTaskDelay(1);
        }
    }

    g_scheduler.started = true;
    g_scheduler.current_task->state = TaskState::Running;

    // Start first task
    start_first_task();

    // Never reached
    while (1);
}

} // namespace RTOS

} // namespace alloy::rtos

#endif // ESP32
