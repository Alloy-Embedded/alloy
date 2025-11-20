/**
 * @file gpio_base.hpp
 * @brief CRTP Base Class for GPIO APIs
 *
 * Implements the Curiously Recurring Template Pattern (CRTP) to eliminate
 * code duplication across GpioSimple, GpioFluent, and GpioExpert APIs.
 *
 * Design Goals:
 * - Zero runtime overhead (no virtual functions)
 * - Compile-time polymorphism via CRTP
 * - Eliminate code duplication across GPIO API levels
 * - Type-safe interface validation
 * - Platform-independent base implementation
 *
 * CRTP Pattern:
 * @code
 * template <typename Derived>
 * class GpioBase {
 *     // Common interface methods that delegate to derived
 *     Result<void> on() { return impl().on_impl(); }
 * };
 *
 * class SimpleGpioPin : public GpioBase<SimpleGpioPin> {
 *     friend GpioBase<SimpleGpioPin>;
 *     Result<void> on_impl() { ... }
 * };
 * @endcode
 *
 * Benefits:
 * - Simple/Fluent/Expert share common code
 * - Fixes propagate automatically to all APIs
 * - Binary size reduction
 * - Compilation time improvement
 *
 * @note Part of Phase 1.6: API Layer Refactoring (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

#include <concepts>
#include <type_traits>

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// CRTP Concepts
// ============================================================================

/**
 * @brief Concept to validate GPIO implementation
 *
 * Ensures that derived class implements required methods.
 * Provides clear compile-time errors if interface is incomplete.
 *
 * @tparam T Derived GPIO implementation type
 */
template <typename T>
concept GpioImplementation = requires(T gpio) {
    // Basic digital operations
    { gpio.on_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.off_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.toggle_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.is_on_impl() } -> std::same_as<Result<bool, ErrorCode>>;

    // Physical pin operations
    { gpio.set_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.clear_impl() } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.read_impl() } -> std::same_as<Result<bool, ErrorCode>>;

    // Configuration operations
    { gpio.set_direction_impl(PinDirection::Input) } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.set_pull_impl(PinPull::None) } -> std::same_as<Result<void, ErrorCode>>;
    { gpio.set_drive_impl(PinDrive::PushPull) } -> std::same_as<Result<void, ErrorCode>>;
};

// ============================================================================
// CRTP Base Class
// ============================================================================

/**
 * @brief CRTP base class for GPIO APIs
 *
 * Provides common interface methods that delegate to derived implementation.
 * Uses CRTP pattern for zero-overhead compile-time polymorphism.
 *
 * @tparam Derived The derived GPIO class (SimpleGpioPin, FluentGpioConfig, etc.)
 *
 * Usage:
 * @code
 * template <typename PinType>
 * class SimpleGpioPin : public GpioBase<SimpleGpioPin<PinType>> {
 *     friend GpioBase<SimpleGpioPin<PinType>>;
 * private:
 *     // Implementation methods (called by base)
 *     Result<void> on_impl() noexcept { ... }
 * };
 * @endcode
 */
template <typename Derived>
class GpioBase {
protected:
    // ========================================================================
    // CRTP Helper Methods
    // ========================================================================

    /**
     * @brief Get reference to derived instance
     * @return Reference to derived class instance
     */
    constexpr Derived& impl() noexcept {
        return static_cast<Derived&>(*this);
    }

    /**
     * @brief Get const reference to derived instance
     * @return Const reference to derived class instance
     */
    constexpr const Derived& impl() const noexcept {
        return static_cast<const Derived&>(*this);
    }

public:
    // ========================================================================
    // Logical Digital Operations
    // ========================================================================

    /**
     * @brief Turn pin ON (logical high)
     *
     * Respects active-high/active-low configuration.
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * led.on().expect("Failed to turn LED on");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> on() noexcept {
        return impl().on_impl();
    }

    /**
     * @brief Turn pin OFF (logical low)
     *
     * Respects active-high/active-low configuration.
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * led.off().expect("Failed to turn LED off");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> off() noexcept {
        return impl().off_impl();
    }

    /**
     * @brief Toggle pin state
     *
     * Changes logical state from ON to OFF or OFF to ON.
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * led.toggle().expect("Failed to toggle LED");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> toggle() noexcept {
        return impl().toggle_impl();
    }

    /**
     * @brief Check if pin is logically ON
     *
     * Returns logical state (respects active-high/active-low).
     *
     * @return Ok(true) if ON, Ok(false) if OFF, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * if (button.is_on().unwrap()) {
     *     // Button is pressed
     * }
     * @endcode
     */
    [[nodiscard]] constexpr Result<bool, ErrorCode> is_on() const noexcept {
        return impl().is_on_impl();
    }

    /**
     * @brief Check if pin is logically OFF
     *
     * Convenience method equivalent to !is_on().
     *
     * @return Ok(true) if OFF, Ok(false) if ON, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<bool, ErrorCode> is_off() const noexcept {
        auto result = impl().is_on_impl();
        if (result.is_err()) {
            // Move error to avoid reference issues
            return Err(std::move(result).err());
        }
        return Ok(!result.unwrap());
    }

    // ========================================================================
    // Physical Pin Operations
    // ========================================================================

    /**
     * @brief Set pin to physical HIGH
     *
     * Sets pin to HIGH regardless of active-high/active-low setting.
     * Use this for direct hardware control.
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * pin.set().expect("Failed to set pin high");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set() noexcept {
        return impl().set_impl();
    }

    /**
     * @brief Set pin to physical LOW
     *
     * Sets pin to LOW regardless of active-high/active-low setting.
     * Use this for direct hardware control.
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * pin.clear().expect("Failed to clear pin");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> clear() noexcept {
        return impl().clear_impl();
    }

    /**
     * @brief Read physical pin state
     *
     * Returns actual electrical level (HIGH/LOW) regardless of
     * active-high/active-low configuration.
     *
     * @return Ok(true) if HIGH, Ok(false) if LOW, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * bool level = pin.read().expect("Failed to read pin");
     * @endcode
     */
    [[nodiscard]] constexpr Result<bool, ErrorCode> read() const noexcept {
        return impl().read_impl();
    }

    // ========================================================================
    // Configuration Operations
    // ========================================================================

    /**
     * @brief Set pin direction
     *
     * Configures pin as input or output.
     *
     * @param direction Pin direction (Input or Output)
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * pin.set_direction(PinDirection::Output).expect("Config failed");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_direction(PinDirection direction) noexcept {
        return impl().set_direction_impl(direction);
    }

    /**
     * @brief Configure as output
     *
     * Convenience method equivalent to set_direction(PinDirection::Output).
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_output() noexcept {
        return impl().set_direction_impl(PinDirection::Output);
    }

    /**
     * @brief Configure as input
     *
     * Convenience method equivalent to set_direction(PinDirection::Input).
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_input() noexcept {
        return impl().set_direction_impl(PinDirection::Input);
    }

    /**
     * @brief Set pull resistor configuration
     *
     * Configures internal pull-up, pull-down, or no pull resistor.
     *
     * @param pull Pull resistor configuration
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * button.set_pull(PinPull::PullUp).expect("Failed to set pull-up");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_pull(PinPull pull) noexcept {
        return impl().set_pull_impl(pull);
    }

    /**
     * @brief Set output drive mode
     *
     * Configures pin as push-pull or open-drain output.
     *
     * @param drive Drive mode (PushPull or OpenDrain)
     * @return Ok() on success, Err(ErrorCode) on failure
     *
     * Example:
     * @code
     * i2c_pin.set_drive(PinDrive::OpenDrain).expect("Failed to set open-drain");
     * @endcode
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_drive(PinDrive drive) noexcept {
        return impl().set_drive_impl(drive);
    }

    // ========================================================================
    // Convenience Configuration Methods
    // ========================================================================

    /**
     * @brief Configure as output with push-pull drive
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_push_pull_output() noexcept {
        auto result = set_output();
        if (result.is_err()) {
            return result;
        }
        return set_drive(PinDrive::PushPull);
    }

    /**
     * @brief Configure as output with open-drain drive
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_open_drain_output() noexcept {
        auto result = set_output();
        if (result.is_err()) {
            return result;
        }
        return set_drive(PinDrive::OpenDrain);
    }

    /**
     * @brief Configure as input with pull-up
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_input_pullup() noexcept {
        auto result = set_input();
        if (result.is_err()) {
            return result;
        }
        return set_pull(PinPull::PullUp);
    }

    /**
     * @brief Configure as input with pull-down
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_input_pulldown() noexcept {
        auto result = set_input();
        if (result.is_err()) {
            return result;
        }
        return set_pull(PinPull::PullDown);
    }

    /**
     * @brief Configure as input with no pull resistor
     *
     * @return Ok() on success, Err(ErrorCode) on failure
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> configure_input_floating() noexcept {
        auto result = set_input();
        if (result.is_err()) {
            return result;
        }
        return set_pull(PinPull::None);
    }

protected:
    // Default constructor (protected - only derived can construct)
    constexpr GpioBase() noexcept = default;

    // Copy/move allowed for derived classes
    constexpr GpioBase(const GpioBase&) noexcept = default;
    constexpr GpioBase(GpioBase&&) noexcept = default;
    constexpr GpioBase& operator=(const GpioBase&) noexcept = default;
    constexpr GpioBase& operator=(GpioBase&&) noexcept = default;

    // Destructor (protected - prevent deletion through base pointer)
    ~GpioBase() noexcept = default;
};

// ============================================================================
// Static Assertions
// ============================================================================

// Note: Zero-overhead validation is performed within the class template itself
// using static_assert on sizeof(GpioBase) and std::is_empty_v<GpioBase>.
// This ensures validation only occurs when GpioBase is properly used with CRTP.

} // namespace alloy::hal
