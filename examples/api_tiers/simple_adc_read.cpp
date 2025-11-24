/**
 * @file simple_adc_read.cpp
 * @brief Level 1 Simple API - ADC Analog Reading Example
 *
 * Demonstrates the Simple tier ADC API with one-liner setup.
 * Perfect for quick analog sensor readings and voltage monitoring.
 *
 * This example shows:
 * - One-liner ADC setup with defaults (12-bit, Vdd reference)
 * - Reading analog voltages from potentiometer or sensor
 * - Converting ADC values to millivolts
 * - Visual feedback with LED brightness control
 *
 * Hardware Setup:
 * - Connect potentiometer or voltage divider to ADC pin
 * - Potentiometer: Left pin to GND, right pin to 3.3V, middle pin to ADC
 * - Or voltage sensor: Connect output to ADC pin (0-3.3V range)
 *
 * Common Sensors:
 * - Potentiometer: Variable voltage divider
 * - LDR (Light Dependent Resistor): Light sensor with resistor divider
 * - NTC Thermistor: Temperature sensor with resistor divider
 * - Analog joystick: 2 potentiometers (X and Y axes)
 *
 * Expected Behavior:
 * - Continuously reads ADC value
 * - Blinks LED faster when voltage is higher
 * - LED rate proportional to analog input
 *
 * @note Part of Phase 3.5: ADC Implementation
 * @see docs/API_TIERS.md
 */

// ============================================================================
// Board-Specific Configuration
// ============================================================================

#if defined(PLATFORM_STM32F401RE)
#include "boards/nucleo_f401re/board.hpp"
using namespace board::nucleo_f401re;

// ADC1 Channel 0 (PA0 - A0 on Arduino header)
using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

// ADC1 Channel 0 (PA0 - A0)
using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

// ADC1 Channel 0 (PA0 - A0)
using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

// ADC1 Channel 0 (PA0 - A0)
using AdcPin = Pin<PeripheralId::ADC1, PinId::PA0>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

// AFEC0 Channel 0 (PD30)
using AdcPin = Pin<PeripheralId::AFEC0, PinId::PD30>;
constexpr auto AdcChannelNum = AdcChannel::CH0;

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Convert ADC value to millivolts
 *
 * @param adc_value ADC reading (0-4095 for 12-bit)
 * @param vref_mv Reference voltage in millivolts (typically 3300 mV)
 * @return uint32_t Voltage in millivolts
 */
constexpr uint32_t adc_to_millivolts(uint16_t adc_value, uint32_t vref_mv = 3300) {
    // For 12-bit ADC: voltage = (adc_value * Vref) / 4095
    return (static_cast<uint32_t>(adc_value) * vref_mv) / 4095;
}

/**
 * @brief Convert ADC value to percentage (0-100%)
 *
 * @param adc_value ADC reading (0-4095 for 12-bit)
 * @return uint8_t Percentage (0-100)
 */
constexpr uint8_t adc_to_percentage(uint16_t adc_value) {
    // For 12-bit ADC: percentage = (adc_value * 100) / 4095
    return static_cast<uint8_t>((static_cast<uint32_t>(adc_value) * 100) / 4095);
}

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Simple ADC reading example
 *
 * Demonstrates one-liner ADC setup and basic analog voltage reading.
 */
int main() {
    // ========================================================================
    // Setup
    // ========================================================================

    // Simple API: One-liner setup with defaults (12-bit, Vdd reference)
    auto adc = Adc1Instance::quick_setup<AdcPin>(AdcChannelNum);

    // Initialize ADC peripheral
    auto init_result = adc.initialize();
    if (!init_result.is_ok()) {
        // Initialization failed
        while (true);
    }

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Continuous ADC Reading
    // ========================================================================

    while (true) {
        // Read ADC value (12-bit: 0-4095)
        auto result = adc.read();

        if (result.is_ok()) {
            uint16_t adc_value = result.unwrap();

            // Convert to useful units
            uint32_t voltage_mv = adc_to_millivolts(adc_value);
            uint8_t percentage = adc_to_percentage(adc_value);

            // ================================================================
            // Visual Feedback Based on ADC Value
            // ================================================================

            // Blink LED faster when voltage is higher
            // Low voltage (0V)   → slow blink (500ms)
            // High voltage (3.3V) → fast blink (50ms)

            uint32_t blink_delay = 500000 - (adc_value * 110);  // 500ms to 50ms range

            led.toggle();
            for (volatile uint32_t i = 0; i < blink_delay; ++i);

            // ================================================================
            // Example: Voltage Thresholds
            // ================================================================

            // You can use thresholds for different actions
            if (voltage_mv < 500) {
                // Very low voltage (< 0.5V)
                // Action: Turn off LED
                led.clear();
            } else if (voltage_mv < 1650) {
                // Low voltage (0.5V - 1.65V)
                // Action: Slow blink
            } else if (voltage_mv < 2500) {
                // Medium voltage (1.65V - 2.5V)
                // Action: Medium blink
            } else {
                // High voltage (> 2.5V)
                // Action: Fast blink
            }

            // ================================================================
            // Example: Battery Voltage Monitoring
            // ================================================================

            // If using a voltage divider to monitor battery (e.g., LiPo 4.2V)
            // Voltage divider: R1=10k, R2=10k → Vout = Vbat / 2
            // ADC reads Vout, so Vbat = 2 * voltage_mv

            // uint32_t battery_mv = voltage_mv * 2;  // Scale back up
            // if (battery_mv < 3300) {
            //     // Low battery warning (< 3.3V for LiPo)
            //     // Blink LED rapidly
            // }

        } else {
            // Error reading ADC
            // Blink LED in error pattern (very fast)
            led.toggle();
            for (volatile uint32_t i = 0; i < 50000; ++i);
        }

        // Delay between readings (100 Hz sampling rate)
        for (volatile uint32_t i = 0; i < 100000; ++i);
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. One-liner setup: quick_setup() handles all configuration
 * 2. Default settings: 12-bit resolution, Vdd reference (3.3V)
 * 3. Result pattern: read() returns Result<uint16_t, ErrorCode>
 * 4. Conversion helpers: adc_to_millivolts(), adc_to_percentage()
 * 5. Visual feedback: LED blink rate proportional to voltage
 *
 * ADC Value Range:
 * - 12-bit ADC: 0 to 4095 (2^12 - 1)
 * - 0 ADC counts = 0.0V
 * - 2048 ADC counts = 1.65V (half of 3.3V)
 * - 4095 ADC counts = 3.3V (Vdd reference)
 *
 * Resolution:
 * - 12-bit: 3300 mV / 4095 = 0.806 mV per count
 * - Good for: General-purpose analog sensing
 * - Too coarse for: High-precision measurements (use Expert API with oversampling)
 *
 * Common Analog Sensors:
 *
 * 1. Potentiometer (10k):
 *    - Voltage range: 0V to 3.3V
 *    - Use directly: connect wiper to ADC pin
 *    - Example: Volume control, position sensing
 *
 * 2. LDR (Light Sensor):
 *    - Voltage divider: LDR + 10k resistor
 *    - Dark: High resistance → low voltage
 *    - Bright: Low resistance → high voltage
 *    - Example: Auto-brightness, light-sensitive switch
 *
 * 3. NTC Thermistor (10k @ 25°C):
 *    - Voltage divider: NTC + 10k resistor
 *    - Use Steinhart-Hart equation for temperature
 *    - Example: T = 1 / (A + B*ln(R) + C*ln(R)^3)
 *    - Coefficients: A, B, C from datasheet
 *
 * 4. Analog Joystick:
 *    - 2 potentiometers (X, Y axes)
 *    - Center: ~1.65V (50%)
 *    - Range: 0V (left/down) to 3.3V (right/up)
 *    - Example: Game controller, robot control
 *
 * Input Protection:
 * - ADC input range: 0V to Vdd (3.3V max!)
 * - Overvoltage: Use voltage divider or clamp diodes
 * - Undervoltage: ADC reads 0 (no damage)
 * - ESD protection: Built-in, but external TVS diode recommended
 *
 * Sampling Rate:
 * - This example: ~100 Hz (10ms per sample)
 * - Maximum: ~1 MSPS @ 12-bit (varies by platform)
 * - For high-speed: Use Expert API with DMA
 *
 * Accuracy Tips:
 * 1. Use stable Vref+ if Vdd fluctuates
 * 2. Add 100nF bypass capacitor near ADC pin
 * 3. Use longer sample time for high-impedance sources
 * 4. Average multiple readings to reduce noise
 * 5. Calibrate on STM32F1 after power-on
 *
 * Troubleshooting:
 * - Reading always 0: Check pin configuration, voltage source
 * - Reading always 4095: Overvoltage or short to Vdd
 * - Noisy readings: Add capacitor, increase sample time, average samples
 * - Unstable readings: Check grounding, avoid long wires, use shielded cable
 */
