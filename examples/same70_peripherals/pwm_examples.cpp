/**
 * @file pwm_examples.cpp
 * @brief Comprehensive PWM examples for SAME70
 *
 * This file demonstrates:
 * 1. Simple PWM output (LED dimming)
 * 2. Motor control with dead-time
 * 3. Servo control
 * 4. Multi-channel synchronized PWM
 * 5. Center-aligned PWM
 * 6. Dynamic duty cycle control
 *
 * PWM Features Demonstrated:
 * - Left-aligned and center-aligned modes
 * - Dead-time generation
 * - Multiple synchronized channels
 * - Dynamic frequency/duty cycle updates
 * - Compile-time calculation helpers
 */

#include <stdio.h>

#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/pwm.hpp"

using namespace alloy::hal::same70;

// ============================================================================
// Example 1: LED Dimming with PWM
// ============================================================================

/**
 * Control LED brightness using PWM
 * Fade LED from 0% to 100% and back
 *
 * Connection: LED on PWM0 Ch0 output pin
 * Expected: LED gradually fades in and out
 */
void example1_led_dimming() {
    printf("\n=== Example 1: LED Dimming ===\n");

    Pwm0Ch0 pwm;

    // Open PWM
    auto result = pwm.open();
    if (!result.is_ok()) {
        printf("Failed to open PWM\n");
        return;
    }

    // Configure PWM for LED dimming
    // High frequency to avoid visible flicker (1 kHz)
    constexpr uint32_t pwm_freq = 1000;  // 1 kHz
    const uint16_t period =
        Pwm0Ch0::calculatePeriod(PwmPrescaler::DIV_128, pwm_freq, PwmAlignment::Edge);

    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Edge;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_128;
    config.period = period;
    config.duty_cycle = 0;  // Start with LED off

    result = pwm.configure(config);
    if (!result.is_ok()) {
        printf("Failed to configure PWM\n");
        return;
    }

    // Start PWM
    pwm.start();
    printf("PWM started: %u Hz, period=%u\n", pwm_freq, period);

    // Fade in
    printf("Fading LED in...\n");
    for (uint8_t brightness = 0; brightness <= 100; brightness += 5) {
        uint16_t duty = Pwm0Ch0::calculateDuty(period, brightness);
        pwm.setDutyCycle(duty);
        printf("  Brightness: %u%% (duty=%u)\n", brightness, duty);

        // Delay
        for (volatile uint32_t i = 0; i < 1000000; ++i) {}
    }

    // Fade out
    printf("Fading LED out...\n");
    for (int8_t brightness = 100; brightness >= 0; brightness -= 5) {
        uint16_t duty = Pwm0Ch0::calculateDuty(period, brightness);
        pwm.setDutyCycle(duty);
        printf("  Brightness: %d%% (duty=%u)\n", brightness, duty);

        for (volatile uint32_t i = 0; i < 1000000; ++i) {}
    }

    pwm.stop();
    pwm.close();
    printf("LED dimming complete\n");
}

// ============================================================================
// Example 2: Motor Control with Dead-Time
// ============================================================================

/**
 * Generate complementary PWM signals for H-bridge motor control
 * Includes dead-time to prevent shoot-through
 *
 * Connection: PWM0 Ch1 output to H-bridge high-side, low-side
 * Expected: Complementary PWM signals with dead-time protection
 */
void example2_motor_control() {
    printf("\n=== Example 2: Motor Control with Dead-Time ===\n");

    Pwm0Ch1 pwm;
    pwm.open();

    // Motor control PWM: 20 kHz typical for motor drives
    constexpr uint32_t motor_freq = 20000;  // 20 kHz
    const uint16_t period =
        Pwm0Ch1::calculatePeriod(PwmPrescaler::DIV_8, motor_freq,
                                 PwmAlignment::Center  // Center-aligned for symmetric switching
        );

    // Dead-time: 500ns typical for MOSFETs
    // Clock @ DIV_8 = 18.75 MHz, period = 53.33ns
    // 500ns / 53.33ns = ~9.4 ticks
    constexpr uint16_t dead_time_ticks = 10;

    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Center;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_8;
    config.period = period;
    config.duty_cycle = Pwm0Ch1::calculateDuty(period, 50);  // 50% duty = no net rotation
    config.dead_time_h = dead_time_ticks;                    // High-side dead-time
    config.dead_time_l = dead_time_ticks;                    // Low-side dead-time

    pwm.configure(config);
    pwm.start();

    printf("Motor PWM started: %u Hz\n", motor_freq);
    printf("  Mode: Center-aligned\n");
    printf("  Period: %u ticks\n", period);
    printf("  Dead-time: %u ticks (~500ns)\n", dead_time_ticks);
    printf("  Initial duty: 50%% (motor stopped)\n");

    // Ramp motor speed from 0% to 80%
    printf("\nRamping motor speed:\n");
    for (uint8_t speed = 50; speed <= 80; speed += 5) {
        uint16_t duty = Pwm0Ch1::calculateDuty(period, speed);
        pwm.setDutyCycle(duty);
        printf("  Motor speed: %u%% (duty=%u)\n", speed, duty);

        for (volatile uint32_t i = 0; i < 2000000; ++i) {}
    }

    // Hold at 80% for a while
    printf("Holding at 80%% speed\n");
    for (volatile uint32_t i = 0; i < 10000000; ++i) {}

    // Ramp down
    printf("Ramping down:\n");
    for (int8_t speed = 80; speed >= 50; speed -= 5) {
        uint16_t duty = Pwm0Ch1::calculateDuty(period, speed);
        pwm.setDutyCycle(duty);
        printf("  Motor speed: %d%%\n", speed);

        for (volatile uint32_t i = 0; i < 2000000; ++i) {}
    }

    pwm.stop();
    pwm.close();
    printf("Motor control complete\n");
}

// ============================================================================
// Example 3: Servo Control
// ============================================================================

/**
 * Generate PWM signal for standard hobby servo
 * Servo expects 50 Hz (20ms period) with 1-2ms pulse width
 *
 * Connection: Servo signal wire to PWM0 Ch2
 * Expected: Servo sweeps from 0° to 180°
 */
void example3_servo_control() {
    printf("\n=== Example 3: Servo Control ===\n");

    Pwm0Ch2 pwm;
    pwm.open();

    // Servo standard: 50 Hz (20ms period)
    constexpr uint32_t servo_freq = 50;
    const uint16_t period =
        Pwm0Ch2::calculatePeriod(PwmPrescaler::DIV_128, servo_freq, PwmAlignment::Edge);

    // Servo pulse width:
    // - 1.0ms = 0°
    // - 1.5ms = 90°
    // - 2.0ms = 180°
    //
    // With DIV_128: clock = 1.171875 MHz, tick period = 0.853us
    // 1.0ms = 1172 ticks
    // 1.5ms = 1758 ticks
    // 2.0ms = 2344 ticks

    constexpr uint16_t servo_0deg = 1172;
    constexpr uint16_t servo_90deg = 1758;
    constexpr uint16_t servo_180deg = 2344;

    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Edge;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_128;
    config.period = period;
    config.duty_cycle = servo_90deg;  // Start at 90°

    pwm.configure(config);
    pwm.start();

    printf("Servo PWM started: %u Hz, period=%u ticks\n", servo_freq, period);
    printf("  0° = %u ticks (1.0ms)\n", servo_0deg);
    printf("  90° = %u ticks (1.5ms)\n", servo_90deg);
    printf("  180° = %u ticks (2.0ms)\n", servo_180deg);

    // Sweep servo from 0° to 180°
    printf("\nSweeping servo from 0° to 180°:\n");
    for (uint16_t angle = 0; angle <= 180; angle += 10) {
        // Linear interpolation
        uint16_t duty = servo_0deg + ((servo_180deg - servo_0deg) * angle) / 180;
        pwm.setDutyCycle(duty);
        printf("  Angle: %u° (duty=%u)\n", angle, duty);

        // Servo needs time to move
        for (volatile uint32_t i = 0; i < 5000000; ++i) {}
    }

    // Sweep back
    printf("Sweeping back from 180° to 0°:\n");
    for (int16_t angle = 180; angle >= 0; angle -= 10) {
        uint16_t duty = servo_0deg + ((servo_180deg - servo_0deg) * angle) / 180;
        pwm.setDutyCycle(duty);
        printf("  Angle: %d°\n", angle);

        for (volatile uint32_t i = 0; i < 5000000; ++i) {}
    }

    pwm.stop();
    pwm.close();
    printf("Servo control complete\n");
}

// ============================================================================
// Example 4: Multi-Channel Synchronized PWM
// ============================================================================

/**
 * Generate synchronized PWM on 4 channels
 * Useful for RGB+W LED control, 3-phase motor control, etc.
 *
 * Connection: PWM0 Ch0-3 to RGB+W LED or motor phases
 * Expected: 4 synchronized PWM channels with different duty cycles
 */
void example4_multi_channel_pwm() {
    printf("\n=== Example 4: Multi-Channel Synchronized PWM ===\n");

    Pwm0Ch0 pwm0;
    Pwm0Ch1 pwm1;
    Pwm0Ch2 pwm2;
    Pwm0Ch3 pwm3;

    // Open all channels
    pwm0.open();
    pwm1.open();
    pwm2.open();
    pwm3.open();

    // Configure all for same frequency (1 kHz)
    constexpr uint32_t pwm_freq = 1000;
    const uint16_t period =
        Pwm0Ch0::calculatePeriod(PwmPrescaler::DIV_128, pwm_freq, PwmAlignment::Edge);

    // Configure each channel with different duty cycle
    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Edge;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_128;
    config.period = period;

    // Channel 0: 25% (Red)
    config.duty_cycle = Pwm0Ch0::calculateDuty(period, 25);
    pwm0.configure(config);

    // Channel 1: 50% (Green)
    config.duty_cycle = Pwm0Ch1::calculateDuty(period, 50);
    pwm1.configure(config);

    // Channel 2: 75% (Blue)
    config.duty_cycle = Pwm0Ch2::calculateDuty(period, 75);
    pwm2.configure(config);

    // Channel 3: 100% (White)
    config.duty_cycle = period;
    pwm3.configure(config);

    printf("Starting 4 synchronized PWM channels:\n");
    printf("  Ch0: 25%% duty\n");
    printf("  Ch1: 50%% duty\n");
    printf("  Ch2: 75%% duty\n");
    printf("  Ch3: 100%% duty\n");

    // Start all channels (synchronized start)
    pwm0.start();
    pwm1.start();
    pwm2.start();
    pwm3.start();

    // Verify all are running
    printf("Status: Ch0=%d Ch1=%d Ch2=%d Ch3=%d\n", pwm0.isRunning(), pwm1.isRunning(),
           pwm2.isRunning(), pwm3.isRunning());

    // Animate: cycle through different color combinations
    printf("\nAnimating RGB+W LED:\n");
    for (int i = 0; i < 10; ++i) {
        // Create color patterns
        uint8_t r = (i * 10) % 100;
        uint8_t g = ((i * 20) % 100);
        uint8_t b = ((i * 30) % 100);
        uint8_t w = 100 - ((i * 10) % 100);

        pwm0.setDutyCycle(Pwm0Ch0::calculateDuty(period, r));
        pwm1.setDutyCycle(Pwm0Ch1::calculateDuty(period, g));
        pwm2.setDutyCycle(Pwm0Ch2::calculateDuty(period, b));
        pwm3.setDutyCycle(Pwm0Ch3::calculateDuty(period, w));

        printf("  R=%u%% G=%u%% B=%u%% W=%u%%\n", r, g, b, w);

        for (volatile uint32_t j = 0; j < 3000000; ++j) {}
    }

    // Stop all
    pwm0.stop();
    pwm1.stop();
    pwm2.stop();
    pwm3.stop();

    pwm0.close();
    pwm1.close();
    pwm2.close();
    pwm3.close();

    printf("Multi-channel PWM complete\n");
}

// ============================================================================
// Example 5: Center-Aligned PWM
// ============================================================================

/**
 * Demonstrate center-aligned PWM mode
 * Center-aligned provides symmetric switching, useful for motor control
 *
 * Connection: PWM1 Ch0 output
 * Expected: Center-aligned PWM waveform
 */
void example5_center_aligned_pwm() {
    printf("\n=== Example 5: Center-Aligned PWM ===\n");

    Pwm1Ch0 pwm;
    pwm.open();

    // Configure center-aligned PWM
    constexpr uint32_t pwm_freq = 10000;  // 10 kHz
    const uint16_t period =
        Pwm1Ch0::calculatePeriod(PwmPrescaler::DIV_16, pwm_freq, PwmAlignment::Center);

    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Center;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_16;
    config.period = period;
    config.duty_cycle = Pwm1Ch0::calculateDuty(period, 50);

    pwm.configure(config);
    pwm.start();

    printf("Center-aligned PWM started: %u Hz\n", pwm_freq);
    printf("  Period: %u ticks\n", period);
    printf("  Note: Counter counts UP to period, then DOWN to 0\n");
    printf("  Provides symmetric edge placement\n");

    // Sweep duty cycle
    printf("\nDuty cycle sweep:\n");
    for (uint8_t duty = 10; duty <= 90; duty += 10) {
        uint16_t duty_val = Pwm1Ch0::calculateDuty(period, duty);
        pwm.setDutyCycle(duty_val);
        printf("  Duty: %u%% (value=%u)\n", duty, duty_val);

        for (volatile uint32_t i = 0; i < 2000000; ++i) {}
    }

    pwm.stop();
    pwm.close();
    printf("Center-aligned PWM complete\n");
}

// ============================================================================
// Example 6: Dynamic Frequency Change
// ============================================================================

/**
 * Dynamically change PWM frequency during operation
 * Demonstrates real-time frequency adjustment
 */
void example6_dynamic_frequency() {
    printf("\n=== Example 6: Dynamic Frequency Change ===\n");

    Pwm1Ch1 pwm;
    pwm.open();

    Same70PwmConfig config{};
    config.alignment = PwmAlignment::Edge;
    config.polarity = PwmPolarity::Normal;
    config.prescaler = PwmPrescaler::DIV_8;
    config.period = 1000;
    config.duty_cycle = 500;  // 50% duty

    pwm.configure(config);
    pwm.start();

    printf("Starting with initial frequency\n");
    printf("Sweeping frequency by changing period:\n");

    // Sweep frequency by changing period
    for (uint16_t period = 100; period <= 10000; period += 500) {
        pwm.setPeriod(period);
        pwm.setDutyCycle(period / 2);  // Maintain 50% duty

        uint32_t freq =
            Pwm1Ch1::calculateFrequency(PwmPrescaler::DIV_8, period, PwmAlignment::Edge);

        printf("  Period=%u -> Frequency=%lu Hz\n", period, freq);

        for (volatile uint32_t i = 0; i < 1000000; ++i) {}
    }

    pwm.stop();
    pwm.close();
    printf("Dynamic frequency sweep complete\n");
}

// ============================================================================
// Example 7: Compile-Time Calculations
// ============================================================================

/**
 * Demonstrate compile-time PWM calculations
 * All math done at compile time - zero runtime overhead
 */
void example7_compile_time_calculations() {
    printf("\n=== Example 7: Compile-Time Calculations ===\n");

    // All calculations happen at COMPILE TIME
    const uint16_t period_1khz =
        Pwm0Ch0::calculatePeriod(PwmPrescaler::DIV_128, 1000, PwmAlignment::Edge);

    const uint16_t period_50hz =
        Pwm0Ch0::calculatePeriod(PwmPrescaler::DIV_128, 50, PwmAlignment::Edge);

    const uint32_t freq_from_period =
        Pwm0Ch0::calculateFrequency(PwmPrescaler::DIV_128, period_1khz, PwmAlignment::Edge);

    constexpr uint16_t duty_25 = Pwm0Ch0::calculateDuty(1000, 25);
    constexpr uint16_t duty_50 = Pwm0Ch0::calculateDuty(1000, 50);
    constexpr uint16_t duty_75 = Pwm0Ch0::calculateDuty(1000, 75);

    printf("Compile-time calculations (zero runtime cost):\n");
    printf("  1 kHz @ DIV_128 requires period = %u\n", period_1khz);
    printf("  50 Hz @ DIV_128 requires period = %u\n", period_50hz);
    printf("  Frequency from period %u = %lu Hz\n", period_1khz, freq_from_period);
    printf("  25%% of 1000 = %u\n", duty_25);
    printf("  50%% of 1000 = %u\n", duty_50);
    printf("  75%% of 1000 = %u\n", duty_75);

    // Verify calculations
    if (duty_25 == 250 && duty_50 == 500 && duty_75 == 750) {
        printf("All duty cycle calculations verified!\n");
    } else {
        printf("Duty cycle calculation mismatch!\n");
    }
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        SAME70 PWM Examples                                 ║\n");
    printf("║        Template-Based Zero-Overhead HAL                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    // Run all examples
    example1_led_dimming();
    example2_motor_control();
    example3_servo_control();
    example4_multi_channel_pwm();
    example5_center_aligned_pwm();
    example6_dynamic_frequency();
    example7_compile_time_calculations();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  All PWM examples completed successfully!                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
