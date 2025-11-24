/**
 * @file fluent_adc_multi_channel.cpp
 * @brief Level 2 Fluent API - Multi-Channel ADC Acquisition Example
 *
 * Demonstrates the Fluent tier ADC API with builder pattern for custom configuration.
 * Perfect for applications with multiple analog sensors and data logging.
 *
 * This example shows:
 * - Builder pattern for custom ADC configuration
 * - Multi-channel analog acquisition (temperature, light, voltage)
 * - Sensor data processing and filtering
 * - Data logging pattern with circular buffer
 *
 * Hardware Setup:
 * - CH0 (PA0/A0): NTC Thermistor with 10k resistor divider
 * - CH1 (PA1/A1): LDR (light sensor) with 10k resistor divider
 * - CH2 (PA4/A2): Battery voltage divider (R1=10k, R2=10k)
 *
 * Sensor Circuits:
 * 1. Temperature (NTC 10k @ 25°C):
 *    3.3V --- [NTC 10k] --- ADC CH0 --- [10k] --- GND
 *
 * 2. Light (LDR):
 *    3.3V --- [LDR] --- ADC CH1 --- [10k] --- GND
 *
 * 3. Battery Voltage:
 *    Vbat --- [10k] --- ADC CH2 --- [10k] --- GND
 *
 * Expected Behavior:
 * - Reads 3 channels in sequence
 * - Converts raw ADC to physical units (°C, lux, mV)
 * - Stores data in circular buffer
 * - Blinks LED based on sensor readings
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

using TempPin = Pin<PeripheralId::ADC1, PinId::PA0>;    // A0
using LightPin = Pin<PeripheralId::ADC1, PinId::PA1>;   // A1
using BattPin = Pin<PeripheralId::ADC1, PinId::PA4>;    // A2

#elif defined(PLATFORM_STM32F722ZE)
#include "boards/nucleo_f722ze/board.hpp"
using namespace board::nucleo_f722ze;

using TempPin = Pin<PeripheralId::ADC1, PinId::PA0>;
using LightPin = Pin<PeripheralId::ADC1, PinId::PA1>;
using BattPin = Pin<PeripheralId::ADC1, PinId::PA4>;

#elif defined(PLATFORM_STM32G071RB)
#include "boards/nucleo_g071rb/board.hpp"
using namespace board::nucleo_g071rb;

using TempPin = Pin<PeripheralId::ADC1, PinId::PA0>;
using LightPin = Pin<PeripheralId::ADC1, PinId::PA1>;
using BattPin = Pin<PeripheralId::ADC1, PinId::PA4>;

#elif defined(PLATFORM_STM32G0B1RE)
#include "boards/nucleo_g0b1re/board.hpp"
using namespace board::nucleo_g0b1re;

using TempPin = Pin<PeripheralId::ADC1, PinId::PA0>;
using LightPin = Pin<PeripheralId::ADC1, PinId::PA1>;
using BattPin = Pin<PeripheralId::ADC1, PinId::PA4>;

#elif defined(PLATFORM_SAME70)
#include "boards/same70_xplained/board.hpp"
using namespace board::same70_xplained;

using TempPin = Pin<PeripheralId::AFEC0, PinId::PD30>;
using LightPin = Pin<PeripheralId::AFEC0, PinId::PD31>;
using BattPin = Pin<PeripheralId::AFEC0, PinId::PD32>;

#else
#error "Unsupported platform. Please define one of the supported platforms."
#endif

#include "hal/gpio.hpp"

using namespace ucore;
using namespace ucore::hal;

// ============================================================================
// Sensor Data Structures
// ============================================================================

struct SensorData {
    uint16_t temp_raw;        // Raw ADC value (0-4095)
    uint16_t light_raw;       // Raw ADC value (0-4095)
    uint16_t battery_raw;     // Raw ADC value (0-4095)

    int16_t temperature_c;    // Temperature in °C * 10 (e.g., 255 = 25.5°C)
    uint16_t light_lux;       // Light in lux (approximate)
    uint16_t battery_mv;      // Battery voltage in mV
};

// Circular buffer for data logging
constexpr size_t BUFFER_SIZE = 100;
SensorData data_buffer[BUFFER_SIZE];
size_t buffer_index = 0;

// ============================================================================
// Sensor Conversion Functions
// ============================================================================

/**
 * @brief Convert NTC thermistor ADC value to temperature
 *
 * Uses simplified Beta equation for NTC 10k thermistor:
 * T = 1 / (1/T0 + (1/B) * ln(R/R0))
 *
 * @param adc_value ADC reading (0-4095)
 * @return int16_t Temperature in °C * 10 (e.g., 255 = 25.5°C)
 */
int16_t ntc_to_temperature(uint16_t adc_value) {
    // NTC parameters (10k @ 25°C, Beta = 3950)
    constexpr float R0 = 10000.0f;      // Resistance at 25°C
    constexpr float T0 = 298.15f;       // 25°C in Kelvin
    constexpr float BETA = 3950.0f;     // Beta coefficient
    constexpr float R_SERIES = 10000.0f; // Series resistor

    // Calculate NTC resistance from voltage divider
    // V_adc = Vdd * (R_series / (R_ntc + R_series))
    // R_ntc = R_series * (4095 / adc_value - 1)

    if (adc_value == 0) return -500;  // Prevent division by zero

    float resistance = R_SERIES * ((4095.0f / adc_value) - 1.0f);

    // Beta equation: T = 1 / (1/T0 + (1/Beta) * ln(R/R0))
    float ln_r = 0.0f;  // ln(resistance / R0) - simplified
    if (resistance > 0) {
        // Simplified approximation (avoids floating point log)
        // For production, use lookup table or integer log approximation
        float ratio = resistance / R0;
        if (ratio > 1.0f) {
            ln_r = (ratio - 1.0f) * 0.5f;  // Approximation
        } else {
            ln_r = -(1.0f - ratio) * 0.5f;
        }
    }

    float temp_k = 1.0f / (1.0f / T0 + (1.0f / BETA) * ln_r);
    float temp_c = temp_k - 273.15f;

    return static_cast<int16_t>(temp_c * 10.0f);  // Return °C * 10
}

/**
 * @brief Convert LDR ADC value to approximate lux
 *
 * @param adc_value ADC reading (0-4095)
 * @return uint16_t Approximate light intensity in lux
 */
uint16_t ldr_to_lux(uint16_t adc_value) {
    // Simplified mapping for typical LDR (GL5528)
    // Dark (10k-1M ohms): ~0-10 lux
    // Room light (5-10k ohms): ~100-500 lux
    // Bright (1-5k ohms): ~1000+ lux

    // Linear approximation: lux ≈ adc_value / 4
    return adc_value / 4;
}

/**
 * @brief Convert battery divider ADC value to voltage
 *
 * @param adc_value ADC reading (0-4095)
 * @return uint16_t Battery voltage in mV
 */
uint16_t battery_to_millivolts(uint16_t adc_value) {
    // Voltage divider: Vout = Vbat / 2 (R1 = R2 = 10k)
    // ADC reads Vout, so Vbat = 2 * Vout

    uint32_t vout_mv = (static_cast<uint32_t>(adc_value) * 3300) / 4095;
    return static_cast<uint16_t>(vout_mv * 2);  // Scale back up
}

// ============================================================================
// Multi-Channel ADC Acquisition
// ============================================================================

/**
 * @brief Multi-channel ADC acquisition class
 */
template <typename AdcBuilder>
class MultiChannelAdc {
public:
    explicit MultiChannelAdc(AdcBuilder& builder) : m_builder(builder) {}

    /**
     * @brief Read all channels and convert to sensor data
     */
    Result<SensorData, ErrorCode> read_all_sensors() {
        SensorData data = {};

        // Read temperature channel (CH0)
        auto temp_result = read_channel(AdcChannel::CH0);
        if (temp_result.is_ok()) {
            data.temp_raw = temp_result.unwrap();
            data.temperature_c = ntc_to_temperature(data.temp_raw);
        }

        // Read light channel (CH1)
        auto light_result = read_channel(AdcChannel::CH1);
        if (light_result.is_ok()) {
            data.light_raw = light_result.unwrap();
            data.light_lux = ldr_to_lux(data.light_raw);
        }

        // Read battery channel (CH2)
        auto batt_result = read_channel(AdcChannel::CH2);
        if (batt_result.is_ok()) {
            data.battery_raw = batt_result.unwrap();
            data.battery_mv = battery_to_millivolts(data.battery_raw);
        }

        return Ok(data);
    }

private:
    /**
     * @brief Read single ADC channel
     */
    Result<uint16_t, ErrorCode> read_channel(AdcChannel channel) {
        // Reconfigure builder for this channel
        auto adc = m_builder.channel(channel).initialize();

        if (!adc.is_ok()) {
            return Err(adc.err());
        }

        // Read ADC value
        return adc.unwrap().read();
    }

    AdcBuilder& m_builder;
};

// ============================================================================
// Main Application
// ============================================================================

/**
 * @brief Fluent multi-channel ADC example
 *
 * Demonstrates builder pattern for multi-sensor data acquisition.
 */
int main() {
    // ========================================================================
    // Setup with Fluent Builder Pattern
    // ========================================================================

    // Fluent API: Configure ADC with custom settings
    auto adc_builder = Adc1BuilderInstance()
        .resolution(AdcResolution::Bits12)     // 12-bit resolution
        .bits_12()                              // Or use convenience method
        .initialize();

    if (!adc_builder.is_ok()) {
        // Initialization failed
        while (true);
    }

    auto adc = adc_builder.unwrap();
    auto multi_adc = MultiChannelAdc(adc);

    // Setup LED for visual feedback
    auto led = LedPin::output();

    // ========================================================================
    // Continuous Multi-Channel Acquisition
    // ========================================================================

    while (true) {
        // Read all sensors
        auto result = multi_adc.read_all_sensors();

        if (result.is_ok()) {
            SensorData data = result.unwrap();

            // Store in circular buffer
            data_buffer[buffer_index] = data;
            buffer_index = (buffer_index + 1) % BUFFER_SIZE;

            // ================================================================
            // Sensor Data Processing
            // ================================================================

            // Temperature: 25.5°C → data.temperature_c = 255
            int16_t temp_c = data.temperature_c / 10;
            int16_t temp_frac = data.temperature_c % 10;

            // Light: 0-1023 lux (approximate)
            uint16_t light_lux = data.light_lux;

            // Battery: 3300-4200 mV (for LiPo)
            uint16_t battery_mv = data.battery_mv;

            // ================================================================
            // Alert Conditions
            // ================================================================

            // Temperature alert (> 30°C)
            bool temp_high = (data.temperature_c > 300);

            // Low light alert (< 50 lux)
            bool light_low = (light_lux < 50);

            // Low battery alert (< 3.3V)
            bool battery_low = (battery_mv < 3300);

            // ================================================================
            // Visual Feedback
            // ================================================================

            if (battery_low) {
                // Rapid blink for low battery (critical)
                led.toggle();
                for (volatile uint32_t i = 0; i < 50000; ++i);
            } else if (temp_high) {
                // Fast blink for high temperature (warning)
                led.toggle();
                for (volatile uint32_t i = 0; i < 200000; ++i);
            } else {
                // Slow blink for normal operation
                led.toggle();
                for (volatile uint32_t i = 0; i < 500000; ++i);
            }

            // ================================================================
            // Data Logging Pattern
            // ================================================================

            // In a real application, you would:
            // 1. Store data to SD card, flash, or EEPROM
            // 2. Transmit data via UART, SPI, or wireless
            // 3. Calculate statistics (min, max, average)
            // 4. Trigger actions based on thresholds

            // Example: Calculate average temperature over last 10 samples
            int32_t temp_sum = 0;
            size_t samples_to_avg = 10;
            for (size_t i = 0; i < samples_to_avg; ++i) {
                size_t idx = (buffer_index + BUFFER_SIZE - i - 1) % BUFFER_SIZE;
                temp_sum += data_buffer[idx].temperature_c;
            }
            int16_t temp_avg = static_cast<int16_t>(temp_sum / samples_to_avg);

        } else {
            // Error reading sensors
            led.toggle();
            for (volatile uint32_t i = 0; i < 100000; ++i);
        }

        // Sampling rate: 10 Hz (100ms per cycle)
        for (volatile uint32_t i = 0; i < 1000000; ++i);
    }

    return 0;
}

/**
 * Key Points:
 *
 * 1. Fluent builder: Method chaining for readable configuration
 * 2. Multi-channel: Read 3 sensors (temperature, light, battery)
 * 3. Sensor conversion: Raw ADC → physical units (°C, lux, mV)
 * 4. Circular buffer: Store last 100 samples for analysis
 * 5. Alert conditions: Temperature, light, battery thresholds
 *
 * Builder Pattern Benefits:
 * - Readable: .resolution().bits_12() is self-documenting
 * - Flexible: Easy to add/remove configuration steps
 * - Type-safe: Compiler catches invalid configurations
 * - Chainable: Multiple configurations in one statement
 *
 * Multi-Channel Strategies:
 *
 * 1. Sequential (this example):
 *    - Read CH0, then CH1, then CH2
 *    - Simple, deterministic timing
 *    - Total time: 3 × conversion time
 *
 * 2. Scan Mode (Expert API with DMA):
 *    - Configure sequence in hardware
 *    - DMA transfers results automatically
 *    - Zero CPU overhead
 *
 * 3. Simultaneous (Dual/Triple ADC):
 *    - Use multiple ADCs in parallel
 *    - Sample all channels at exact same time
 *    - Best for synchronized measurements
 *
 * Sensor Calibration:
 *
 * 1. Temperature (NTC):
 *    - Measure at known temperature (ice water = 0°C)
 *    - Adjust Beta coefficient or use lookup table
 *    - Accuracy: ±1-2°C typical
 *
 * 2. Light (LDR):
 *    - Calibrate with lux meter at known illumination
 *    - Create piecewise linear mapping
 *    - Accuracy: ±20% typical (non-linear response)
 *
 * 3. Battery Voltage:
 *    - Measure with multimeter
 *    - Adjust divider ratio or Vref
 *    - Accuracy: ±1-2% typical
 *
 * Data Logging Best Practices:
 * 1. Use circular buffer to avoid memory overflow
 * 2. Store timestamp with each sample (use RTC)
 * 3. Calculate statistics (min, max, avg, std dev)
 * 4. Compress data before storage (delta encoding)
 * 5. Add checksum for data integrity
 *
 * Performance:
 * - 3 channels @ 10 Hz = 30 samples/sec
 * - With 12-bit ADC: 60 bytes/sec (uncompressed)
 * - SD card: Can log for days/weeks
 * - UART: Can transmit real-time @ 9600 baud
 *
 * Advanced Features (see Expert API):
 * - DMA circular mode for continuous acquisition
 * - Hardware averaging (oversampling on STM32G0)
 * - Trigger from timer for precise sampling
 * - Low-power modes between conversions
 */
