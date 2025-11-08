/**
 * @file adc_simple_example.cpp
 * @brief Simple ADC example for SAME70 without DMA
 *
 * This example demonstrates basic ADC usage for reading analog inputs.
 * Perfect for reading sensors, potentiometers, battery voltage, etc.
 *
 * Hardware Setup:
 * - Connect analog signal to AD0 (PD30) on AFEC0
 * - Voltage range: 0V to 3.3V
 * - Connect GND and ADVREF
 *
 * Use Cases:
 * - Temperature sensor reading
 * - Potentiometer position
 * - Battery voltage monitoring
 * - Light sensor (photodiode/phototransistor)
 */

#include "hal/platform/same70/adc.hpp"

using namespace alloy::hal::same70;

/**
 * @brief Simple ADC wrapper for sensor readings
 */
class AnalogSensor {
   public:
    constexpr AnalogSensor(AdcChannel channel) : m_channel(channel) {}

    auto init() -> alloy::core::Result<void> {
        // Initialize ADC
        auto result = m_adc.open();
        if (!result.is_ok()) {
            return result;
        }

        // Configure ADC for software trigger
        AdcConfig config;
        config.resolution = AdcResolution::Bits12;
        config.trigger = AdcTrigger::Software;
        config.use_dma = false;

        return m_adc.configure(config);
    }

    /**
     * @brief Read raw ADC value (0-4095 for 12-bit)
     */
    auto readRaw() -> alloy::core::Result<uint16_t> { return m_adc.readSingle(m_channel); }

    /**
     * @brief Read voltage in millivolts
     */
    auto readVoltage(uint32_t vref_mv = 3300) -> alloy::core::Result<uint32_t> {
        auto result = readRaw();
        if (!result.is_ok()) {
            return alloy::core::Result<uint32_t>::error(result.error());
        }

        uint32_t voltage = Adc0::toVoltage(result.value(), vref_mv);
        return alloy::core::Result<uint32_t>::ok(voltage);
    }

    /**
     * @brief Read percentage (0-100%)
     *
     * Useful for potentiometers, battery level indicators, etc.
     */
    auto readPercentage() -> alloy::core::Result<uint8_t> {
        auto result = readRaw();
        if (!result.is_ok()) {
            return alloy::core::Result<uint8_t>::error(result.error());
        }

        uint8_t percentage = static_cast<uint8_t>((result.value() * 100) / 4095);
        return alloy::core::Result<uint8_t>::ok(percentage);
    }

    auto close() -> alloy::core::Result<void> { return m_adc.close(); }

   private:
    Adc0 m_adc;
    AdcChannel m_channel;
};

/**
 * @brief Multi-channel ADC example
 *
 * Demonstrates reading multiple channels sequentially.
 */
class MultiChannelAdc {
   public:
    auto init() -> alloy::core::Result<void> {
        auto result = m_adc.open();
        if (!result.is_ok()) {
            return result;
        }

        AdcConfig config;
        config.resolution = AdcResolution::Bits12;
        config.trigger = AdcTrigger::Software;
        config.channels = 4;  // Reading 4 channels

        return m_adc.configure(config);
    }

    /**
     * @brief Read multiple channels
     *
     * @param channels Array of channels to read
     * @param values Output buffer for readings
     * @param count Number of channels
     */
    auto readChannels(const AdcChannel* channels, uint16_t* values, size_t count)
        -> alloy::core::Result<void> {
        // Enable all channels
        for (size_t i = 0; i < count; ++i) {
            auto result = m_adc.enableChannel(channels[i]);
            if (!result.is_ok()) {
                return result;
            }
        }

        // Start conversion
        auto start_result = m_adc.startConversion();
        if (!start_result.is_ok()) {
            return start_result;
        }

        // Read all channels
        for (size_t i = 0; i < count; ++i) {
            auto result = m_adc.read(channels[i]);
            if (!result.is_ok()) {
                return alloy::core::Result<void>::error(result.error());
            }
            values[i] = result.value();
        }

        // Disable all channels
        for (size_t i = 0; i < count; ++i) {
            m_adc.disableChannel(channels[i]);
        }

        return alloy::core::Result<void>::ok();
    }

    auto close() -> alloy::core::Result<void> { return m_adc.close(); }

   private:
    Adc0 m_adc;
};

/**
 * @brief Example usage
 */
int main() {
    // ========================================================================
    // Example 1: Single channel sensor reading
    // ========================================================================
    {
        // Create sensor on channel 0 (AD0)
        AnalogSensor sensor(AdcChannel::CH0);

        [[maybe_unused]] auto init_result = sensor.init();

        // Read raw ADC value (0-4095)
        [[maybe_unused]] auto raw_result = sensor.readRaw();
        uint16_t raw_value = 0;
        if (raw_result.is_ok()) {
            raw_value = raw_result.value();
        }

        // Read voltage in millivolts
        [[maybe_unused]] auto voltage_result = sensor.readVoltage();
        uint32_t voltage_mv = 0;
        if (voltage_result.is_ok()) {
            voltage_mv = voltage_result.value();
        }

        // Read as percentage
        [[maybe_unused]] auto percentage_result = sensor.readPercentage();
        uint8_t percentage = 0;
        if (percentage_result.is_ok()) {
            percentage = percentage_result.value();
        }

        [[maybe_unused]] auto close_result = sensor.close();

        // Use the values (prevent unused warnings)
        (void)raw_value;
        (void)voltage_mv;
        (void)percentage;
    }

    // ========================================================================
    // Example 2: Multi-channel reading
    // ========================================================================
    {
        MultiChannelAdc multi_adc;

        [[maybe_unused]] auto init_result = multi_adc.init();

        // Read 4 channels
        AdcChannel channels[] = {AdcChannel::CH0, AdcChannel::CH1, AdcChannel::CH2,
                                 AdcChannel::CH3};
        uint16_t values[4];

        [[maybe_unused]] auto read_result = multi_adc.readChannels(channels, values, 4);

        // Process readings
        for (size_t i = 0; i < 4; ++i) {
            uint32_t voltage = Adc0::toVoltage(values[i]);
            (void)voltage;  // Use the voltage
        }

        [[maybe_unused]] auto close_result = multi_adc.close();
    }

    // ========================================================================
    // Example 3: Direct ADC usage
    // ========================================================================
    {
        auto adc = Adc0{};

        [[maybe_unused]] auto open_result = adc.open();

        AdcConfig config;
        config.resolution = AdcResolution::Bits12;
        config.trigger = AdcTrigger::Software;
        [[maybe_unused]] auto config_result = adc.configure(config);

        // Single reading
        [[maybe_unused]] auto value_result = adc.readSingle(AdcChannel::CH0);

        [[maybe_unused]] auto close_result = adc.close();
    }

    return 0;
}
