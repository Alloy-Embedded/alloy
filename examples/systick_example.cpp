/// Example: Using SysTick for Time Tracking
///
/// This example demonstrates how to use the shared ARM SysTick
/// implementation for system time tracking (ideal for RTOS timebase).
///
/// Hardware: Any ARM Cortex-M (SAME70, STM32, SAMD21, RP2040)
///
/// Features demonstrated:
/// - Initialize SysTick with 1ms tick rate
/// - Implement interrupt handler
/// - Track elapsed time
/// - Use as RTOS timebase
///
/// Platform Selection:
/// The code automatically selects the correct SysTick implementation
/// based on the defined MCU macro. Supported platforms:
/// - SAME70 (300MHz) - Atmel/Microchip ARM Cortex-M7
/// - STM32F103 (72MHz) - ST ARM Cortex-M3 (Blue Pill)
/// - SAMD21 (48MHz) - Microchip ARM Cortex-M0+
/// - RP2040 (125MHz) - Raspberry Pi ARM Cortex-M0+

// Platform-specific includes
#if defined(MCU_SAME70)
    #include "hal/vendors/atmel/same70/systick.hpp"
    using namespace alloy::hal::atmel::same70;
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F1)
    #include "hal/vendors/st/stm32f1/systick.hpp"
    using namespace alloy::hal::st::stm32f1;
#elif defined(MCU_SAMD21)
    #include "hal/vendors/microchip/samd21/systick_time.hpp"
    using namespace alloy::hal::microchip::samd21;
#elif defined(MCU_RP2040)
    #include "hal/vendors/raspberrypi/rp2040/systick_time.hpp"
    using namespace alloy::hal::raspberrypi::rp2040;
#else
    #error "Unsupported MCU - please add SysTick support for your platform"
#endif

// IMPORTANT: Implement the SysTick interrupt handler
// This gets called automatically every 1ms (if configured for 1000Hz)
extern "C" void SysTick_Handler() {
    // Increment the tick counter
    systick::tick();

    // You can add your own code here:
    // - Toggle LED every N ticks
    // - Wake up tasks (if using RTOS)
    // - Update software timers
}

void example_basic_usage() {
    // 1. Initialize SysTick for 1ms ticks (1000 Hz)
    systick::init(1000);

    // Now SysTick is running! The interrupt fires every 1ms

    while (true) {
        // Read elapsed time
        uint64_t ticks = systick::get_ticks();
        uint64_t ms = systick::get_time_ms();
        uint64_t us = systick::get_time_us();
        uint32_t s = systick::get_time_s();

        // Use time for your application
        // ...

        // Delay using the tick counter
        systick::delay_ms(1000);  // Wait 1 second
    }
}

void example_periodic_task() {
    systick::init(1000);  // 1ms ticks

    uint64_t last_run = 0;
    const uint64_t period_ms = 100;  // Run every 100ms

    while (true) {
        uint64_t now = systick::get_time_ms();

        if ((now - last_run) >= period_ms) {
            // Task runs every 100ms
            // Do your periodic work here

            last_run = now;
        }

        // Other work...
    }
}

void example_timeout() {
    systick::init(1000);

    const uint64_t timeout_ms = 5000;  // 5 second timeout
    uint64_t start = systick::get_time_ms();

    while (true) {
        // Check for timeout
        if ((systick::get_time_ms() - start) > timeout_ms) {
            // Timeout expired!
            break;
        }

        // Do work with timeout protection
        // ...
    }
}

void example_profiling() {
    systick::init(1000);

    // Measure execution time
    uint64_t start = systick::get_time_us();

    // Code to profile
    for (int i = 0; i < 1000; i++) {
        // Some work...
    }

    uint64_t end = systick::get_time_us();
    uint64_t elapsed = end - start;

    // elapsed contains execution time in microseconds
}

// Example: Custom tick rate for high-resolution timing
void example_high_resolution() {
    // 10kHz = 100us ticks (10x more precise than standard 1ms)
    systick::init(10000);

    // Now you get 100us resolution
    uint64_t time_100us = systick::get_ticks();
}

// Example: FreeRTOS integration
// In FreeRTOSConfig.h, set:
// #define configTICK_RATE_HZ 1000
// #define configUSE_PREEMPTION 1

// Then implement vApplicationTickHook() if needed:
extern "C" void vApplicationTickHook() {
    // Called each tick from FreeRTOS
    // Can be used for additional tick processing
}

// The SysTick_Handler serves as FreeRTOS tick:
// extern "C" void SysTick_Handler() {
//     systick::tick();        // Update our counter
//     xPortSysTickHandler();  // FreeRTOS handler
// }

int main() {
    example_basic_usage();
    return 0;
}
