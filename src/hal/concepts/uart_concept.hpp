/**
 * @file uart_concept.hpp
 * @brief UART interface concept for compile-time validation (zero runtime overhead)
 *
 * This file defines the compile-time interface requirements for UART peripherals.
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

// Import Result and ErrorCode into this namespace for convenience
using alloy::core::Result;
using alloy::core::ErrorCode;
using alloy::hal::Baudrate;

#if __cplusplus >= 202002L
// ============================================================================
// C++20 Implementation - Using Concepts
// ============================================================================

/**
 * @brief UART concept - compile-time interface validation
 *
 * Defines the interface contract that all UART implementations must satisfy.
 * Validated at compile-time with ZERO runtime overhead.
 *
 * Required methods:
 * - open() -> Result<void>
 * - close() -> Result<void>
 * - write(const uint8_t*, size_t) -> Result<size_t>
 * - read(uint8_t*, size_t) -> Result<size_t>
 * - setBaudrate(Baudrate) -> Result<void>
 * - getBaudrate() -> Result<Baudrate>
 * - isOpen() -> bool
 *
 * @tparam T The type to validate against UART concept
 *
 * Example usage:
 * @code
 * template <UartConcept TUart>
 * void send_message(TUart& uart, const char* msg) {
 *     uart.open();
 *     uart.setBaudrate(Baudrate::e115200);
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
 *     uart.close();
 * }
 * @endcode
 */
template <typename T>
concept UartConcept = requires(
    T uart,
    const T const_uart,
    const uint8_t* const_data,
    uint8_t* data,
    size_t size,
    Baudrate baudrate
) {
    // Open/close operations
    { uart.open() } -> std::same_as<Result<void>>;
    { uart.close() } -> std::same_as<Result<void>>;

    // Data transfer operations
    { uart.write(const_data, size) } -> std::same_as<Result<size_t>>;
    { uart.read(data, size) } -> std::same_as<Result<size_t>>;

    // Configuration operations
    { uart.setBaudrate(baudrate) } -> std::same_as<Result<void>>;
    { const_uart.getBaudrate() } -> std::same_as<Result<Baudrate>>;

    // State query
    { const_uart.isOpen() } -> std::same_as<bool>;
};

#else
// ============================================================================
// C++17 Fallback - Using SFINAE + static_assert
// ============================================================================

namespace detail {

// Helper to check if T has open() -> Result<void>
template <typename T, typename = void>
struct has_open : std::false_type {};

template <typename T>
struct has_open<T, std::void_t<
    decltype(std::declval<T&>().open())
>> : std::is_same<
    decltype(std::declval<T&>().open()),
    Result<void>
> {};

// Helper to check if T has close() -> Result<void>
template <typename T, typename = void>
struct has_close : std::false_type {};

template <typename T>
struct has_close<T, std::void_t<
    decltype(std::declval<T&>().close())
>> : std::is_same<
    decltype(std::declval<T&>().close()),
    Result<void>
> {};

// Helper to check if T has write(const uint8_t*, size_t) -> Result<size_t>
template <typename T, typename = void>
struct has_write : std::false_type {};

template <typename T>
struct has_write<T, std::void_t<
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
struct has_read : std::false_type {};

template <typename T>
struct has_read<T, std::void_t<
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

// Helper to check if T has setBaudrate(Baudrate) -> Result<void>
template <typename T, typename = void>
struct has_set_baudrate : std::false_type {};

template <typename T>
struct has_set_baudrate<T, std::void_t<
    decltype(std::declval<T&>().setBaudrate(std::declval<Baudrate>()))
>> : std::is_same<
    decltype(std::declval<T&>().setBaudrate(std::declval<Baudrate>())),
    Result<void>
> {};

// Helper to check if T has getBaudrate() const -> Result<Baudrate>
template <typename T, typename = void>
struct has_get_baudrate : std::false_type {};

template <typename T>
struct has_get_baudrate<T, std::void_t<
    decltype(std::declval<const T&>().getBaudrate())
>> : std::is_same<
    decltype(std::declval<const T&>().getBaudrate()),
    Result<Baudrate>
> {};

// Helper to check if T has isOpen() const -> bool
template <typename T, typename = void>
struct has_is_open : std::false_type {};

template <typename T>
struct has_is_open<T, std::void_t<
    decltype(std::declval<const T&>().isOpen())
>> : std::is_same<
    decltype(std::declval<const T&>().isOpen()),
    bool
> {};

} // namespace detail

/**
 * @brief Type trait to check if T satisfies UartConcept (C++17 version)
 *
 * Usage with static_assert:
 * @code
 * template <typename TUart>
 * void send_message(TUart& uart, const char* msg) {
 *     static_assert(is_uart_v<TUart>, "TUart must satisfy UartConcept");
 *     // ... implementation
 * }
 * @endcode
 */
template <typename T>
struct is_uart : std::conjunction<
    detail::has_open<T>,
    detail::has_close<T>,
    detail::has_write<T>,
    detail::has_read<T>,
    detail::has_set_baudrate<T>,
    detail::has_get_baudrate<T>,
    detail::has_is_open<T>
> {};

/// @brief Helper variable template for is_uart
template <typename T>
inline constexpr bool is_uart_v = is_uart<T>::value;

#endif // __cplusplus >= 202002L

} // namespace alloy::hal::concepts

// ============================================================================
// Usage Examples (Documentation)
// ============================================================================

#if 0 // Documentation only, not compiled

// Example 1: Generic function using UART concept
#if __cplusplus >= 202002L
template <alloy::hal::concepts::UartConcept TUart>
#else
template <typename TUart>
#endif
void send_hello_world(TUart& uart) {
#if __cplusplus < 202002L
    static_assert(alloy::hal::concepts::is_uart_v<TUart>,
                  "TUart must satisfy UartConcept");
#endif

    using namespace alloy::hal;

    const char* msg = "Hello, World!\n";
    const size_t len = 14;

    if (auto result = uart.open(); !result) {
        // Handle error
        return;
    }

    if (auto result = uart.setBaudrate(Baudrate::e115200); !result) {
        // Handle error
        uart.close();
        return;
    }

    if (auto result = uart.write(reinterpret_cast<const uint8_t*>(msg), len); !result) {
        // Handle error
        uart.close();
        return;
    }

    uart.close();
}

// Example 2: Platform-specific UART implementation
namespace alloy::hal::same70 {

template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Uart {
public:
    // This class satisfies UartConcept at compile-time
    Result<void> open();
    Result<void> close();
    Result<size_t> write(const uint8_t* data, size_t size);
    Result<size_t> read(uint8_t* data, size_t size);
    Result<void> setBaudrate(Baudrate baudrate);
    Result<Baudrate> getBaudrate() const;
    bool isOpen() const;

private:
    bool m_opened = false;
};

using Uart0 = Uart<0x400E0800, 7>;

} // namespace alloy::hal::same70

// Example 3: Compile-time validation
void example_usage() {
    using namespace alloy::hal::same70;

    auto uart = Uart0{};

    // This works because Uart0 satisfies UartConcept
    send_hello_world(uart);

    // This would fail at compile-time:
    // int not_a_uart = 42;
    // send_hello_world(not_a_uart); // ERROR: does not satisfy UartConcept
}

#endif // Documentation examples
