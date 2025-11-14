/**
 * @file pwm_simple.hpp
 * @brief Level 1 Simple API for PWM
 *
 * Provides one-liner setup and common presets for PWM channels.
 * Simple API is designed for common use cases like LED dimming and motor control.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief PWM frequency presets for common use cases
 */
struct PwmPresets {
    static constexpr u32 LED_DIMMING_HZ = 1000;      ///< 1 kHz - LED dimming (no flicker)
    static constexpr u32 SERVO_MOTOR_HZ = 50;        ///< 50 Hz - Standard servo PWM
    static constexpr u32 BUZZER_1KHZ = 1000;         ///< 1 kHz - Audible buzzer
    static constexpr u32 BUZZER_2KHZ = 2000;         ///< 2 kHz - High-pitched buzzer
    static constexpr u32 DC_MOTOR_HZ = 20000;        ///< 20 kHz - DC motor (silent)
    static constexpr u32 FAST_SWITCHING_HZ = 100000; ///< 100 kHz - Fast switching
};

/**
 * @brief Simple PWM channel wrapper
 *
 * Provides easy-to-use interface for PWM control.
 *
 * @tparam PwmPolicy PWM hardware policy (e.g., Pwm0Hardware, Pwm1Hardware)
 */
template <typename PwmPolicy>
class SimplePwmChannel {
public:
    constexpr SimplePwmChannel(u8 channel, u32 frequency_hz)
        : channel_(channel), frequency_hz_(frequency_hz), duty_percent_(0) {}

    /**
     * @brief Start PWM output
     */
    Result<void, ErrorCode> start() {
        PwmPolicy::enable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Stop PWM output
     */
    Result<void, ErrorCode> stop() {
        PwmPolicy::disable_channel(channel_);
        return Ok();
    }

    /**
     * @brief Set duty cycle (0-100%)
     *
     * @param percent Duty cycle percentage (0.0 - 100.0)
     * @return Result with error code
     */
    Result<void, ErrorCode> set_duty(float percent) {
        if (percent < 0.0f || percent > 100.0f) {
            return Err(ErrorCode::InvalidParameter);
        }

        duty_percent_ = percent;

        // Calculate period from frequency
        u32 period = PwmPolicy::periph_clock_hz / frequency_hz_;
        u32 duty_value = static_cast<u32>((percent / 100.0f) * period);

        PwmPolicy::set_period(channel_, period);
        PwmPolicy::set_duty_cycle(channel_, duty_value);

        return Ok();
    }

    /**
     * @brief Get current duty cycle
     */
    float get_duty() const { return duty_percent_; }

    /**
     * @brief Get frequency
     */
    u32 get_frequency() const { return frequency_hz_; }

    /**
     * @brief Get channel number
     */
    u8 get_channel() const { return channel_; }

private:
    u8 channel_;
    u32 frequency_hz_;
    float duty_percent_;
};

/**
 * @brief Simple PWM API for common use cases
 *
 * Provides factory methods for quickly creating PWM channels with
 * common configurations.
 *
 * Example usage:
 * @code
 * // LED dimming on channel 0
 * auto led_pwm = Pwm<Pwm0Hardware>::led_dimming(0);
 * led_pwm.set_duty(50.0f);  // 50% brightness
 * led_pwm.start();
 *
 * // Servo control on channel 1
 * auto servo = Pwm<Pwm0Hardware>::servo(1);
 * servo.set_duty(7.5f);  // Center position (1.5ms pulse)
 * servo.start();
 *
 * // DC motor control on channel 2
 * auto motor = Pwm<Pwm1Hardware>::dc_motor(2);
 * motor.set_duty(75.0f);  // 75% speed
 * motor.start();
 * @endcode
 *
 * @tparam PwmPolicy PWM hardware policy
 */
template <typename PwmPolicy>
class Pwm {
public:
    /**
     * @brief Create PWM for LED dimming (1 kHz, no flicker)
     *
     * @param channel PWM channel number (0-3)
     * @return SimplePwmChannel configured for LED dimming
     */
    static SimplePwmChannel<PwmPolicy> led_dimming(u8 channel) {
        return SimplePwmChannel<PwmPolicy>(channel, PwmPresets::LED_DIMMING_HZ);
    }

    /**
     * @brief Create PWM for servo motor (50 Hz standard)
     *
     * Duty cycle controls servo position:
     * - 5% (1ms) = 0 degrees
     * - 7.5% (1.5ms) = 90 degrees (center)
     * - 10% (2ms) = 180 degrees
     *
     * @param channel PWM channel number
     * @return SimplePwmChannel configured for servo
     */
    static SimplePwmChannel<PwmPolicy> servo(u8 channel) {
        return SimplePwmChannel<PwmPolicy>(channel, PwmPresets::SERVO_MOTOR_HZ);
    }

    /**
     * @brief Create PWM for DC motor (20 kHz, silent)
     *
     * @param channel PWM channel number
     * @return SimplePwmChannel configured for DC motor
     */
    static SimplePwmChannel<PwmPolicy> dc_motor(u8 channel) {
        return SimplePwmChannel<PwmPolicy>(channel, PwmPresets::DC_MOTOR_HZ);
    }

    /**
     * @brief Create PWM for buzzer (1 kHz)
     *
     * @param channel PWM channel number
     * @return SimplePwmChannel configured for buzzer
     */
    static SimplePwmChannel<PwmPolicy> buzzer_1khz(u8 channel) {
        return SimplePwmChannel<PwmPolicy>(channel, PwmPresets::BUZZER_1KHZ);
    }

    /**
     * @brief Create PWM for buzzer (2 kHz)
     *
     * @param channel PWM channel number
     * @return SimplePwmChannel configured for high-pitched buzzer
     */
    static SimplePwmChannel<PwmPolicy> buzzer_2khz(u8 channel) {
        return SimplePwmChannel<PwmPolicy>(channel, PwmPresets::BUZZER_2KHZ);
    }

    /**
     * @brief Create PWM with custom frequency
     *
     * @param channel PWM channel number
     * @param frequency_hz Frequency in Hz
     * @return SimplePwmChannel with custom frequency
     */
    static SimplePwmChannel<PwmPolicy> custom(u8 channel, u32 frequency_hz) {
        return SimplePwmChannel<PwmPolicy>(channel, frequency_hz);
    }
};

}  // namespace alloy::hal
