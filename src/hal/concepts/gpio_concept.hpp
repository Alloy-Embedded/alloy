/**
 * @file gpio_concept.hpp
 * @brief GPIO interface concept for compile-time validation (zero runtime overhead)
 *
 * This file defines the compile-time interface requirements for GPIO pins.
 * Uses C++20 concepts when available, falls back to C++17 static_assert.
 *
 * Critical Design Decision:
 * - ZERO virtual functions - All validation is compile-time
 * - Templates only - No vtable overhead
 * - Full inlining - Compiler can optimize away all abstraction
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
 */

#pragma once

#include <type_traits>
#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

namespace alloy::hal::concepts {

// Import types into this namespace for convenience
using alloy::core::Result;
using alloy::core::ErrorCode;
using alloy::hal::PinDirection;
using alloy::hal::PinState;
using alloy::hal::PinPull;
using alloy::hal::PinDrive;
using alloy::hal::GpioConfig;

#if __cplusplus >= 202002L
// ============================================================================
// C++20 Implementation - Using Concepts
// ============================================================================

/**
 * @brief GPIO concept - compile-time interface validation
 *
 * Defines the interface contract that all GPIO implementations must satisfy.
 * Validated at compile-time with ZERO runtime overhead.
 *
 * Required methods:
 * - setDirection(PinDirection) -> Result<void>
 * - getDirection() -> Result<PinDirection>
 * - write(PinState) -> Result<void>
 * - read() -> Result<PinState>
 * - toggle() -> Result<void>
 * - setPull(PinPull) -> Result<void>
 * - setDrive(PinDrive) -> Result<void>
 * - configure(const GpioConfig&) -> Result<void>
 *
 * @tparam T The type to validate against GPIO concept
 *
 * Example usage:
 * @code
 * template <GpioConcept TGpio>
 * void blink_led(TGpio& led, uint32_t delay_ms) {
 *     led.setDirection(PinDirection::Output);
 *     led.setDrive(PinDrive::PushPull);
 *     while (true) {
 *         led.toggle();
 *         delay(delay_ms);
 *     }
 * }
 * @endcode
 */
template <typename T>
concept GpioConcept = requires(
    T gpio,
    const T const_gpio,
    PinDirection direction,
    PinState state,
    PinPull pull,
    PinDrive drive,
    const GpioConfig& config
) {
    // Direction configuration
    { gpio.setDirection(direction) } -> std::same_as<Result<void>>;
    { const_gpio.getDirection() } -> std::same_as<Result<PinDirection>>;

    // Output operations
    { gpio.write(state) } -> std::same_as<Result<void>>;
    { gpio.toggle() } -> std::same_as<Result<void>>;

    // Input operations
    { const_gpio.read() } -> std::same_as<Result<PinState>>;

    // Pin configuration
    { gpio.setPull(pull) } -> std::same_as<Result<void>>;
    { gpio.setDrive(drive) } -> std::same_as<Result<void>>;
    { gpio.configure(config) } -> std::same_as<Result<void>>;
};

#else
// ============================================================================
// C++17 Fallback - Using SFINAE + static_assert
// ============================================================================

namespace detail {

// Helper to check if T has setDirection(PinDirection) -> Result<void>
template <typename T, typename = void>
struct has_set_direction : std::false_type {};

template <typename T>
struct has_set_direction<T, std::void_t<
    decltype(std::declval<T&>().setDirection(std::declval<PinDirection>()))
>> : std::is_same<
    decltype(std::declval<T&>().setDirection(std::declval<PinDirection>())),
    Result<void>
> {};

// Helper to check if T has getDirection() const -> Result<PinDirection>
template <typename T, typename = void>
struct has_get_direction : std::false_type {};

template <typename T>
struct has_get_direction<T, std::void_t<
    decltype(std::declval<const T&>().getDirection())
>> : std::is_same<
    decltype(std::declval<const T&>().getDirection()),
    Result<PinDirection>
> {};

// Helper to check if T has write(PinState) -> Result<void>
template <typename T, typename = void>
struct has_write : std::false_type {};

template <typename T>
struct has_write<T, std::void_t<
    decltype(std::declval<T&>().write(std::declval<PinState>()))
>> : std::is_same<
    decltype(std::declval<T&>().write(std::declval<PinState>())),
    Result<void>
> {};

// Helper to check if T has read() const -> Result<PinState>
template <typename T, typename = void>
struct has_read : std::false_type {};

template <typename T>
struct has_read<T, std::void_t<
    decltype(std::declval<const T&>().read())
>> : std::is_same<
    decltype(std::declval<const T&>().read()),
    Result<PinState>
> {};

// Helper to check if T has toggle() -> Result<void>
template <typename T, typename = void>
struct has_toggle : std::false_type {};

template <typename T>
struct has_toggle<T, std::void_t<
    decltype(std::declval<T&>().toggle())
>> : std::is_same<
    decltype(std::declval<T&>().toggle()),
    Result<void>
> {};

// Helper to check if T has setPull(PinPull) -> Result<void>
template <typename T, typename = void>
struct has_set_pull : std::false_type {};

template <typename T>
struct has_set_pull<T, std::void_t<
    decltype(std::declval<T&>().setPull(std::declval<PinPull>()))
>> : std::is_same<
    decltype(std::declval<T&>().setPull(std::declval<PinPull>())),
    Result<void>
> {};

// Helper to check if T has setDrive(PinDrive) -> Result<void>
template <typename T, typename = void>
struct has_set_drive : std::false_type {};

template <typename T>
struct has_set_drive<T, std::void_t<
    decltype(std::declval<T&>().setDrive(std::declval<PinDrive>()))
>> : std::is_same<
    decltype(std::declval<T&>().setDrive(std::declval<PinDrive>())),
    Result<void>
> {};

// Helper to check if T has configure(const GpioConfig&) -> Result<void>
template <typename T, typename = void>
struct has_configure : std::false_type {};

template <typename T>
struct has_configure<T, std::void_t<
    decltype(std::declval<T&>().configure(std::declval<const GpioConfig&>()))
>> : std::is_same<
    decltype(std::declval<T&>().configure(std::declval<const GpioConfig&>())),
    Result<void>
> {};

} // namespace detail

/**
 * @brief Type trait to check if T satisfies GpioConcept (C++17 version)
 *
 * Usage with static_assert:
 * @code
 * template <typename TGpio>
 * void blink_led(TGpio& led, uint32_t delay_ms) {
 *     static_assert(is_gpio_v<TGpio>, "TGpio must satisfy GpioConcept");
 *     // ... implementation
 * }
 * @endcode
 */
template <typename T>
struct is_gpio : std::conjunction<
    detail::has_set_direction<T>,
    detail::has_get_direction<T>,
    detail::has_write<T>,
    detail::has_read<T>,
    detail::has_toggle<T>,
    detail::has_set_pull<T>,
    detail::has_set_drive<T>,
    detail::has_configure<T>
> {};

/// @brief Helper variable template for is_gpio
template <typename T>
inline constexpr bool is_gpio_v = is_gpio<T>::value;

#endif // __cplusplus >= 202002L

} // namespace alloy::hal::concepts

// ============================================================================
// Usage Examples (Documentation)
// ============================================================================

#if 0 // Documentation only, not compiled

// Example 1: Generic function using GPIO concept
#if __cplusplus >= 202002L
template <alloy::hal::concepts::GpioConcept TGpio>
#else
template <typename TGpio>
#endif
void blink_led(TGpio& led, uint32_t times, uint32_t delay_ms) {
#if __cplusplus < 202002L
    static_assert(alloy::hal::concepts::is_gpio_v<TGpio>,
                  "TGpio must satisfy GpioConcept");
#endif

    using namespace alloy::hal;

    // Configure LED as output
    GpioConfig config {
        .direction = PinDirection::Output,
        .drive = PinDrive::PushPull,
        .pull = PinPull::None,
        .initial_state = PinState::Low
    };

    if (auto result = led.configure(config); result.is_error()) {
        // Handle error
        return;
    }

    // Blink the LED
    for (uint32_t i = 0; i < times; ++i) {
        led.toggle();
        delay(delay_ms);
        led.toggle();
        delay(delay_ms);
    }
}

// Example 2: Platform-specific GPIO implementation
namespace alloy::hal::same70 {

template <uint32_t PORT_BASE, uint8_t PIN_NUMBER>
class GpioPin {
public:
    // This class satisfies GpioConcept at compile-time
    Result<void> setDirection(PinDirection direction);
    Result<PinDirection> getDirection() const;
    Result<void> write(PinState state);
    Result<PinState> read() const;
    Result<void> toggle();
    Result<void> setPull(PinPull pull);
    Result<void> setDrive(PinDrive drive);
    Result<void> configure(const GpioConfig& config);

private:
    PinDirection m_direction = PinDirection::Input;
    PinState m_state = PinState::Low;
};

// Type alias for specific GPIO pin
using LedGreen = GpioPin<PIOC_BASE, 8>;

} // namespace alloy::hal::same70

// Example 3: Compile-time validation
void example_usage() {
    using namespace alloy::hal::same70;

    auto led = LedGreen{};

    // This works because LedGreen satisfies GpioConcept
    blink_led(led, 10, 500);

    // This would fail at compile-time:
    // int not_a_gpio = 42;
    // blink_led(not_a_gpio, 10, 500); // ERROR: does not satisfy GpioConcept
}

#endif // Documentation examples
