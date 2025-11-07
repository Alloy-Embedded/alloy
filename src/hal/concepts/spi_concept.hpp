/**
 * @file spi_concept.hpp
 * @brief SPI interface concept for compile-time validation (zero runtime overhead)
 *
 * This file defines the compile-time interface requirements for SPI peripherals.
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
using alloy::hal::SpiConfig;
using alloy::hal::SpiMode;

#if __cplusplus >= 202002L
// ============================================================================
// C++20 Implementation - Using Concepts
// ============================================================================

/**
 * @brief SPI concept - compile-time interface validation
 *
 * Defines the interface contract that all SPI implementations must satisfy.
 * Validated at compile-time with ZERO runtime overhead.
 *
 * Required methods:
 * - open() -> Result<void>
 * - close() -> Result<void>
 * - configure(const SpiConfig&) -> Result<void>
 * - transfer(const uint8_t* tx_data, uint8_t* rx_data, size_t size) -> Result<size_t>
 * - write(const uint8_t* data, size_t size) -> Result<size_t>
 * - read(uint8_t* data, size_t size) -> Result<size_t>
 * - setClockSpeed(uint32_t) -> Result<void>
 * - setMode(SpiMode) -> Result<void>
 * - chipSelect(bool) -> Result<void>
 * - isOpen() -> bool
 *
 * @tparam T The type to validate against SPI concept
 *
 * Example usage:
 * @code
 * template <SpiConcept TSpi>
 * Result<uint8_t> read_register(TSpi& spi, uint8_t reg_addr) {
 *     uint8_t tx[2] = {0x80 | reg_addr, 0x00}; // Read command
 *     uint8_t rx[2];
 *
 *     spi.chipSelect(true);
 *     auto result = spi.transfer(tx, rx, 2);
 *     spi.chipSelect(false);
 *
 *     if (result.is_error()) {
 *         return Result<uint8_t>::error(result.error());
 *     }
 *     return Result<uint8_t>::ok(rx[1]);
 * }
 * @endcode
 */
template <typename T>
concept SpiConcept = requires(
    T spi,
    const T const_spi,
    const SpiConfig& config,
    const uint8_t* const_data,
    uint8_t* data,
    size_t size,
    uint32_t clock_speed,
    SpiMode mode,
    bool cs_state
) {
    // Open/close operations
    { spi.open() } -> std::same_as<Result<void>>;
    { spi.close() } -> std::same_as<Result<void>>;

    // Configuration
    { spi.configure(config) } -> std::same_as<Result<void>>;
    { spi.setClockSpeed(clock_speed) } -> std::same_as<Result<void>>;
    { spi.setMode(mode) } -> std::same_as<Result<void>>;

    // Data transfer operations
    { spi.transfer(const_data, data, size) } -> std::same_as<Result<size_t>>;
    { spi.write(const_data, size) } -> std::same_as<Result<size_t>>;
    { spi.read(data, size) } -> std::same_as<Result<size_t>>;

    // Chip select control
    { spi.chipSelect(cs_state) } -> std::same_as<Result<void>>;

    // State query
    { const_spi.isOpen() } -> std::same_as<bool>;
};

#else
// ============================================================================
// C++17 Fallback - Using SFINAE + static_assert
// ============================================================================

namespace detail {

// Helper to check if T has open() -> Result<void>
template <typename T, typename = void>
struct has_spi_open : std::false_type {};

template <typename T>
struct has_spi_open<T, std::void_t<
    decltype(std::declval<T&>().open())
>> : std::is_same<
    decltype(std::declval<T&>().open()),
    Result<void>
> {};

// Helper to check if T has close() -> Result<void>
template <typename T, typename = void>
struct has_spi_close : std::false_type {};

template <typename T>
struct has_spi_close<T, std::void_t<
    decltype(std::declval<T&>().close())
>> : std::is_same<
    decltype(std::declval<T&>().close()),
    Result<void>
> {};

// Helper to check if T has configure(const SpiConfig&) -> Result<void>
template <typename T, typename = void>
struct has_spi_configure : std::false_type {};

template <typename T>
struct has_spi_configure<T, std::void_t<
    decltype(std::declval<T&>().configure(std::declval<const SpiConfig&>()))
>> : std::is_same<
    decltype(std::declval<T&>().configure(std::declval<const SpiConfig&>())),
    Result<void>
> {};

// Helper to check if T has transfer(const uint8_t*, uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_spi_transfer : std::false_type {};

template <typename T>
struct has_spi_transfer<T, std::void_t<
    decltype(std::declval<T&>().transfer(
        std::declval<const uint8_t*>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().transfer(
        std::declval<const uint8_t*>(),
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has write(const uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_spi_write : std::false_type {};

template <typename T>
struct has_spi_write<T, std::void_t<
    decltype(std::declval<T&>().write(
        std::declval<const uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().write(
        std::declval<const uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has read(uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_spi_read : std::false_type {};

template <typename T>
struct has_spi_read<T, std::void_t<
    decltype(std::declval<T&>().read(
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    ))
>> : std::is_same<
    decltype(std::declval<T&>().read(
        std::declval<uint8_t*>(),
        std::declval<size_t>()
    )),
    Result<size_t>
> {};

// Helper to check if T has setClockSpeed(uint32_t) -> Result<void>
template <typename T, typename = void>
struct has_spi_set_clock_speed : std::false_type {};

template <typename T>
struct has_spi_set_clock_speed<T, std::void_t<
    decltype(std::declval<T&>().setClockSpeed(std::declval<uint32_t>()))
>> : std::is_same<
    decltype(std::declval<T&>().setClockSpeed(std::declval<uint32_t>())),
    Result<void>
> {};

// Helper to check if T has setMode(SpiMode) -> Result<void>
template <typename T, typename = void>
struct has_spi_set_mode : std::false_type {};

template <typename T>
struct has_spi_set_mode<T, std::void_t<
    decltype(std::declval<T&>().setMode(std::declval<SpiMode>()))
>> : std::is_same<
    decltype(std::declval<T&>().setMode(std::declval<SpiMode>())),
    Result<void>
> {};

// Helper to check if T has chipSelect(bool) -> Result<void>
template <typename T, typename = void>
struct has_spi_chip_select : std::false_type {};

template <typename T>
struct has_spi_chip_select<T, std::void_t<
    decltype(std::declval<T&>().chipSelect(std::declval<bool>()))
>> : std::is_same<
    decltype(std::declval<T&>().chipSelect(std::declval<bool>())),
    Result<void>
> {};

// Helper to check if T has isOpen() const -> bool
template <typename T, typename = void>
struct has_spi_is_open : std::false_type {};

template <typename T>
struct has_spi_is_open<T, std::void_t<
    decltype(std::declval<const T&>().isOpen())
>> : std::is_same<
    decltype(std::declval<const T&>().isOpen()),
    bool
> {};

} // namespace detail

/**
 * @brief Type trait to check if T satisfies SpiConcept (C++17 version)
 *
 * Usage with static_assert:
 * @code
 * template <typename TSpi>
 * Result<uint8_t> read_register(TSpi& spi, uint8_t reg_addr) {
 *     static_assert(is_spi_v<TSpi>, "TSpi must satisfy SpiConcept");
 *     // ... implementation
 * }
 * @endcode
 */
template <typename T>
struct is_spi : std::conjunction<
    detail::has_spi_open<T>,
    detail::has_spi_close<T>,
    detail::has_spi_configure<T>,
    detail::has_spi_transfer<T>,
    detail::has_spi_write<T>,
    detail::has_spi_read<T>,
    detail::has_spi_set_clock_speed<T>,
    detail::has_spi_set_mode<T>,
    detail::has_spi_chip_select<T>,
    detail::has_spi_is_open<T>
> {};

/// @brief Helper variable template for is_spi
template <typename T>
inline constexpr bool is_spi_v = is_spi<T>::value;

#endif // __cplusplus >= 202002L

} // namespace alloy::hal::concepts
