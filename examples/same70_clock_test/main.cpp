/**
 * @file clock_test.cpp
 * @brief Clock Configuration Test Examples for SAME70
 *
 * This file demonstrates different clock configurations.
 * Uncomment the configuration you want to test.
 */

#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

int main() {
    using namespace alloy::hal::same70;
    using namespace alloy::generated::atsame70q21b;

    // Note: Clock must be initialized before enabling peripheral clocks
    // We'll do clock init first, then enable PIOC for LED

    // ========================================================================
    // CLOCK CONFIGURATION OPTIONS - Uncomment ONE to test
    // ========================================================================

    // Option 1: RC 12MHz (Default, lowest complexity)
    ClockConfig config = {
        .main_source = MainClockSource::InternalRC_12MHz,
        .mck_source = MasterClockSource::MainClock,
        .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
    };

    // Option 2: RC 4MHz (Low power)
    // ClockConfig config = {
    //     .main_source = MainClockSource::InternalRC_4MHz,
    //     .mck_source = MasterClockSource::MainClock,
    //     .mck_prescaler = MasterClockPrescaler::DIV_1  // 4MHz MCK
    // };

    // Option 3: Crystal 12MHz (External, more stable)
    // ClockConfig config = {
    //     .main_source = MainClockSource::ExternalCrystal,
    //     .crystal_freq_hz = 12000000,
    //     .mck_source = MasterClockSource::MainClock,
    //     .mck_prescaler = MasterClockPrescaler::DIV_1  // 12MHz MCK
    // };

    // Option 4: Crystal + PLL @ 300MHz -> MCK 150MHz (Maximum performance)
    // ClockConfig config = {
    //     .main_source = MainClockSource::ExternalCrystal,
    //     .crystal_freq_hz = 12000000,
    //     .plla = {24, 1},  // 12MHz * 25 / 1 = 300MHz
    //     .mck_source = MasterClockSource::PLLAClock,
    //     .mck_prescaler = MasterClockPrescaler::DIV_2  // 300/2 = 150MHz MCK
    // };

    // Option 5: Crystal + PLL @ 240MHz -> MCK 120MHz (Moderate performance)
    // ClockConfig config = {
    //     .main_source = MainClockSource::ExternalCrystal,
    //     .crystal_freq_hz = 12000000,
    //     .plla = {19, 1},  // 12MHz * 20 / 1 = 240MHz
    //     .mck_source = MasterClockSource::PLLAClock,
    //     .mck_prescaler = MasterClockPrescaler::DIV_2  // 240/2 = 120MHz MCK
    // };

    // ========================================================================
    // Initialize Clock
    // ========================================================================

    auto result = Clock::initialize(config);

    if (!result.is_ok()) {
        // Clock init failed - can't proceed
        // Hang here - would need debugger to see what happened
        while (1) {}
    }

    // ========================================================================
    // Enable Peripheral Clocks & Setup LED
    // ========================================================================

    // Enable PIOC peripheral clock using generated ID
    auto enable_result = Clock::enablePeripheralClock(id::PIOC);

    if (!enable_result.is_ok()) {
        // Failed to enable peripheral clock
        while (1) {}
    }

    // Setup LED (PC8 on SAME70 Xplained)
    using Led0 = GpioPin<PIOC_BASE, 8>;
    Led0 led;
    led.setDirection(PinDirection::Output);

    // ========================================================================
    // Visual Feedback
    // ========================================================================

    // Success indication - blink 5 times rapidly
    for (int i = 0; i < 5; i++) {
        led.clear();
        for (volatile unsigned int j = 0; j < 100000; j++);
        led.set();
        for (volatile unsigned int j = 0; j < 100000; j++);
    }

    // Normal blink loop - frequency depends on clock config
    // With RC 12MHz: ~1 Hz
    // With 150MHz: Same 1 Hz (delay loop is independent of clock)
    while (1) {
        led.clear();
        for (volatile unsigned int i = 0; i < 1000000; i++);
        led.set();
        for (volatile unsigned int i = 0; i < 1000000; i++);
    }

    return 0;
}
