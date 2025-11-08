/**
 * @file clock_examples.cpp
 * @brief Comprehensive Clock (PMC) examples for SAME70
 *
 * This file demonstrates:
 * 1. Basic clock initialization (150 MHz MCK)
 * 2. Clock frequency reporting
 * 3. Peripheral clock management
 * 4. Different clock configurations
 * 5. Runtime clock switching
 *
 * Clock Features Demonstrated:
 * - Main oscillator configuration (crystal and RC)
 * - PLLA configuration and multiplication
 * - Master clock (MCK) source and prescaler
 * - Peripheral clock enable/disable
 * - Clock frequency calculations
 */

#include <stdio.h>

#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/uart.hpp"

using namespace alloy::hal::same70;

// ============================================================================
// Example 1: Basic Clock Initialization
// ============================================================================

/**
 * Initialize system clocks to maximum performance (150 MHz)
 *
 * Clock Tree:
 * - Crystal: 12 MHz
 * - PLLA: 12 MHz * 25 = 300 MHz
 * - MCK: 300 MHz / 2 = 150 MHz
 */
void example1_basic_initialization() {
    printf("\n=== Example 1: Basic Clock Initialization (150 MHz) ===\n");

    // Use predefined 150 MHz configuration
    auto result = Clock::initialize(CLOCK_CONFIG_150MHZ);

    if (!result.is_ok()) {
        printf("Failed to initialize clocks\n");
        return;
    }

    printf("Clock initialization successful!\n");
    printf("Master Clock (MCK): %u Hz\n", Clock::getMasterClockFrequency());
    printf("Main Clock: %u Hz\n", Clock::getMainClockFrequency());
    printf("PLLA Clock: %u Hz\n", Clock::getPllaFrequency());
}

// ============================================================================
// Example 2: Clock Frequency Reporting
// ============================================================================

/**
 * Report all clock frequencies and configuration
 */
void example2_frequency_reporting() {
    printf("\n=== Example 2: Clock Frequency Reporting ===\n");

    if (!Clock::isInitialized()) {
        printf("Clocks not initialized, using default config\n");
        Clock::initialize(CLOCK_CONFIG_150MHZ);
    }

    const auto& config = Clock::getConfig();

    printf("\n--- Clock Configuration ---\n");

    // Main clock source
    printf("Main Clock Source: ");
    switch (config.main_source) {
        case MainClockSource::InternalRC_4MHz:
            printf("Internal RC 4 MHz\n");
            break;
        case MainClockSource::InternalRC_8MHz:
            printf("Internal RC 8 MHz\n");
            break;
        case MainClockSource::InternalRC_12MHz:
            printf("Internal RC 12 MHz\n");
            break;
        case MainClockSource::ExternalCrystal:
            printf("External Crystal (%u Hz)\n", config.crystal_freq_hz);
            break;
    }

    // PLLA configuration
    printf("PLLA Multiplier: %u (x%u)\n", config.plla.multiplier, config.plla.multiplier + 1);
    printf("PLLA Divider: %u\n", config.plla.divider);

    // Master clock
    printf("MCK Source: ");
    switch (config.mck_source) {
        case MasterClockSource::SlowClock:
            printf("Slow Clock (32 kHz)\n");
            break;
        case MasterClockSource::MainClock:
            printf("Main Clock\n");
            break;
        case MasterClockSource::PLLAClock:
            printf("PLLA Clock\n");
            break;
        case MasterClockSource::UPLLClock:
            printf("UPLL Clock\n");
            break;
    }

    printf("MCK Prescaler: ");
    switch (config.mck_prescaler) {
        case MasterClockPrescaler::DIV_1:
            printf("/1\n");
            break;
        case MasterClockPrescaler::DIV_2:
            printf("/2\n");
            break;
        case MasterClockPrescaler::DIV_3:
            printf("/3\n");
            break;
        case MasterClockPrescaler::DIV_4:
            printf("/4\n");
            break;
    }

    printf("\n--- Calculated Frequencies ---\n");
    printf("Main Clock:   %10u Hz (%6.2f MHz)\n", Clock::getMainClockFrequency(),
           Clock::getMainClockFrequency() / 1000000.0f);

    printf("PLLA Clock:   %10u Hz (%6.2f MHz)\n", Clock::getPllaFrequency(),
           Clock::getPllaFrequency() / 1000000.0f);

    printf("Master Clock: %10u Hz (%6.2f MHz)\n", Clock::getMasterClockFrequency(),
           Clock::getMasterClockFrequency() / 1000000.0f);

    printf("\n--- Derived Peripheral Clocks ---\n");
    uint32_t mck = Clock::getMasterClockFrequency();
    printf("MCK/2:   %10u Hz (%6.2f MHz) - Used by TC, PWM\n", mck / 2, (mck / 2) / 1000000.0f);
    printf("MCK/4:   %10u Hz (%6.2f MHz)\n", mck / 4, (mck / 4) / 1000000.0f);
    printf("MCK/8:   %10u Hz (%6.2f MHz) - Used by UART baud rate\n", mck / 8,
           (mck / 8) / 1000000.0f);
}

// ============================================================================
// Example 3: Peripheral Clock Management
// ============================================================================

/**
 * Demonstrate peripheral clock enable/disable
 *
 * Peripheral IDs (examples):
 * - UART0: 7
 * - UART1: 8
 * - PIOA: 10
 * - PIOB: 11
 * - SPI0: 21
 * - TC0: 23
 * - PWM0: 31
 * - AFEC0: 29
 */
void example3_peripheral_clock_management() {
    printf("\n=== Example 3: Peripheral Clock Management ===\n");

    if (!Clock::isInitialized()) {
        Clock::initialize(CLOCK_CONFIG_150MHZ);
    }

    // Example peripheral IDs
    constexpr uint8_t UART0_ID = 7;
    constexpr uint8_t PIOA_ID = 10;
    constexpr uint8_t SPI0_ID = 21;
    constexpr uint8_t TC0_ID = 23;
    constexpr uint8_t PWM0_ID = 31;

    printf("\n--- Initial Peripheral Clock Status ---\n");
    printf("UART0 (ID %u): %s\n", UART0_ID,
           Clock::isPeripheralClockEnabled(UART0_ID) ? "ENABLED" : "DISABLED");
    printf("PIOA  (ID %u): %s\n", PIOA_ID,
           Clock::isPeripheralClockEnabled(PIOA_ID) ? "ENABLED" : "DISABLED");
    printf("SPI0  (ID %u): %s\n", SPI0_ID,
           Clock::isPeripheralClockEnabled(SPI0_ID) ? "ENABLED" : "DISABLED");

    printf("\n--- Enabling Peripheral Clocks ---\n");

    // Enable UART0 clock
    auto result = Clock::enablePeripheralClock(UART0_ID);
    if (result.is_ok()) {
        printf("✓ UART0 clock enabled\n");
    }

    // Enable GPIO port A clock
    result = Clock::enablePeripheralClock(PIOA_ID);
    if (result.is_ok()) {
        printf("✓ PIOA clock enabled\n");
    }

    // Enable SPI0 clock
    result = Clock::enablePeripheralClock(SPI0_ID);
    if (result.is_ok()) {
        printf("✓ SPI0 clock enabled\n");
    }

    // Enable TC0 clock
    result = Clock::enablePeripheralClock(TC0_ID);
    if (result.is_ok()) {
        printf("✓ TC0 clock enabled\n");
    }

    // Enable PWM0 clock
    result = Clock::enablePeripheralClock(PWM0_ID);
    if (result.is_ok()) {
        printf("✓ PWM0 clock enabled\n");
    }

    printf("\n--- Updated Peripheral Clock Status ---\n");
    printf("UART0: %s\n", Clock::isPeripheralClockEnabled(UART0_ID) ? "ENABLED" : "DISABLED");
    printf("PIOA:  %s\n", Clock::isPeripheralClockEnabled(PIOA_ID) ? "ENABLED" : "DISABLED");
    printf("SPI0:  %s\n", Clock::isPeripheralClockEnabled(SPI0_ID) ? "ENABLED" : "DISABLED");
    printf("TC0:   %s\n", Clock::isPeripheralClockEnabled(TC0_ID) ? "ENABLED" : "DISABLED");
    printf("PWM0:  %s\n", Clock::isPeripheralClockEnabled(PWM0_ID) ? "ENABLED" : "DISABLED");

    // Demonstrate disable
    printf("\n--- Disabling SPI0 Clock ---\n");
    result = Clock::disablePeripheralClock(SPI0_ID);
    if (result.is_ok()) {
        printf("✓ SPI0 clock disabled\n");
        printf("SPI0:  %s\n", Clock::isPeripheralClockEnabled(SPI0_ID) ? "ENABLED" : "DISABLED");
    }
}

// ============================================================================
// Example 4: Conservative Clock Configuration (120 MHz)
// ============================================================================

/**
 * Use a more conservative clock configuration for better power efficiency
 *
 * Clock Tree:
 * - Crystal: 12 MHz
 * - PLLA: 12 MHz * 20 = 240 MHz
 * - MCK: 240 MHz / 2 = 120 MHz
 */
void example4_conservative_configuration() {
    printf("\n=== Example 4: Conservative Configuration (120 MHz) ===\n");

    auto result = Clock::initialize(CLOCK_CONFIG_120MHZ);

    if (!result.is_ok()) {
        printf("Failed to initialize clocks\n");
        return;
    }

    printf("Clock initialization successful!\n");
    printf("Master Clock (MCK): %u Hz (%.1f MHz)\n", Clock::getMasterClockFrequency(),
           Clock::getMasterClockFrequency() / 1000000.0f);

    printf("\nBenefits of 120 MHz configuration:\n");
    printf("  - Lower power consumption\n");
    printf("  - Better EMI characteristics\n");
    printf("  - More timing margin\n");
    printf("  - Still excellent performance for most applications\n");
}

// ============================================================================
// Example 5: Low Power Configuration (12 MHz RC)
// ============================================================================

/**
 * Ultra-low power configuration using internal RC oscillator
 *
 * Clock Tree:
 * - Internal RC: 12 MHz
 * - MCK: 12 MHz (no PLL)
 */
void example5_low_power_configuration() {
    printf("\n=== Example 5: Low Power Configuration (12 MHz RC) ===\n");

    auto result = Clock::initialize(CLOCK_CONFIG_12MHZ_RC);

    if (!result.is_ok()) {
        printf("Failed to initialize clocks\n");
        return;
    }

    printf("Clock initialization successful!\n");
    printf("Master Clock (MCK): %u Hz (%.1f MHz)\n", Clock::getMasterClockFrequency(),
           Clock::getMasterClockFrequency() / 1000000.0f);

    printf("\nBenefits of RC oscillator configuration:\n");
    printf("  - No external crystal required\n");
    printf("  - Lowest power consumption\n");
    printf("  - Instant startup (no crystal stabilization)\n");
    printf("  - Suitable for low-speed applications\n");
    printf("\nLimitations:\n");
    printf("  - Lower accuracy (±1%% vs crystal ±50ppm)\n");
    printf("  - Temperature dependent\n");
    printf("  - Not suitable for USB or high-speed communications\n");
}

// ============================================================================
// Example 6: Custom Clock Configuration
// ============================================================================

/**
 * Create a custom clock configuration
 *
 * Example: 100 MHz MCK from 12 MHz crystal
 * PLLA: 12 MHz * 17 / 1 = 204 MHz
 * MCK: 204 MHz / 2 = 102 MHz (close to 100 MHz)
 */
void example6_custom_configuration() {
    printf("\n=== Example 6: Custom Configuration (100 MHz) ===\n");

    ClockConfig custom_config = {
        .main_source = MainClockSource::ExternalCrystal,
        .crystal_freq_hz = 12000000,
        .plla = {16, 1},  // 12 * 17 / 1 = 204 MHz
        .mck_source = MasterClockSource::PLLAClock,
        .mck_prescaler = MasterClockPrescaler::DIV_2  // 204 / 2 = 102 MHz
    };

    auto result = Clock::initialize(custom_config);

    if (!result.is_ok()) {
        printf("Failed to initialize clocks\n");
        return;
    }

    printf("Custom clock configuration successful!\n");
    printf("PLLA Clock: %u Hz (%.1f MHz)\n", Clock::getPllaFrequency(),
           Clock::getPllaFrequency() / 1000000.0f);
    printf("Master Clock (MCK): %u Hz (%.1f MHz)\n", Clock::getMasterClockFrequency(),
           Clock::getMasterClockFrequency() / 1000000.0f);
}

// ============================================================================
// Example 7: Clock-Dependent Peripheral Configuration
// ============================================================================

/**
 * Show how to configure peripherals based on current clock frequency
 * Example: UART baud rate calculation depends on MCK
 */
void example7_clock_dependent_peripherals() {
    printf("\n=== Example 7: Clock-Dependent Peripheral Configuration ===\n");

    if (!Clock::isInitialized()) {
        Clock::initialize(CLOCK_CONFIG_150MHZ);
    }

    uint32_t mck = Clock::getMasterClockFrequency();
    printf("Current MCK: %u Hz\n\n", mck);

    // UART baud rate calculation: CD = MCK / (16 * baud_rate)
    printf("--- UART Baud Rate Calculations ---\n");

    uint32_t baud_rates[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

    for (uint32_t baud : baud_rates) {
        uint32_t cd = mck / (16 * baud);
        uint32_t actual_baud = mck / (16 * cd);
        float error = ((float)actual_baud - baud) / baud * 100.0f;

        printf("  %7u baud: CD=%5u, actual=%7u, error=%+.2f%%\n", baud, cd, actual_baud, error);
    }

    // Timer frequency calculation
    printf("\n--- Timer Clock Frequencies ---\n");
    printf("  MCK/2:   %10u Hz (%.2f MHz)\n", mck / 2, (mck / 2) / 1000000.0f);
    printf("  MCK/8:   %10u Hz (%.2f MHz)\n", mck / 8, (mck / 8) / 1000000.0f);
    printf("  MCK/32:  %10u Hz (%.2f MHz)\n", mck / 32, (mck / 32) / 1000000.0f);
    printf("  MCK/128: %10u Hz (%.2f MHz)\n", mck / 128, (mck / 128) / 1000000.0f);

    // PWM frequency calculation
    printf("\n--- PWM Maximum Frequencies (16-bit counter) ---\n");
    printf("  MCK/1:    %10u Hz (%.2f kHz)\n", mck / 65536, (mck / 65536) / 1000.0f);
    printf("  MCK/2:    %10u Hz (%.2f kHz)\n", (mck / 2) / 65536, ((mck / 2) / 65536) / 1000.0f);
    printf("  MCK/4:    %10u Hz (%.2f kHz)\n", (mck / 4) / 65536, ((mck / 4) / 65536) / 1000.0f);
    printf("  MCK/8:    %10u Hz (%.2f kHz)\n", (mck / 8) / 65536, ((mck / 8) / 65536) / 1000.0f);
}

// ============================================================================
// Main Function
// ============================================================================

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        SAME70 Clock (PMC) Examples                        ║\n");
    printf("║        Power Management and Clock Configuration           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    // Note: In a real system, you would only initialize clocks once
    // These examples are for demonstration purposes

    example1_basic_initialization();
    example2_frequency_reporting();
    example3_peripheral_clock_management();

    // Uncomment to test other configurations:
    // example4_conservative_configuration();
    // example5_low_power_configuration();
    // example6_custom_configuration();

    example7_clock_dependent_peripherals();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  All Clock examples completed successfully!               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
