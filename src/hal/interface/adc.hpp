/// Platform-agnostic ADC (Analog-to-Digital Converter) interface
///
/// Defines ADC concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_ADC_HPP
#define ALLOY_HAL_INTERFACE_ADC_HPP

#include <concepts>
#include <functional>
#include <span>

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/// ADC resolution options
enum class AdcResolution : u8 {
    Bits6 = 6,    ///< 6-bit resolution (0-63)
    Bits8 = 8,    ///< 8-bit resolution (0-255)
    Bits10 = 10,  ///< 10-bit resolution (0-1023)
    Bits12 = 12,  ///< 12-bit resolution (0-4095, most common)
    Bits14 = 14,  ///< 14-bit resolution (0-16383)
    Bits16 = 16   ///< 16-bit resolution (0-65535)
};

/// ADC reference voltage source
enum class AdcReference : u8 {
    Internal,  ///< Internal reference voltage (typically 1.2V or 2.5V)
    External,  ///< External reference voltage (VREF pin)
    Vdd        ///< Supply voltage (VDD/VDDA)
};

/// ADC sample time (conversion speed vs accuracy trade-off)
enum class AdcSampleTime : u8 {
    Cycles1_5 = 0,   ///< 1.5 cycles (fastest, least accurate)
    Cycles7_5 = 1,   ///< 7.5 cycles
    Cycles13_5 = 2,  ///< 13.5 cycles
    Cycles28_5 = 3,  ///< 28.5 cycles
    Cycles41_5 = 4,  ///< 41.5 cycles
    Cycles55_5 = 5,  ///< 55.5 cycles
    Cycles71_5 = 6,  ///< 71.5 cycles
    Cycles84 = 7,    ///< 84 cycles
    Cycles239_5 = 8  ///< 239.5 cycles (slowest, most accurate)
};

/// ADC channel identifier
enum class AdcChannel : u8 {
    Channel0 = 0,
    Channel1 = 1,
    Channel2 = 2,
    Channel3 = 3,
    Channel4 = 4,
    Channel5 = 5,
    Channel6 = 6,
    Channel7 = 7,
    Channel8 = 8,
    Channel9 = 9,
    Channel10 = 10,
    Channel11 = 11,
    Channel12 = 12,
    Channel13 = 13,
    Channel14 = 14,
    Channel15 = 15,
    Channel16 = 16,  ///< Often internal temperature sensor
    Channel17 = 17,  ///< Often internal VREFINT
    Channel18 = 18   ///< Often internal VBAT
};

/// ADC configuration parameters
///
/// Contains all parameters needed to configure an ADC peripheral.
struct AdcConfig {
    AdcResolution resolution;
    AdcReference reference;
    AdcSampleTime sample_time;

    /// Constructor with default configuration
    constexpr AdcConfig(AdcResolution res = AdcResolution::Bits12,
                        AdcReference ref = AdcReference::Vdd,
                        AdcSampleTime sample = AdcSampleTime::Cycles84)
        : resolution(res),
          reference(ref),
          sample_time(sample) {}
};

/// ADC device concept
///
/// Defines the interface that all ADC implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to ADC:
/// - ErrorCode::AdcCalibrationFailed: ADC calibration failed
/// - ErrorCode::AdcOverrun: Data overrun (conversion too fast)
/// - ErrorCode::AdcConversionTimeout: Conversion did not complete in time
/// - ErrorCode::InvalidParameter: Invalid channel or configuration
/// - ErrorCode::NotSupported: Feature not supported by hardware
template <typename T>
concept AdcDevice = requires(T device, const T const_device, AdcChannel channel,
                             std::span<u16> buffer, std::span<AdcChannel> channels,
                             AdcConfig config, std::function<void(u16)> callback) {
    /// Read single ADC channel (blocking)
    ///
    /// Performs a single conversion on the specified channel and returns
    /// the raw ADC value.
    ///
    /// @param channel ADC channel to read
    /// @return Raw ADC value (0 to 2^resolution-1), or error code
    { device.read_single(channel) } -> std::same_as<Result<u16, ErrorCode>>;

    /// Read multiple ADC channels in sequence
    ///
    /// Scans multiple channels and stores results in the buffer.
    /// Buffer size must match number of channels.
    ///
    /// @param channels Array of channels to scan
    /// @param values Buffer to store ADC values
    /// @return Ok on success, error code on failure
    { device.read_multi_channel(channels, buffer) } -> std::same_as<Result<void, ErrorCode>>;

    /// Start continuous conversion mode
    ///
    /// Continuously converts the specified channel and calls the callback
    /// for each new value. Call stop_continuous() to stop.
    ///
    /// @param channel ADC channel to monitor
    /// @param callback Function called for each conversion
    /// @return Ok on success, error code on failure
    { device.start_continuous(channel, callback) } -> std::same_as<Result<void, ErrorCode>>;

    /// Stop continuous conversion mode
    ///
    /// @return Ok on success, error code on failure
    { device.stop_continuous() } -> std::same_as<Result<void, ErrorCode>>;

    /// Start DMA-based data acquisition
    ///
    /// Continuously converts the specified channel and stores results
    /// in the buffer using DMA (no CPU intervention).
    ///
    /// @param channel ADC channel to sample
    /// @param buffer Buffer to store ADC values (filled by DMA)
    /// @return Ok on success, error code on failure
    { device.start_dma(channel, buffer) } -> std::same_as<Result<void, ErrorCode>>;

    /// Stop DMA-based data acquisition
    ///
    /// @return Ok on success, error code on failure
    { device.stop_dma() } -> std::same_as<Result<void, ErrorCode>>;

    /// Check if DMA transfer is complete
    ///
    /// @return true if buffer is full, false if still acquiring
    { const_device.is_dma_complete() } -> std::same_as<bool>;

    /// Calibrate ADC for improved accuracy
    ///
    /// Performs self-calibration if supported by hardware.
    ///
    /// @return Ok on success, error code on failure or if not supported
    { device.calibrate() } -> std::same_as<Result<void, ErrorCode>>;

    /// Configure ADC parameters
    ///
    /// @param config ADC configuration (resolution, reference, sample time)
    /// @return Ok on success, error code on failure
    { device.configure(config) } -> std::same_as<Result<void, ErrorCode>>;
};

/// Helper function to convert raw ADC value to voltage
///
/// @param raw_value Raw ADC reading (0 to max)
/// @param max_value Maximum ADC value (2^resolution - 1)
/// @param reference_voltage Reference voltage in volts (e.g., 3.3V)
/// @return Voltage in volts
inline constexpr float raw_to_voltage(u16 raw_value, u16 max_value,
                                      float reference_voltage) {
    return (static_cast<float>(raw_value) / static_cast<float>(max_value)) * reference_voltage;
}

/// Helper function to convert raw ADC value to percentage
///
/// @param raw_value Raw ADC reading (0 to max)
/// @param max_value Maximum ADC value (2^resolution - 1)
/// @return Percentage (0.0 to 100.0)
inline constexpr float raw_to_percentage(u16 raw_value, u16 max_value) {
    return (static_cast<float>(raw_value) / static_cast<float>(max_value)) * 100.0f;
}

/// Helper function to get maximum ADC value for a given resolution
///
/// @param resolution ADC resolution
/// @return Maximum ADC value (2^resolution - 1)
inline constexpr u16 get_max_adc_value(AdcResolution resolution) {
    return (1u << static_cast<u8>(resolution)) - 1;
}

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_ADC_HPP
