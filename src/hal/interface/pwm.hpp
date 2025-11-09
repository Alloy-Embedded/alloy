/// Platform-agnostic PWM (Pulse Width Modulation) interface
///
/// Defines PWM concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_PWM_HPP
#define ALLOY_HAL_INTERFACE_PWM_HPP

#include <concepts>
#include <span>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

/// PWM channel identifier
enum class PwmChannel : core::u8 { Channel1 = 0, Channel2 = 1, Channel3 = 2, Channel4 = 3 };

/// PWM resolution (bits of duty cycle control)
enum class PwmResolution : core::u8 {
    Bits8 = 8,    ///< 8-bit resolution (0-255 steps)
    Bits10 = 10,  ///< 10-bit resolution (0-1023 steps)
    Bits12 = 12,  ///< 12-bit resolution (0-4095 steps)
    Bits16 = 16   ///< 16-bit resolution (0-65535 steps)
};

/// PWM output polarity
enum class PwmPolarity : core::u8 {
    Normal,   ///< Active high (default)
    Inverted  ///< Active low (inverted)
};

/// PWM configuration parameters
///
/// Contains all parameters needed to configure a PWM peripheral.
struct PwmConfig {
    core::u32 frequency_hz;    ///< PWM frequency in Hz (e.g., 1000 for 1kHz)
    PwmResolution resolution;  ///< Duty cycle resolution
    PwmPolarity polarity;      ///< Output polarity

    /// Constructor with default configuration
    constexpr PwmConfig(core::u32 freq = 1000, PwmResolution res = PwmResolution::Bits12,
                        PwmPolarity pol = PwmPolarity::Normal)
        : frequency_hz(freq),
          resolution(res),
          polarity(pol) {}
};

/// PWM complementary output configuration (for motor control)
///
/// Used for H-bridge motor drivers where two outputs must never be
/// active simultaneously (to prevent shoot-through).
struct PwmComplementaryConfig {
    PwmChannel channel;      ///< Main PWM channel
    core::u16 dead_time_ns;  ///< Dead time in nanoseconds (prevents shoot-through)

    /// Constructor
    constexpr PwmComplementaryConfig(PwmChannel ch, core::u16 dead_ns = 100)
        : channel(ch),
          dead_time_ns(dead_ns) {}
};

/// PWM device concept
///
/// Defines the interface that all PWM implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to PWM:
/// - ErrorCode::InvalidParameter: Invalid channel, frequency, or duty cycle
/// - ErrorCode::NotSupported: Feature not supported by hardware
/// - ErrorCode::OutOfRange: Frequency or duty cycle out of valid range
template <typename T>
concept PwmDevice = requires(
    T device, const T const_device, PwmChannel channel, float duty_cycle_percent,
    core::u16 duty_cycle_ticks, core::u32 frequency_hz, PwmConfig config,
    PwmComplementaryConfig comp_config, PwmPolarity polarity, std::span<PwmChannel> channels) {
    /// Set PWM duty cycle as percentage (0-100%)
    ///
    /// @param channel PWM channel
    /// @param percent Duty cycle as percentage (0.0 = 0%, 100.0 = 100%)
    /// @return Ok on success, error code on failure
    { device.set_duty_cycle(channel, duty_cycle_percent) } -> std::same_as<core::Result<void>>;

    /// Set PWM duty cycle as absolute ticks
    ///
    /// Provides precise control using timer ticks directly.
    ///
    /// @param channel PWM channel
    /// @param ticks Duty cycle in timer ticks (0 to 2^resolution-1)
    /// @return Ok on success, error code on failure
    { device.set_duty_cycle_ticks(channel, duty_cycle_ticks) } -> std::same_as<core::Result<void>>;

    /// Set PWM frequency
    ///
    /// Changes the PWM frequency. Note that this may affect all channels
    /// on the same timer peripheral.
    ///
    /// @param frequency_hz Frequency in Hz (e.g., 1000 for 1kHz)
    /// @return Ok on success, error code on failure
    { device.set_frequency(frequency_hz) } -> std::same_as<core::Result<void>>;

    /// Start PWM output on channel
    ///
    /// @param channel PWM channel to start
    /// @return Ok on success, error code on failure
    { device.start(channel) } -> std::same_as<core::Result<void>>;

    /// Stop PWM output on channel
    ///
    /// @param channel PWM channel to stop
    /// @return Ok on success, error code on failure
    { device.stop(channel) } -> std::same_as<core::Result<void>>;

    /// Set PWM output polarity
    ///
    /// @param channel PWM channel
    /// @param polarity Normal (active high) or Inverted (active low)
    /// @return Ok on success, error code on failure
    { device.set_polarity(channel, polarity) } -> std::same_as<core::Result<void>>;

    /// Configure complementary PWM outputs with dead-time
    ///
    /// For H-bridge motor control. Generates two complementary signals
    /// with dead-time insertion to prevent shoot-through.
    ///
    /// @param config Complementary configuration
    /// @return Ok on success, error code on failure or if not supported
    { device.configure_complementary(comp_config) } -> std::same_as<core::Result<void>>;

    /// Synchronize multiple PWM channels
    ///
    /// Starts multiple channels simultaneously for coordinated control
    /// (e.g., RGB LED, 3-phase motor).
    ///
    /// @param channels Array of channels to synchronize
    /// @return Ok on success, error code on failure or if not supported
    { device.synchronize(channels) } -> std::same_as<core::Result<void>>;

    /// Start synchronized PWM channels
    ///
    /// Starts all previously synchronized channels at the same instant.
    ///
    /// @return Ok on success, error code on failure
    { device.start_synchronized() } -> std::same_as<core::Result<void>>;

    /// Configure PWM parameters
    ///
    /// @param config PWM configuration (frequency, resolution, polarity)
    /// @return Ok on success, error code on failure
    { device.configure(config) } -> std::same_as<core::Result<void>>;
};

/// Helper function to convert duty cycle percentage to ticks
///
/// @param percent Duty cycle as percentage (0.0 to 100.0)
/// @param max_ticks Maximum tick value (2^resolution - 1)
/// @return Duty cycle in ticks
inline constexpr core::u16 percent_to_ticks(float percent, core::u16 max_ticks) {
    if (percent < 0.0f)
        percent = 0.0f;
    if (percent > 100.0f)
        percent = 100.0f;
    return static_cast<core::u16>((percent / 100.0f) * static_cast<float>(max_ticks));
}

/// Helper function to convert duty cycle ticks to percentage
///
/// @param ticks Duty cycle in ticks
/// @param max_ticks Maximum tick value (2^resolution - 1)
/// @return Duty cycle as percentage (0.0 to 100.0)
inline constexpr float ticks_to_percent(core::u16 ticks, core::u16 max_ticks) {
    return (static_cast<float>(ticks) / static_cast<float>(max_ticks)) * 100.0f;
}

/// Helper function to get maximum PWM ticks for a given resolution
///
/// @param resolution PWM resolution
/// @return Maximum tick value (2^resolution - 1)
inline constexpr core::u16 get_max_pwm_ticks(PwmResolution resolution) {
    return (1u << static_cast<core::u8>(resolution)) - 1;
}

/// Helper function to calculate PWM period in microseconds
///
/// @param frequency_hz PWM frequency in Hz
/// @return Period in microseconds
inline constexpr core::u32 frequency_to_period_us(core::u32 frequency_hz) {
    return 1000000u / frequency_hz;
}

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_PWM_HPP
