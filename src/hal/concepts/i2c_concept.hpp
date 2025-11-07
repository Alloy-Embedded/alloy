/**
 * @file i2c_concept.hpp
 * @brief I2C interface concept for compile-time validation (zero runtime overhead)
 *
 * This file defines the compile-time interface requirements for I2C peripherals.
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

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

namespace alloy::hal::concepts {

// Import types into this namespace for convenience
using alloy::core::Result;
using alloy::core::ErrorCode;
using alloy::hal::I2cConfig;
using alloy::hal::I2cSpeed;

#if __cplusplus >= 202002L
// ============================================================================
// C++20 Implementation - Using Concepts
// ============================================================================

/**
 * @brief I2C concept - compile-time interface validation
 *
 * Defines the interface contract that all I2C implementations must satisfy.
 * Validated at compile-time with ZERO runtime overhead.
 *
 * Required methods:
 * - open() -> Result<void>
 * - close() -> Result<void>
 * - configure(const I2cConfig&) -> Result<void>
 * - write(uint8_t address, const uint8_t* data, size_t size) -> Result<size_t>
 * - read(uint8_t address, uint8_t* data, size_t size) -> Result<size_t>
 * - writeRead(uint8_t address, const uint8_t* tx, size_t tx_size, uint8_t* rx, size_t rx_size) -> Result<size_t>
 * - setSpeed(I2cSpeed) -> Result<void>
 * - isOpen() -> bool
 *
 * @tparam T The type to validate against I2C concept
 *
 * Example usage:
 * @code
 * template <I2cConcept TI2c>
 * Result<uint8_t> read_sensor(TI2c& i2c, uint8_t device_addr, uint8_t reg_addr) {
 *     uint8_t value;
 *     if (auto result = i2c.writeRead(device_addr, &reg_addr, 1, &value, 1); result.is_error()) {
 *         return Result<uint8_t>::error(result.error());
 *     }
 *     return Result<uint8_t>::ok(value);
 * }
 * @endcode
 */
template <typename T>
concept I2cConcept = requires(
    T i2c,
    const T const_i2c,
    const I2cConfig& config,
    uint8_t address,
    const uint8_t* const_data,
    uint8_t* data,
    size_t size,
    I2cSpeed speed
) {
    // Open/close operations
    { i2c.open() } -> std::same_as<Result<void>>;
    { i2c.close() } -> std::same_as<Result<void>>;

    // Configuration
    { i2c.configure(config) } -> std::same_as<Result<void>>;
    { i2c.setSpeed(speed) } -> std::same_as<Result<void>>;

    // Data transfer operations
    { i2c.write(address, const_data, size) } -> std::same_as<Result<size_t>>;
    { i2c.read(address, data, size) } -> std::same_as<Result<size_t>>;
    { i2c.writeRead(address, const_data, size, data, size) } -> std::same_as<Result<size_t>>;

    // State query
    { const_i2c.isOpen() } -> std::same_as<bool>;
};

#else
// ============================================================================
// C++17 Fallback - Using SFINAE + static_assert
// ============================================================================

namespace detail {

// Helper to check if T has open() -> Result<void>
template <typename T, typename = void>
struct has_i2c_open : std::false_type {};

template <typename T>
struct has_i2c_open<T, std::void_t<
    decltype(std::declval<T&>().open())
>> : std::is_same<
    decltype(std::declval<T&>().open()),
    Result<void>
> {};

// Helper to check if T has close() -> Result<void>
template <typename T, typename = void>
struct has_i2c_close : std::false_type {};

template <typename T>
struct has_i2c_close<T, std::void_t<
    decltype(std::declval<T&>().close())
>> : std::is_same<
    decltype(std::declval<T&>().close()),
    Result<void>
> {};

// Helper to check if T has configure(const I2cConfig&) -> Result<void>
template <typename T, typename = void>
struct has_i2c_configure : std::false_type {};

template <typename T>
struct has_i2c_configure<T, std::void_t<
    decltype(std::declval<T&>().configure(std::declval<const I2cConfig&>()))
>> : std::is_same<
    decltype(std::declval<T&>().configure(std::declval<const I2cConfig&>())),
    Result<void>
> {};

// Helper to check if T has write(uint8_t, const uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_i2c_write : std::false_type {};

template <typename T>
struct has_i2c_write<T, std::void_t<
    decltype(std::declval<T&>().write(
        std::declval<uint8_t>(),
        std::declval<const uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().write(
        std::declval<uint8_t>(),
        std::declval<const uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has read(uint8_t, uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_i2c_read : std::false_type {};

template <typename T>
struct has_i2c_read<T, std::void_t<
    decltype(std::declval<T&>().read(
        std::declval<uint8_t>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().read(
        std::declval<uint8_t>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has writeRead(...) -> Result<size_t>
template <typename T, typename = void>
struct has_i2c_write_read : std::false_type {};

template <typename T>
struct has_i2c_write_read<T, std::void_t<
    decltype(std::declval<T&>().writeRead(
        std::declval<uint8_t>(),
        std::declval<const uint8_t*>(),
        std::declval<size_t>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().writeRead(
        std::declval<uint8_t>(),
        std::declval<const uint8_t*>(),
        std::declval<size_t>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has setSpeed(I2cSpeed) -> Result<void>
template <typename T, typename = void>
struct has_i2c_set_speed : std::false_type {};

template <typename T>
struct has_i2c_set_speed<T, std::void_t<
    decltype(std::declval<T&>().setSpeed(std::declval<I2cSpeed>()))
>> : std::is_same<
    decltype(std::declval<T&>().setSpeed(std::declval<I2cSpeed>())),
    Result<void>
> {};

// Helper to check if T has isOpen() const -> bool
template <typename T, typename = void>
struct has_i2c_is_open : std::false_type {};

template <typename T>
struct has_i2c_is_open<T, std::void_t<
    decltype(std::declval<const T&>().isOpen())
>> : std::is_same<
    decltype(std::declval<const T&>().isOpen()),
    bool
> {};

} // namespace detail

/**
 * @brief Type trait to check if T satisfies I2cConcept (C++17 version)
 *
 * Usage with static_assert:
 * @code
 * template <typename TI2c>
 * Result<uint8_t> read_sensor(TI2c& i2c, uint8_t device_addr, uint8_t reg_addr) {
 *     static_assert(is_i2c_v<TI2c>, "TI2c must satisfy I2cConcept");
 *     // ... implementation
 * }
 * @endcode
 */
template <typename T>
struct is_i2c : std::conjunction<
    detail::has_i2c_open<T>,
    detail::has_i2c_close<T>,
    detail::has_i2c_configure<T>,
    detail::has_i2c_write<T>,
    detail::has_i2c_read<T>,
    detail::has_i2c_write_read<T>,
    detail::has_i2c_set_speed<T>,
    detail::has_i2c_is_open<T>
> {};

/// @brief Helper variable template for is_i2c
template <typename T>
inline constexpr bool is_i2c_v = is_i2c<T>::value;

#endif // __cplusplus >= 202002L

} // namespace alloy::hal::concepts
