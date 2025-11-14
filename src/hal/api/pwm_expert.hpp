/**
 * @file pwm_expert.hpp
 * @brief Level 3 Expert API for PWM
 *
 * Provides full control over PWM configuration with all parameters exposed.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert PWM configuration
 */
struct PwmExpertConfig {
    u8 channel;                ///< Channel number (0-3)
    u32 frequency_hz;          ///< Output frequency in Hz
    float duty_percent;        ///< Duty cycle percentage (0-100)
    bool inverted;             ///< true = inverted polarity
    u8 prescaler;              ///< Clock prescaler (0-10)
    bool dead_time_enable;     ///< Enable dead-time generation
    u16 dead_time_cycles;      ///< Dead-time in clock cycles

    constexpr bool is_valid() const {
        if (channel > 3) return false;
        if (frequency_hz == 0 || frequency_hz > 100000000) return false;
        if (duty_percent < 0.0f || duty_percent > 100.0f) return false;
        if (prescaler > 10) return false;
        return true;
    }

    constexpr const char* error_message() const {
        if (channel > 3) return "Channel must be 0-3";
        if (frequency_hz == 0) return "Frequency cannot be zero";
        if (frequency_hz > 100000000) return "Frequency too high (max 100 MHz)";
        if (duty_percent < 0.0f || duty_percent > 100.0f) return "Duty must be 0-100%";
        if (prescaler > 10) return "Prescaler must be 0-10";
        return "Valid";
    }

    // Factory methods for common configurations
    static constexpr PwmExpertConfig led_dimming(u8 ch, float duty = 50.0f) {
        return PwmExpertConfig{
            .channel = ch,
            .frequency_hz = 1000,
            .duty_percent = duty,
            .inverted = false,
            .prescaler = 0,
            .dead_time_enable = false,
            .dead_time_cycles = 0
        };
    }

    static constexpr PwmExpertConfig servo_motor(u8 ch, float duty = 7.5f) {
        return PwmExpertConfig{
            .channel = ch,
            .frequency_hz = 50,
            .duty_percent = duty,
            .inverted = false,
            .prescaler = 0,
            .dead_time_enable = false,
            .dead_time_cycles = 0
        };
    }

    static constexpr PwmExpertConfig motor_bridge(u8 ch, float duty, bool inv = false, u16 dead_time = 100) {
        return PwmExpertConfig{
            .channel = ch,
            .frequency_hz = 20000,
            .duty_percent = duty,
            .inverted = inv,
            .prescaler = 0,
            .dead_time_enable = true,
            .dead_time_cycles = dead_time
        };
    }

    static constexpr PwmExpertConfig custom(
        u8 ch, u32 freq_hz, float duty,
        bool inv = false, u8 prescale = 0) {

        return PwmExpertConfig{
            .channel = ch,
            .frequency_hz = freq_hz,
            .duty_percent = duty,
            .inverted = inv,
            .prescaler = prescale,
            .dead_time_enable = false,
            .dead_time_cycles = 0
        };
    }
};

namespace expert {

template <typename PwmPolicy>
Result<void, ErrorCode> configure(const PwmExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // Set prescaler
    PwmPolicy::set_prescaler(config.prescaler);

    // Calculate and set period
    u32 period = PwmPolicy::periph_clock_hz / (1 << config.prescaler) / config.frequency_hz;
    PwmPolicy::set_period(config.channel, period);

    // Set duty cycle
    u32 duty_value = static_cast<u32>((config.duty_percent / 100.0f) * period);
    PwmPolicy::set_duty_cycle(config.channel, duty_value);

    // Set polarity
    PwmPolicy::set_polarity(config.channel, config.inverted);

    // TODO: Implement dead-time configuration (platform-specific)

    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
