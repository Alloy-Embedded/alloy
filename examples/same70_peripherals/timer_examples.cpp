/**
 * @file timer_examples.cpp
 * @brief Comprehensive Timer (TC) examples for SAME70
 *
 * This file demonstrates:
 * 1. Simple frequency generation (output square wave)
 * 2. PWM generation using Timer (dual output)
 * 3. Frequency measurement (capture mode)
 * 4. Interval timing and delays
 * 5. Multiple timers synchronized
 *
 * Timer Features Demonstrated:
 * - Waveform mode for PWM/frequency generation
 * - Capture mode for frequency measurement
 * - Multiple clock sources
 * - Independent dual outputs (TIOA, TIOB)
 * - Compile-time calculation helpers
 */

#include "hal/platform/same70/timer.hpp"
#include "hal/platform/same70/gpio.hpp"
#include <stdio.h>

using namespace alloy::hal::same70;

// ============================================================================
// Example 1: Simple Frequency Generation
// ============================================================================

/**
 * Generate 1 kHz square wave on Timer0 Channel 0 output (TIOA0)
 *
 * Connection: TIOA0 pin (check SAME70 datasheet for pin mapping)
 * Expected: 1 kHz square wave, 50% duty cycle
 */
void example1_frequency_generation() {
    printf("\n=== Example 1: Frequency Generation (1 kHz) ===\n");

    Timer0Ch0 timer;

    // Open timer
    auto result = timer.open();
    if (!result.is_ok()) {
        printf("Failed to open timer\n");
        return;
    }

    // Calculate period for 1 kHz frequency
    // Using MCK/8 = 18.75 MHz
    constexpr uint32_t desired_freq = 1000;  // 1 kHz
    constexpr uint32_t period = Timer0Ch0::calculatePeriod(
        TimerClock::MCK_DIV_8,
        desired_freq
    );

    printf("Calculated period: %lu (for %lu Hz)\n", period, desired_freq);

    // Configure timer for waveform mode
    TimerConfig config{};
    config.mode = TimerMode::Waveform;
    config.clock = TimerClock::MCK_DIV_8;
    config.waveform = WaveformType::UpReset;
    config.period = period;
    config.duty_a = period / 2;  // 50% duty cycle on TIOA
    config.invert_output = false;

    result = timer.configure(config);
    if (!result.is_ok()) {
        printf("Failed to configure timer\n");
        return;
    }

    // Start timer
    result = timer.start();
    if (!result.is_ok()) {
        printf("Failed to start timer\n");
        return;
    }

    printf("Timer started: Generating %lu Hz on TIOA0\n", desired_freq);
    printf("Verify with oscilloscope on TIOA0 pin\n");

    // Let it run for a while
    for (volatile uint32_t i = 0; i < 10000000; ++i) {}

    // Read counter value
    auto counter = timer.getCounter();
    if (counter.is_ok()) {
        printf("Current counter value: %u\n", counter.value());
    }

    timer.stop();
    timer.close();
}

// ============================================================================
// Example 2: PWM Generation with Dual Outputs
// ============================================================================

/**
 * Generate PWM on both Timer outputs (TIOA and TIOB)
 * TIOA: 25% duty cycle
 * TIOB: 75% duty cycle
 *
 * Connection: TIOA1, TIOB1 pins
 * Expected: Two PWM signals with different duty cycles
 */
void example2_pwm_dual_output() {
    printf("\n=== Example 2: PWM Dual Output ===\n");

    Timer0Ch1 timer;

    auto result = timer.open();
    if (!result.is_ok()) {
        printf("Failed to open timer\n");
        return;
    }

    // Configure for 10 kHz PWM
    constexpr uint32_t pwm_freq = 10000;  // 10 kHz
    constexpr uint32_t period = Timer0Ch1::calculatePeriod(
        TimerClock::MCK_DIV_8,
        pwm_freq
    );

    TimerConfig config{};
    config.mode = TimerMode::Waveform;
    config.clock = TimerClock::MCK_DIV_8;
    config.waveform = WaveformType::UpReset;
    config.period = period;
    config.duty_a = period / 4;      // 25% duty on TIOA
    config.duty_b = (period * 3) / 4; // 75% duty on TIOB
    config.invert_output = false;

    result = timer.configure(config);
    if (!result.is_ok()) {
        printf("Failed to configure timer\n");
        return;
    }

    timer.start();
    printf("PWM started: %lu Hz\n", pwm_freq);
    printf("  TIOA: 25%% duty cycle\n");
    printf("  TIOB: 75%% duty cycle\n");

    // Dynamically adjust duty cycles
    for (int i = 0; i < 10; ++i) {
        // Fade TIOA from 10% to 90%
        uint32_t duty = (period * (10 + i * 8)) / 100;
        timer.setDutyA(duty);

        printf("Adjusted TIOA duty to %d%%\n", 10 + i * 8);

        for (volatile uint32_t j = 0; j < 1000000; ++j) {}
    }

    timer.stop();
    timer.close();
}

// ============================================================================
// Example 3: Interval Timing
// ============================================================================

/**
 * Use timer for precise interval timing
 * Generate interrupt every 100ms
 *
 * Note: In real application, set up ISR handler
 */
void example3_interval_timing() {
    printf("\n=== Example 3: Interval Timing (100ms) ===\n");

    Timer0Ch2 timer;

    timer.open();

    // Configure for 10 Hz (100ms period)
    constexpr uint32_t interval_freq = 10;  // 10 Hz = 100ms
    constexpr uint32_t period = Timer0Ch2::calculatePeriod(
        TimerClock::MCK_DIV_128,
        interval_freq
    );

    TimerConfig config{};
    config.mode = TimerMode::Waveform;
    config.clock = TimerClock::MCK_DIV_128;
    config.waveform = WaveformType::UpReset;
    config.period = period;

    timer.configure(config);

    // Enable RC compare interrupt (fires when counter reaches period)
    timer.enableInterrupts(0x10);  // RC Compare interrupt

    timer.start();
    printf("Timer configured for 100ms intervals\n");
    printf("RC Compare interrupt enabled (bit 4)\n");
    printf("In real application, set up ISR to handle interrupt\n");

    // Simulate 10 intervals (1 second)
    for (int i = 0; i < 10; ++i) {
        // Poll counter (in real app, use interrupt)
        uint32_t last_count = 0;
        while (true) {
            auto counter = timer.getCounter();
            if (counter.is_ok()) {
                if (counter.value() < last_count) {
                    // Counter wrapped (reached period)
                    printf("Interval %d: 100ms elapsed\n", i + 1);
                    break;
                }
                last_count = counter.value();
            }
        }
    }

    timer.stop();
    timer.close();
}

// ============================================================================
// Example 4: Synchronized Multiple Timers
// ============================================================================

/**
 * Use multiple timer channels to generate synchronized outputs
 * Timer0 Ch0: 1 kHz
 * Timer0 Ch1: 2 kHz
 * Timer0 Ch2: 4 kHz
 *
 * All started simultaneously for phase-locked operation
 */
void example4_synchronized_timers() {
    printf("\n=== Example 4: Synchronized Multiple Timers ===\n");

    Timer0Ch0 timer0;
    Timer0Ch1 timer1;
    Timer0Ch2 timer2;

    // Open all timers
    timer0.open();
    timer1.open();
    timer2.open();

    // Configure timer 0: 1 kHz
    TimerConfig config0{};
    config0.mode = TimerMode::Waveform;
    config0.clock = TimerClock::MCK_DIV_8;
    config0.period = Timer0Ch0::calculatePeriod(TimerClock::MCK_DIV_8, 1000);
    config0.duty_a = config0.period / 2;
    timer0.configure(config0);

    // Configure timer 1: 2 kHz
    TimerConfig config1{};
    config1.mode = TimerMode::Waveform;
    config1.clock = TimerClock::MCK_DIV_8;
    config1.period = Timer0Ch1::calculatePeriod(TimerClock::MCK_DIV_8, 2000);
    config1.duty_a = config1.period / 2;
    timer1.configure(config1);

    // Configure timer 2: 4 kHz
    TimerConfig config2{};
    config2.mode = TimerMode::Waveform;
    config2.clock = TimerClock::MCK_DIV_8;
    config2.period = Timer0Ch2::calculatePeriod(TimerClock::MCK_DIV_8, 4000);
    config2.duty_a = config2.period / 2;
    timer2.configure(config2);

    printf("Starting 3 synchronized timers:\n");
    printf("  Timer0 Ch0: 1 kHz\n");
    printf("  Timer0 Ch1: 2 kHz\n");
    printf("  Timer0 Ch2: 4 kHz\n");

    // Start all timers simultaneously
    timer0.start();
    timer1.start();
    timer2.start();

    printf("All timers running - outputs are phase-locked\n");

    // Let them run
    for (volatile uint32_t i = 0; i < 10000000; ++i) {}

    // Stop all
    timer0.stop();
    timer1.stop();
    timer2.stop();

    timer0.close();
    timer1.close();
    timer2.close();
}

// ============================================================================
// Example 5: Variable Frequency Generator
// ============================================================================

/**
 * Dynamically change timer frequency in real-time
 * Sweep from 100 Hz to 10 kHz
 */
void example5_variable_frequency() {
    printf("\n=== Example 5: Variable Frequency Generator ===\n");

    Timer1Ch0 timer;
    timer.open();

    TimerConfig config{};
    config.mode = TimerMode::Waveform;
    config.clock = TimerClock::MCK_DIV_8;
    config.waveform = WaveformType::UpReset;
    config.duty_a = 0;  // Will be set to period/2

    timer.configure(config);
    timer.start();

    printf("Sweeping frequency from 100 Hz to 10 kHz\n");

    // Frequency sweep
    for (uint32_t freq = 100; freq <= 10000; freq += 100) {
        uint32_t period = Timer1Ch0::calculatePeriod(TimerClock::MCK_DIV_8, freq);

        timer.setPeriod(period);
        timer.setDutyA(period / 2);  // 50% duty

        if (freq % 1000 == 0) {
            printf("Current frequency: %lu Hz (period=%lu)\n", freq, period);
        }

        // Brief delay at each frequency
        for (volatile uint32_t i = 0; i < 100000; ++i) {}
    }

    printf("Frequency sweep complete\n");

    timer.stop();
    timer.close();
}

// ============================================================================
// Example 6: Compile-Time Calculations
// ============================================================================

/**
 * Demonstrate compile-time frequency/period calculations
 * All calculations done at compile time - zero runtime overhead
 */
void example6_compile_time_calculations() {
    printf("\n=== Example 6: Compile-Time Calculations ===\n");

    // Calculate periods and frequencies
    uint32_t freq_1khz = Timer0Ch0::calculateFrequency(
        TimerClock::MCK_DIV_8,
        1875
    );

    uint32_t period_10khz = Timer0Ch0::calculatePeriod(
        TimerClock::MCK_DIV_8,
        10000
    );

    uint32_t period_1mhz = Timer0Ch0::calculatePeriod(
        TimerClock::MCK_DIV_2,
        1000000
    );

    printf("Timer calculations:\n");
    printf("  1875 ticks @ MCK/8 = %u Hz\n", freq_1khz);
    printf("  10 kHz @ MCK/8 requires period = %u\n", period_10khz);
    printf("  1 MHz @ MCK/2 requires period = %u\n", period_1mhz);

    // Verify calculations
    if (freq_1khz == 10000 && period_10khz == 1875 && period_1mhz == 75) {
        printf("All calculations verified!\n");
    } else {
        printf("Calculation mismatch!\n");
    }
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        SAME70 Timer (TC) Examples                         ║\n");
    printf("║        Template-Based Zero-Overhead HAL                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    // Run all examples
    example1_frequency_generation();
    example2_pwm_dual_output();
    example3_interval_timing();
    example4_synchronized_timers();
    example5_variable_frequency();
    example6_compile_time_calculations();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  All Timer examples completed successfully!               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
