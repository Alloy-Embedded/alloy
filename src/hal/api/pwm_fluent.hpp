/**
 * @file pwm_fluent.hpp
 * @brief Level 2 Fluent API for PWM
 *
 * Provides chainable builder pattern for readable PWM configuration.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/pwm_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Builder state tracking
 */
struct PwmBuilderState {
    bool has_frequency = false;
    bool has_channel = false;
    bool has_duty = false;

    constexpr bool is_valid() const {
        return has_frequency && has_channel;
    }
};

/**
 * @brief Fluent PWM configuration result
 */
template <typename PwmPolicy>
struct FluentPwmConfig {
    SimplePwmChannel<PwmPolicy> channel;

    Result<void, ErrorCode> start() { return channel.start(); }
    Result<void, ErrorCode> stop() { return channel.stop(); }
    Result<void, ErrorCode> set_duty(float percent) { return channel.set_duty(percent); }
};

/**
 * @brief Fluent PWM configuration builder
 *
 * Example:
 * @code
 * auto led = PwmBuilder<Pwm0Hardware>()
 *     .channel(0)
 *     .frequency_hz(1000)
 *     .duty_percent(50.0f)
 *     .initialize();
 * led.value().start();
 * @endcode
 */
template <typename PwmPolicy>
class PwmBuilder {
public:
    constexpr PwmBuilder() : frequency_hz_(1000), channel_(0), duty_percent_(0.0f), state_() {}

    constexpr PwmBuilder& channel(u8 ch) {
        channel_ = ch;
        state_.has_channel = true;
        return *this;
    }

    constexpr PwmBuilder& frequency_hz(u32 freq) {
        frequency_hz_ = freq;
        state_.has_frequency = true;
        return *this;
    }

    constexpr PwmBuilder& duty_percent(float duty) {
        duty_percent_ = duty;
        state_.has_duty = true;
        return *this;
    }

    constexpr PwmBuilder& for_led_dimming() {
        frequency_hz_ = PwmPresets::LED_DIMMING_HZ;
        state_.has_frequency = true;
        return *this;
    }

    constexpr PwmBuilder& for_servo() {
        frequency_hz_ = PwmPresets::SERVO_MOTOR_HZ;
        state_.has_frequency = true;
        return *this;
    }

    constexpr PwmBuilder& for_dc_motor() {
        frequency_hz_ = PwmPresets::DC_MOTOR_HZ;
        state_.has_frequency = true;
        return *this;
    }

    Result<FluentPwmConfig<PwmPolicy>, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        SimplePwmChannel<PwmPolicy> ch(channel_, frequency_hz_);

        if (state_.has_duty) {
            auto result = ch.set_duty(duty_percent_);
            if (!result.is_ok()) {
                return Err(result.error());
            }
        }

        return Ok(FluentPwmConfig<PwmPolicy>{std::move(ch)});
    }

private:
    u32 frequency_hz_;
    u8 channel_;
    float duty_percent_;
    PwmBuilderState state_;
};

}  // namespace alloy::hal
