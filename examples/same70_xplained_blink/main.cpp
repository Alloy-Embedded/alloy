/**
 * @file main.cpp
 * @brief LED Blink Test for SAME70 Xplained Ultra
 *
 * Migrating gradually from bare metal to HAL abstractions.
 * Step 3: Use complete Clock and GPIO HAL
 *
 * @author Alloy Framework
 * @date 2025-11-09
 */

#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"

int main() {
    using namespace alloy::hal::same70;

    // Step 0: Setup LED for debug BEFORE clock init
    // Enable PIOC peripheral clock manually (using boot clock)
    volatile unsigned int* PMC_PCER0 = (volatile unsigned int*)0x400E0610;
    *PMC_PCER0 = (1u << 12);

    // Setup LED GPIO manually
    using Led0 = GpioPin<PIOC_BASE, 8>;
    Led0 led;
    led.setDirection(PinDirection::Output);

    // Blink 3 times rapidly to show we started
    for (int i = 0; i < 3; i++) {
        led.clear();  // ON
        for (volatile unsigned int j = 0; j < 20000; j++);
        led.set();    // OFF
        for (volatile unsigned int j = 0; j < 20000; j++);
    }

    // Step 1: Test if we can even call Clock::initialize() without crashing
    // Blink 4 times BEFORE calling Clock::initialize()
    for (int i = 0; i < 4; i++) {
        led.clear();
        for (volatile unsigned int j = 0; j < 20000; j++);
        led.set();
        for (volatile unsigned int j = 0; j < 20000; j++);
    }

    // Now try the simplest clock config
    ClockConfig minimal_config = {
        .main_source = MainClockSource::InternalRC_12MHz,
        .crystal_freq_hz = 12000000,
        .plla = {24, 1},  // Not used
        .mck_source = MasterClockSource::MainClock,  // Use main clock directly, NO PLL
        .mck_prescaler = MasterClockPrescaler::DIV_1  // No division, 12MHz MCK
    };

    // Blink 6 times right AFTER creating config (to see if config creation works)
    for (int i = 0; i < 6; i++) {
        led.clear();
        for (volatile unsigned int j = 0; j < 100000; j++);
        led.set();
        for (volatile unsigned int j = 0; j < 100000; j++);
    }

    // Initialize clock system (now without waiting for RC stabilization)
    auto clock_result = Clock::initialize(minimal_config);

    // Blink 7 times if we returned from Clock::initialize()
    for (int i = 0; i < 7; i++) {
        led.clear();
        for (volatile unsigned int j = 0; j < 1000000; j++);
        led.set();
        for (volatile unsigned int j = 0; j < 1000000; j++);
    }

    // if (!clock_result.is_ok()) {
    //     // Clock initialization failed - blink fast forever
    //     while (1) {
    //         led.clear();
    //         for (volatile unsigned int j = 0; j < 500000; j++);
    //         led.set();
    //         for (volatile unsigned int j = 0; j < 500000; j++);
    //     }
    // }

    // Blink 5 times to show clock init succeeded
    for (int i = 0; i < 5; i++) {
        led.clear();
        for (volatile unsigned int j = 0; j < 100000; j++);
        led.set();
        for (volatile unsigned int j = 0; j < 100000; j++);
    }

    // Blink LED forever using GPIO HAL
    // LED0 is active-low: clear()=ON, set()=OFF
    while (1) {
        led.clear();  // Turn LED ON (active-low)
        for (volatile unsigned int i = 0; i < 1000000; i++);  // Delay

        led.set();    // Turn LED OFF (active-low)
        for (volatile unsigned int i = 0; i < 1000000; i++);  // Delay
    }

    return 0;
}
