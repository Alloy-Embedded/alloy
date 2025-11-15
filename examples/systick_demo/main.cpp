/**
 * @file main.cpp
 * @brief SysTick Modes and RTOS Integration Demo
 *
 * Demonstrates different SysTick tick rates and RTOS integration patterns.
 * Shows trade-offs between tick resolution, interrupt overhead, and accuracy.
 *
 * ## Hardware Setup
 * - Connect LED to board (uses built-in LED)
 * - Optional: Logic analyzer to measure tick accuracy
 *
 * ## Modes Demonstrated
 * 1. Standard 1ms tick (RTOS-compatible, low overhead)
 * 2. High-resolution 100us tick (precise timing, higher overhead)
 * 3. RTOS integration patterns (hook points for scheduler)
 * 4. Tick resolution trade-off analysis
 *
 * ## Learning Objectives
 * - Understanding tick resolution vs CPU overhead
 * - RTOS tick integration patterns
 * - Choosing appropriate tick rate for application
 * - Measuring interrupt timing with hardware
 */

#include "board/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

// =============================================================================
// RTOS Integration Hooks (Example - not a real RTOS)
// =============================================================================

/**
 * @brief Simulated RTOS state
 *
 * In a real RTOS, this would be the scheduler state.
 * We simulate basic tick counting for demonstration.
 */
namespace rtos {
    static volatile u32 tick_count = 0;
    static volatile bool scheduler_enabled = false;

    /**
     * @brief RTOS tick hook - called from SysTick ISR
     *
     * In a real RTOS, this would:
     * - Increment RTOS tick counter
     * - Check for sleeping tasks to wake
     * - Trigger context switch if needed
     * - Update software timers
     */
    void tick() {
        tick_count++;

        // In real RTOS: Check if scheduler should run
        if (scheduler_enabled) {
            // Would trigger PendSV for context switch
            // __asm volatile ("DSB");
            // SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }

    /**
     * @brief Enable RTOS scheduler
     */
    void enable_scheduler() {
        scheduler_enabled = true;
    }

    /**
     * @brief Get RTOS tick count
     */
    u32 get_ticks() {
        return tick_count;
    }
}

// =============================================================================
// Interrupt Statistics (for overhead measurement)
// =============================================================================

namespace stats {
    static volatile u32 interrupt_count = 0;
    static volatile u32 max_interrupt_time_us = 0;

    void increment_interrupt() {
        interrupt_count++;
    }

    void record_interrupt_time(u32 time_us) {
        if (time_us > max_interrupt_time_us) {
            max_interrupt_time_us = time_us;
        }
    }

    u32 get_interrupt_count() {
        return interrupt_count;
    }

    u32 get_max_interrupt_time() {
        return max_interrupt_time_us;
    }

    void reset() {
        interrupt_count = 0;
        max_interrupt_time_us = 0;
    }
}

// =============================================================================
// Demo 1: Standard 1ms Tick (RTOS-Compatible)
// =============================================================================

/**
 * @brief Demonstrate standard 1ms tick mode
 *
 * This is the most common configuration:
 * - Low interrupt overhead (~0.1% CPU @ 100 MHz)
 * - RTOS-compatible (FreeRTOS, Zephyr use 1ms default)
 * - Good balance of resolution and efficiency
 * - Suitable for most embedded applications
 *
 * Trade-offs:
 * ✅ Low CPU overhead
 * ✅ RTOS-friendly
 * ✅ Adequate for human interface
 * ⚠️ ±1ms timing resolution
 */
void demo_1ms_tick_mode() {
    // Board already initialized with 1ms tick in board::init()
    // This is the default mode

    // Demonstrate RTOS hook integration
    rtos::enable_scheduler();

    // Run for 5 seconds and measure overhead
    stats::reset();
    u32 start = SysTickTimer::millis<board::BoardSysTick>();
    u32 last_print = start;

    while (SysTickTimer::millis<board::BoardSysTick>() - start < 5000) {
        // Blink LED at 2 Hz (every 500ms)
        u32 now = SysTickTimer::millis<board::BoardSysTick>();
        if ((now / 500) % 2 == 0) {
            board::led::on();
        } else {
            board::led::off();
        }

        // Print stats every second (simulated - would use UART in real app)
        if (now - last_print >= 1000) {
            // In real app: printf("RTOS ticks: %u, Interrupts: %u\n",
            //                     rtos::get_ticks(), stats::get_interrupt_count());
            last_print = now;
        }

        // Simulate RTOS task work
        SysTickTimer::delay_ms<board::BoardSysTick>(10);
    }

    board::led::off();

    // Expected results at 1ms tick:
    // - ~5000 interrupts in 5 seconds
    // - <0.1% CPU overhead (assuming <10us per ISR at 100 MHz)
    // - RTOS tick count = millis (1:1 ratio)
}

// =============================================================================
// Demo 2: High-Resolution 100us Tick
// =============================================================================

/**
 * @brief Demonstrate high-resolution 100us tick mode
 *
 * High-precision timing for:
 * - Motor control (PWM updates)
 * - Audio processing
 * - Sensor sampling (IMU at 1 kHz)
 * - Protocol timing (CAN, industrial protocols)
 *
 * Trade-offs:
 * ✅ ±100us timing resolution (10x better)
 * ✅ Precise control loops possible
 * ⚠️ Higher CPU overhead (~1% at 100 MHz)
 * ⚠️ More interrupt latency variation
 * ❌ Not recommended for standard RTOS (too frequent context switches)
 *
 * Note: This demo shows the pattern but doesn't actually reconfigure
 *       SysTick since that would affect other demos.
 */
void demo_100us_high_resolution_tick() {
    // In a real application, you would reconfigure SysTick:
    // SysTickTimer::init_us<board::BoardSysTick>(100);  // 100us tick

    // For demonstration, we simulate high-res timing using delay_us
    board::led::off();

    // Simulate 1 kHz control loop (1ms period)
    u32 start = SysTickTimer::millis<board::BoardSysTick>();

    while (SysTickTimer::millis<board::BoardSysTick>() - start < 3000) {
        u64 loop_start = SysTickTimer::micros<board::BoardSysTick>();

        // Toggle LED at 1 kHz (visible as dim glow)
        board::led::toggle();

        // Simulate control loop work (e.g., PID calculation)
        // In real app: read_sensor(), update_pid(), set_actuator()
        volatile int dummy_work = 0;
        for (int i = 0; i < 100; i++) {
            dummy_work += i;  // Simulate computation
        }
        (void)dummy_work;

        // Wait for next 1ms period (using 100us resolution)
        while (SysTickTimer::micros<board::BoardSysTick>() - loop_start < 1000) {
            // Busy wait (in real app, could do other work)
        }

        // With 100us tick, we can maintain precise 1ms loop timing
        // With 1ms tick, jitter would be up to ±1ms
    }

    board::led::off();

    // Expected with 100us tick:
    // - ~50,000 interrupts in 5 seconds
    // - ~1% CPU overhead (assuming <10us per ISR at 100 MHz)
    // - Precise loop timing (±100us vs ±1ms with 1ms tick)
}

// =============================================================================
// Demo 3: RTOS Integration Pattern
// =============================================================================

/**
 * @brief Demonstrate RTOS tick integration pattern
 *
 * Shows how to integrate SysTick with an RTOS scheduler.
 * This pattern works with FreeRTOS, Zephyr, CMSIS-RTOS, etc.
 *
 * Integration points:
 * 1. SysTick_Handler calls RTOS tick function
 * 2. RTOS tick updates scheduler state
 * 3. Context switch triggered if needed (via PendSV)
 * 4. Application uses RTOS timing APIs
 */
void demo_rtos_integration_pattern() {
    board::led::off();

    // In a real RTOS application, the integration would be in board.cpp:
    //
    // extern "C" void SysTick_Handler() {
    //     // 1. Update HAL tick (for delays, timeouts)
    //     board::BoardSysTick::increment_tick();
    //
    //     // 2. Update RTOS tick (for scheduler)
    //     #ifdef ALLOY_RTOS_ENABLED
    //         if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    //             xPortSysTickHandler();  // FreeRTOS
    //             // or: sys_clock_announce(1);  // Zephyr
    //             // or: osRtxTick_Handler();    // CMSIS-RTOS
    //         }
    //     #endif
    //
    //     // 3. Custom application hooks (optional)
    //     // app::on_systick();
    // }

    // Demonstrate how application would use both HAL and RTOS timing:
    u32 rtos_tick_start = rtos::get_ticks();
    u32 hal_tick_start = SysTickTimer::millis<board::BoardSysTick>();

    for (int i = 0; i < 10; i++) {
        // HAL delay (uses SysTick directly)
        SysTickTimer::delay_ms<board::BoardSysTick>(100);

        // In RTOS app, you would use RTOS delay:
        // vTaskDelay(pdMS_TO_TICKS(100));  // FreeRTOS
        // k_sleep(K_MSEC(100));            // Zephyr

        // Both HAL and RTOS ticks should advance together
        board::led::toggle();
    }

    u32 rtos_elapsed = rtos::get_ticks() - rtos_tick_start;
    u32 hal_elapsed = SysTickTimer::millis<board::BoardSysTick>() - hal_tick_start;

    // They should match (both use same SysTick counter)
    // rtos_elapsed ≈ hal_elapsed ≈ 1000ms
    (void)rtos_elapsed;
    (void)hal_elapsed;

    board::led::off();
}

// =============================================================================
// Demo 4: Tick Resolution Trade-off Analysis
// =============================================================================

/**
 * @brief Demonstrate tick resolution vs overhead trade-offs
 *
 * Compares different tick rates:
 * - 10ms: Very low overhead, coarse resolution
 * - 1ms: Standard RTOS tick (recommended)
 * - 100us: High resolution, moderate overhead
 * - 10us: Very high resolution, high overhead
 *
 * Visual indicator: LED blink rate represents interrupt frequency
 */
void demo_tick_resolution_tradeoffs() {
    struct TickConfig {
        u32 period_us;
        const char* name;
        u32 blink_delay_ms;
    };

    // Different tick configurations (for demonstration)
    const TickConfig configs[] = {
        {10000, "10ms tick (low res)",     2000},  // Slow blink = low freq
        {1000,  "1ms tick (standard)",     500},   // Medium blink = medium freq
        {100,   "100us tick (high res)",   100},   // Fast blink = high freq
        {10,    "10us tick (very high)",   50}     // Very fast = very high freq
    };

    for (const auto& config : configs) {
        // Visual indicator of interrupt frequency
        // (In real app, would actually reconfigure SysTick)
        for (int i = 0; i < 10; i++) {
            board::led::toggle();
            SysTickTimer::delay_ms<board::BoardSysTick>(config.blink_delay_ms);
        }

        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(1000);  // Pause between configs

        // Analysis of each configuration:
        //
        // 10ms tick (100 Hz):
        // - CPU overhead: <0.01% (minimal)
        // - Timing resolution: ±10ms
        // - Use case: Low-power sensors, UI updates
        // - RTOS: Too coarse for most applications
        //
        // 1ms tick (1 kHz):
        // - CPU overhead: ~0.1% (very low)
        // - Timing resolution: ±1ms
        // - Use case: Standard RTOS, general embedded
        // - RTOS: Recommended default (FreeRTOS, Zephyr)
        //
        // 100us tick (10 kHz):
        // - CPU overhead: ~1% (acceptable)
        // - Timing resolution: ±100us
        // - Use case: Motor control, audio, fast sensors
        // - RTOS: Possible but increases context switch overhead
        //
        // 10us tick (100 kHz):
        // - CPU overhead: ~10% (high!)
        // - Timing resolution: ±10us
        // - Use case: Very fast control loops, real-time DSP
        // - RTOS: Not recommended (too much overhead)
        // - Alternative: Use hardware timers instead
    }
}

// =============================================================================
// Main Application
// =============================================================================

/**
 * @brief Main entry point - SysTick modes demonstration
 */
int main() {
    // Initialize board with standard 1ms tick
    board::init();

    // Infinite loop demonstrating different SysTick modes
    while (true) {
        // Demo 1: Standard 1ms tick (RTOS-compatible)
        // LED: 2 Hz blink for 5 seconds
        demo_1ms_tick_mode();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Demo 2: High-resolution 100us tick simulation
        // LED: 1 kHz toggle (dim glow) for 3 seconds
        demo_100us_high_resolution_tick();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Demo 3: RTOS integration pattern
        // LED: 10 toggles at 100ms intervals
        demo_rtos_integration_pattern();
        SysTickTimer::delay_ms<board::BoardSysTick>(2000);

        // Demo 4: Tick resolution trade-offs
        // LED: Variable blink rate showing interrupt frequency
        demo_tick_resolution_tradeoffs();
        SysTickTimer::delay_ms<board::BoardSysTick>(5000);
    }

    return 0;
}

// =============================================================================
// Modified SysTick_Handler for RTOS Integration
// =============================================================================

/**
 * @brief Example SysTick_Handler with RTOS integration
 *
 * This would replace the handler in board.cpp for RTOS builds.
 * Normally defined in board.cpp, shown here for documentation.
 */
#if 0  // Not compiled - for documentation only
extern "C" void SysTick_Handler() {
    // Measure interrupt time (for overhead analysis)
    u64 isr_start = SysTickTimer::micros<board::BoardSysTick>();

    // 1. Update HAL tick counter
    board::BoardSysTick::increment_tick();

    // 2. Record interrupt statistics
    stats::increment_interrupt();

    // 3. RTOS tick hook (if RTOS enabled)
    #ifdef ALLOY_RTOS_ENABLED
        rtos::tick();
    #endif

    // 4. Measure interrupt duration
    u64 isr_time = SysTickTimer::micros<board::BoardSysTick>() - isr_start;
    stats::record_interrupt_time(static_cast<u32>(isr_time));

    // Typical ISR time:
    // - Without RTOS: 1-3 us @ 100 MHz (just increment counter)
    // - With RTOS: 5-15 us @ 100 MHz (scheduler overhead)
    // - Max acceptable: <10% of tick period
    //   (e.g., <100us for 1ms tick, <10us for 100us tick)
}
#endif
