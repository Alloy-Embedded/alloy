/// Auto-generated BitField Template Utilities
/// Part of Alloy HAL Code Generator
///
/// This file provides zero-overhead bit field manipulation templates
/// using modern C++20 features. All operations are constexpr and inline,
/// resulting in assembly identical to manual bit manipulation.
///
/// DO NOT EDIT - This is a template file

#pragma once

#include <concepts>
#include <type_traits>

#include <stdint.h>

namespace alloy::hal::bitfields {

// ============================================================================
// BIT FIELD TEMPLATE
// ============================================================================

/// Type-safe bit field template with compile-time validation
///
/// @tparam Pos  Bit position (0-31)
/// @tparam Width  Bit width (1-32)
///
/// Usage:
///   using TXE = BitField<7, 1>;  // Single bit at position 7
///   using BRR = BitField<0, 16>; // 16-bit field at position 0
///
///   uint32_t reg = 0;
///   reg = TXE::set(reg);              // Set bit 7
///   reg = BRR::write(reg, 0x1D4C);    // Write 16-bit value
///   bool is_set = TXE::test(reg);     // Test if bit 7 is set
///
template <uint32_t Pos, uint32_t Width>
struct BitField {
    // Compile-time validation
    static_assert(Pos < 32, "Bit position must be less than 32");
    static_assert(Width > 0 && Width <= 32, "Bit width must be between 1 and 32");
    static_assert(Pos + Width <= 32, "Bit field exceeds register size (32 bits)");

    // Constants
    static constexpr uint32_t position = Pos;
    static constexpr uint32_t width = Width;
    static constexpr uint32_t mask = ((1U << Width) - 1) << Pos;

    /// Read bit field value from register
    ///
    /// @param reg  Register value
    /// @return  Extracted bit field value (shifted to LSB)
    ///
    /// Example:
    ///   uint32_t brr_value = BRR::read(USART1->BRR);
    [[nodiscard]] static constexpr uint32_t read(uint32_t reg) noexcept {
        return (reg & mask) >> position;
    }

    /// Write bit field value to register
    ///
    /// @param reg  Current register value
    /// @param value  Value to write (will be masked to fit)
    /// @return  Modified register value
    ///
    /// Example:
    ///   USART1->BRR = BRR::write(USART1->BRR, 0x1D4C);
    [[nodiscard]] static constexpr uint32_t write(uint32_t reg, uint32_t value) noexcept {
        return (reg & ~mask) | ((value << position) & mask);
    }

    /// Set bit field (all bits to 1)
    ///
    /// @param reg  Current register value
    /// @return  Register value with bit field set
    ///
    /// Example:
    ///   USART1->CR1 = UE::set(USART1->CR1);  // Enable USART
    [[nodiscard]] static constexpr uint32_t set(uint32_t reg) noexcept { return reg | mask; }

    /// Clear bit field (all bits to 0)
    ///
    /// @param reg  Current register value
    /// @return  Register value with bit field cleared
    ///
    /// Example:
    ///   USART1->CR1 = UE::clear(USART1->CR1);  // Disable USART
    [[nodiscard]] static constexpr uint32_t clear(uint32_t reg) noexcept { return reg & ~mask; }

    /// Test if any bit in field is set
    ///
    /// @param reg  Register value
    /// @return  True if any bit is set, false otherwise
    ///
    /// Example:
    ///   while (!TXE::test(USART1->SR)) { /* wait */ }
    [[nodiscard]] static constexpr bool test(uint32_t reg) noexcept { return (reg & mask) != 0; }

    /// Test if bit field equals specific value
    ///
    /// @param reg  Register value
    /// @param value  Value to compare against
    /// @return  True if bit field equals value
    ///
    /// Example:
    ///   if (MODE::equals(GPIOA->MODER, 0b01)) { /* output mode */ }
    [[nodiscard]] static constexpr bool equals(uint32_t reg, uint32_t value) noexcept {
        return read(reg) == value;
    }

    /// Toggle bit field (flip all bits)
    ///
    /// @param reg  Current register value
    /// @return  Register value with bit field toggled
    ///
    /// Example:
    ///   GPIOC->ODR = LED::toggle(GPIOC->ODR);
    [[nodiscard]] static constexpr uint32_t toggle(uint32_t reg) noexcept { return reg ^ mask; }
};

// ============================================================================
// CONVENIENCE ALIASES
// ============================================================================

/// Single-bit field specialization
template <uint32_t Pos>
using Bit = BitField<Pos, 1>;

// ============================================================================
// MULTI-BIT FIELD OPERATIONS
// ============================================================================

/// Set multiple bit fields atomically
///
/// Example:
///   uint32_t cr1 = 0;
///   cr1 = UE::set(TE::set(RE::set(cr1)));  // Enable USART, TX, RX
template <typename... BitFields>
[[nodiscard]] constexpr uint32_t set_bits(uint32_t reg) noexcept {
    return ((reg | BitFields::mask) | ...);
}

/// Clear multiple bit fields atomically
template <typename... BitFields>
[[nodiscard]] constexpr uint32_t clear_bits(uint32_t reg) noexcept {
    return ((reg & ~BitFields::mask) & ...);
}

// ============================================================================
// REGISTER ACCESS CONCEPTS (C++20)
// ============================================================================

/// Concept for register size validation
template <typename T>
concept RegisterSize =
    std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t>;

// ============================================================================
// 16-BIT REGISTER SUPPORT
// ============================================================================

/// BitField specialization for 16-bit registers
template <uint32_t Pos, uint32_t Width>
struct BitField16 {
    static_assert(Pos < 16, "Bit position must be less than 16");
    static_assert(Width > 0 && Width <= 16, "Bit width must be between 1 and 16");
    static_assert(Pos + Width <= 16, "Bit field exceeds register size (16 bits)");

    static constexpr uint16_t position = Pos;
    static constexpr uint16_t width = Width;
    static constexpr uint16_t mask = ((1U << Width) - 1) << Pos;

    [[nodiscard]] static constexpr uint16_t read(uint16_t reg) noexcept {
        return (reg & mask) >> position;
    }

    [[nodiscard]] static constexpr uint16_t write(uint16_t reg, uint16_t value) noexcept {
        return (reg & ~mask) | ((value << position) & mask);
    }

    [[nodiscard]] static constexpr uint16_t set(uint16_t reg) noexcept { return reg | mask; }

    [[nodiscard]] static constexpr uint16_t clear(uint16_t reg) noexcept { return reg & ~mask; }

    [[nodiscard]] static constexpr bool test(uint16_t reg) noexcept { return (reg & mask) != 0; }

    [[nodiscard]] static constexpr bool equals(uint16_t reg, uint16_t value) noexcept {
        return read(reg) == value;
    }

    [[nodiscard]] static constexpr uint16_t toggle(uint16_t reg) noexcept { return reg ^ mask; }
};

// ============================================================================
// 8-BIT REGISTER SUPPORT
// ============================================================================

/// BitField specialization for 8-bit registers
template <uint32_t Pos, uint32_t Width>
struct BitField8 {
    static_assert(Pos < 8, "Bit position must be less than 8");
    static_assert(Width > 0 && Width <= 8, "Bit width must be between 1 and 8");
    static_assert(Pos + Width <= 8, "Bit field exceeds register size (8 bits)");

    static constexpr uint8_t position = Pos;
    static constexpr uint8_t width = Width;
    static constexpr uint8_t mask = ((1U << Width) - 1) << Pos;

    [[nodiscard]] static constexpr uint8_t read(uint8_t reg) noexcept {
        return (reg & mask) >> position;
    }

    [[nodiscard]] static constexpr uint8_t write(uint8_t reg, uint8_t value) noexcept {
        return (reg & ~mask) | ((value << position) & mask);
    }

    [[nodiscard]] static constexpr uint8_t set(uint8_t reg) noexcept { return reg | mask; }

    [[nodiscard]] static constexpr uint8_t clear(uint8_t reg) noexcept { return reg & ~mask; }

    [[nodiscard]] static constexpr bool test(uint8_t reg) noexcept { return (reg & mask) != 0; }

    [[nodiscard]] static constexpr bool equals(uint8_t reg, uint8_t value) noexcept {
        return read(reg) == value;
    }

    [[nodiscard]] static constexpr uint8_t toggle(uint8_t reg) noexcept { return reg ^ mask; }
};

}  // namespace alloy::hal::bitfields
