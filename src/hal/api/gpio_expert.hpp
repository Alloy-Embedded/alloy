/**
 * @file gpio_expert.hpp
 * @brief Level 3 Expert API for GPIO
 *
 * Provides full control over GPIO configuration with all parameters exposed.
 * Expert API is for advanced use cases requiring precise control.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert GPIO configuration
 *
 * Provides complete control over all GPIO parameters.
 * Use this when you need fine-grained control over pin behavior.
 */
struct GpioExpertConfig {
    PinDirection direction;       ///< Pin direction (Input/Output)
    PinPull pull;                ///< Pull resistor configuration
    PinDrive drive;              ///< Output drive mode (PushPull/OpenDrain)
    bool active_high;            ///< true = active-high, false = active-low
    bool initial_state_on;       ///< Initial logical state (for outputs)

    // Advanced features (platform-dependent)
    uint8_t drive_strength;      ///< Drive strength (0-3, platform-specific)
    bool slew_rate_fast;         ///< Fast slew rate (reduces EMI if false)
    bool input_filter_enable;    ///< Enable input glitch filter
    uint8_t filter_clock_div;    ///< Filter clock divider (0-7)

    /**
     * @brief Validate configuration
     */
    constexpr bool is_valid() const {
        // Drive strength must be 0-3
        if (drive_strength > 3) {
            return false;
        }
        // Filter clock divider must be 0-7
        if (filter_clock_div > 7) {
            return false;
        }
        // Drive mode only valid for outputs
        if (direction == PinDirection::Input && drive != PinDrive::PushPull) {
            return false;
        }
        return true;
    }

    /**
     * @brief Get validation error message
     */
    constexpr const char* error_message() const {
        if (drive_strength > 3) {
            return "Drive strength must be 0-3";
        }
        if (filter_clock_div > 7) {
            return "Filter clock divider must be 0-7";
        }
        if (direction == PinDirection::Input && drive != PinDrive::PushPull) {
            return "Drive mode only valid for outputs";
        }
        return "Valid";
    }

    // ========================================================================
    // Factory Methods for Common Configurations
    // ========================================================================

    /**
     * @brief Standard output (push-pull, active-high)
     *
     * @param initial_on Initial state (true = ON/HIGH, false = OFF/LOW)
     * @return GpioExpertConfig for standard output
     */
    static constexpr GpioExpertConfig standard_output(bool initial_on = false) {
        return GpioExpertConfig{
            .direction = PinDirection::Output,
            .pull = PinPull::None,
            .drive = PinDrive::PushPull,
            .active_high = true,
            .initial_state_on = initial_on,
            .drive_strength = 2,      // Medium strength
            .slew_rate_fast = false,  // Reduced EMI
            .input_filter_enable = false,
            .filter_clock_div = 0
        };
    }

    /**
     * @brief LED output (push-pull, configurable polarity)
     *
     * @param active_low true if LED is active-low (common)
     * @param initial_on Initial LED state (true = ON)
     * @return GpioExpertConfig for LED
     */
    static constexpr GpioExpertConfig led(bool active_low = true, bool initial_on = false) {
        return GpioExpertConfig{
            .direction = PinDirection::Output,
            .pull = PinPull::None,
            .drive = PinDrive::PushPull,
            .active_high = !active_low,
            .initial_state_on = initial_on,
            .drive_strength = 1,      // Low strength (LED doesn't need much)
            .slew_rate_fast = false,
            .input_filter_enable = false,
            .filter_clock_div = 0
        };
    }

    /**
     * @brief Button input with pull-up (active-low, debounced)
     *
     * @return GpioExpertConfig for button
     */
    static constexpr GpioExpertConfig button_pullup() {
        return GpioExpertConfig{
            .direction = PinDirection::Input,
            .pull = PinPull::PullUp,
            .drive = PinDrive::PushPull,  // N/A for input
            .active_high = false,         // Button press = LOW
            .initial_state_on = false,
            .drive_strength = 0,
            .slew_rate_fast = false,
            .input_filter_enable = true,  // Debounce
            .filter_clock_div = 3         // ~10ms debounce
        };
    }

    /**
     * @brief Open-drain output (for I2C, 1-Wire, etc.)
     *
     * @param enable_pullup Enable internal pull-up
     * @return GpioExpertConfig for open-drain
     */
    static constexpr GpioExpertConfig open_drain(bool enable_pullup = true) {
        return GpioExpertConfig{
            .direction = PinDirection::Output,
            .pull = enable_pullup ? PinPull::PullUp : PinPull::None,
            .drive = PinDrive::OpenDrain,
            .active_high = true,
            .initial_state_on = false,  // Start HIGH (released)
            .drive_strength = 2,
            .slew_rate_fast = false,
            .input_filter_enable = false,
            .filter_clock_div = 0
        };
    }

    /**
     * @brief High-speed output (fast slew rate, high drive)
     *
     * Use for SPI, SDIO, or other high-speed protocols.
     *
     * @return GpioExpertConfig for high-speed output
     */
    static constexpr GpioExpertConfig high_speed_output() {
        return GpioExpertConfig{
            .direction = PinDirection::Output,
            .pull = PinPull::None,
            .drive = PinDrive::PushPull,
            .active_high = true,
            .initial_state_on = false,
            .drive_strength = 3,      // Maximum strength
            .slew_rate_fast = true,   // Fast edges
            .input_filter_enable = false,
            .filter_clock_div = 0
        };
    }

    /**
     * @brief Analog input (minimal digital interference)
     *
     * @return GpioExpertConfig for analog input
     */
    static constexpr GpioExpertConfig analog_input() {
        return GpioExpertConfig{
            .direction = PinDirection::Input,
            .pull = PinPull::None,           // No pull for analog
            .drive = PinDrive::PushPull,     // N/A
            .active_high = true,
            .initial_state_on = false,
            .drive_strength = 0,
            .slew_rate_fast = false,
            .input_filter_enable = false,    // No digital filter
            .filter_clock_div = 0
        };
    }

    /**
     * @brief Custom configuration
     *
     * @param dir Direction
     * @param pull_cfg Pull configuration
     * @param drive_mode Drive mode
     * @param active_hi Active-high if true
     * @return GpioExpertConfig with custom settings
     */
    static constexpr GpioExpertConfig custom(
        PinDirection dir,
        PinPull pull_cfg = PinPull::None,
        PinDrive drive_mode = PinDrive::PushPull,
        bool active_hi = true) {

        return GpioExpertConfig{
            .direction = dir,
            .pull = pull_cfg,
            .drive = drive_mode,
            .active_high = active_hi,
            .initial_state_on = false,
            .drive_strength = 2,
            .slew_rate_fast = false,
            .input_filter_enable = false,
            .filter_clock_div = 0
        };
    }
};

namespace expert {

/**
 * @brief Configure GPIO with expert settings
 *
 * Applies expert configuration to a GPIO pin.
 *
 * @tparam PinType GPIO pin type
 * @param config Expert configuration
 * @return Result with error code
 *
 * Example:
 * @code
 * GpioPin<peripherals::PIOC, 8> led_pin;
 * auto config = GpioExpertConfig::led(true, false);  // Active-low LED, initially OFF
 * auto result = expert::configure(led_pin, config);
 * @endcode
 */
template <typename PinType>
Result<void, ErrorCode> configure(PinType& pin, const GpioExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    // Apply direction
    auto dir_result = pin.setDirection(config.direction);
    if (!dir_result.is_ok()) {
        return dir_result;
    }

    // Apply pull configuration
    auto pull_result = pin.setPull(config.pull);
    if (!pull_result.is_ok()) {
        return pull_result;
    }

    // Apply drive mode (for outputs)
    if (config.direction == PinDirection::Output) {
        auto drive_result = pin.setDrive(config.drive);
        if (!drive_result.is_ok()) {
            return drive_result;
        }

        // Set initial state
        bool physical_state = config.active_high ? config.initial_state_on : !config.initial_state_on;
        if (physical_state) {
            auto set_result = pin.set();
            if (!set_result.is_ok()) {
                return set_result;
            }
        } else {
            auto clear_result = pin.clear();
            if (!clear_result.is_ok()) {
                return clear_result;
            }
        }
    }

    // TODO: Apply advanced features (drive_strength, slew_rate, filter)
    // These are platform-specific and would require extension to the platform layer

    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
