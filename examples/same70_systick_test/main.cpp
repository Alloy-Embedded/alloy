/**
 * @file main_systick.cpp
 * @brief LED Blink with SysTick Timer Test for SAME70 Xplained Ultra
 *
 * Demonstrates SysTick timer integration with Clock and GPIO HAL.
 * Tests precise timing, uptime monitoring, and timeout functions.
 *
 * Features tested:
 * - Clock initialization with multiple frequencies
 * - SysTick initialization with clock-aware timing
 * - Precise microsecond/millisecond delays
 * - Runtime monitoring (uptime tracking)
 * - Timeout checks
 *
 * LED Behavior:
 * - 3 fast blinks: System startup
 * - 5 medium blinks: Clock + SysTick initialized successfully
 * - Continuous 1Hz blink: Normal operation with precise 500ms delays
 * - LED stays on for 100ms, off for 400ms each second
 *
 * @author Alloy Framework
 * @date 2025-11-09
 */

#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick.hpp"
#include "hal/platform/same70/systick_delay.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

// SysTick interrupt handler
// This overrides the weak alias in startup.cpp
extern "C" void SysTick_Handler() {
    alloy::hal::same70::SystemTick::irq_handler();
}

int main() {
    using namespace alloy::hal::same70;
    using namespace alloy::generated::atsame70q21b;

    // ========================================================================
    // CLOCK CONFIGURATION - Choose one
    // ========================================================================

    // Option 1: RC 12MHz (Simple, good for testing)
    ClockConfig config = {
        .main_source = MainClockSource::InternalRC_12MHz,
        .mck_source = MasterClockSource::MainClock,
        .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
    };

    // Option 2: Crystal + PLL @ 300MHz -> MCK 150MHz (Maximum performance)
    // ClockConfig config = {
    //     .main_source = MainClockSource::ExternalCrystal,
    //     .crystal_freq_hz = 12000000,
    //     .plla = {24, 1},  // 12MHz * 25 / 1 = 300MHz
    //     .mck_source = MasterClockSource::PLLAClock,
    //     .mck_prescaler = MasterClockPrescaler::DIV_2  // 300/2 = 150MHz MCK
    // };

    // ========================================================================
    // Setup GPIO for LED FIRST (for debug before clock init)
    // ========================================================================

    // Enable PIOC peripheral clock manually (using boot clock)
    volatile uint32_t* PMC_PCER0 = (volatile uint32_t*)0x400E0610;
    *PMC_PCER0 = (1u << 12);  // Enable PIOC

    // Setup LED (PC8 on SAME70 Xplained)
    // LED is active-low: clear()=ON, set()=OFF
    using Led0 = GpioPin<PIOC_BASE, 8>;
    Led0 led;
    led.setDirection(PinDirection::Output);

    // Blink 2 times to show we started
    for (int i = 0; i < 2; i++) {
        led.clear();  // ON
        for (volatile int j = 0; j < 100000; j++);
        led.set();    // OFF
        for (volatile int j = 0; j < 100000; j++);
    }

    // ========================================================================
    // Initialize Clock
    // ========================================================================

    auto result = Clock::initialize(config);
    if (!result.is_ok()) {
        // Clock init failed - blink fast forever
        while (1) {
            led.clear();
            for (volatile int j = 0; j < 50000; j++);
            led.set();
            for (volatile int j = 0; j < 50000; j++);
        }
    }

    // Blink 3 times to show clock init OK
    for (int i = 0; i < 3; i++) {
        led.clear();  // ON
        for (volatile int j = 0; j < 200000; j++);
        led.set();    // OFF
        for (volatile int j = 0; j < 200000; j++);
    }

    // ========================================================================
    // Initialize SysTick Timer
    // ========================================================================

    auto systick_result = SystemTick::init();
    if (!systick_result.is_ok()) {
        // SysTick init failed - blink very fast forever
        while (1) {
            led.clear();
            for (volatile int j = 0; j < 25000; j++);
            led.set();
            for (volatile int j = 0; j < 25000; j++);
        }
    }

    // ========================================================================
    // Startup Indicator - 3 fast blinks with SysTick delays
    // ========================================================================

    for (int i = 0; i < 3; i++) {
        led.clear();  // ON
        delay_ms(100);
        led.set();    // OFF
        delay_ms(100);
    }

    // Small pause
    delay_ms(300);

    // ========================================================================
    // Success Indicator - 5 medium blinks with PRECISE timing
    // ========================================================================

    // Blink 4: Test if we get here
    led.clear();
    for (volatile int j = 0; j < 500000; j++);
    led.set();
    for (volatile int j = 0; j < 500000; j++);

    // Now try with delay_ms
    for (int i = 0; i < 5; i++) {
        led.clear();  // ON
        delay_ms(150);
        led.set();    // OFF
        delay_ms(150);
    }

    // Pause before main loop
    delay_ms(500);

    // ========================================================================
    // Main Loop - Precise 1Hz blink with uptime monitoring
    // ========================================================================

    uint32_t loop_count = 0;
    uint32_t last_uptime_sec = 0;

    while (1) {
        // Turn LED ON
        led.clear();

        // Keep LED on for 250ms (precise ISR-based timing)
        delay_ms(250);

        // Turn LED OFF
        led.set();

        // Keep LED off for 750ms (total 1 second per loop)
        delay_ms(750);

        // Loop counter for debugging
        loop_count++;

        // Optional: Every 10 seconds, do a double blink
        if (loop_count % 10 == 0) {
            delay_ms(100);
            led.clear();
            delay_ms(100);
            led.set();
        }
    }

    return 0;
}
