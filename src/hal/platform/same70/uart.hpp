/**
 * @file uart.hpp
 * @brief Template-based UART implementation for SAME70 (ARM Cortex-M7)
 *
 * This file implements the UART peripheral for SAME70 using templates
 * with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Concept-validated: Satisfies UartConcept at compile-time
 * - Type-safe: Strong typing prevents errors
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"
#include "hal/concepts/uart_concept.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/atsame70q21/registers/uart0_registers.hpp"
#include "hal/vendors/atmel/same70/atsame70q21/bitfields/uart0_bitfields.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import SAME70 register types
using namespace alloy::hal::atmel::same70::atsame70q21;

/**
 * @brief Template-based UART peripheral for SAME70
 *
 * This class provides a template-based UART implementation that satisfies
 * UartConcept at compile-time with ZERO runtime overhead.
 *
 * Template Parameters:
 * - BASE_ADDR: UART peripheral base address (compile-time constant)
 * - IRQ_ID: UART interrupt ID (for PMC clock enable)
 *
 * Example usage:
 * @code
 * // Define UART0 instance
 * using Uart0 = Uart<0x400E0800, 7>;
 *
 * // Use it
 * auto uart = Uart0{};
 * uart.open();
 * uart.setBaudrate(Baudrate::e115200);
 * uart.write(data, size);
 * uart.close();
 * @endcode
 *
 * @tparam BASE_ADDR UART peripheral base address
 * @tparam IRQ_ID UART peripheral ID (for PMC clock)
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Uart {
public:
    // Compile-time constants
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Hardware register access (compile-time resolved address, runtime pointer)
    // Note: reinterpret_cast cannot be constexpr, but BASE_ADDR is still compile-time
    static inline volatile uart0::UART0_Registers* get_hw() {
#ifdef ALLOY_UART_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_UART_MOCK_HW();
#else
        return reinterpret_cast<volatile uart0::UART0_Registers*>(BASE_ADDR);
#endif
    }

    // PMC (Power Management Controller) for clock enable
    static inline volatile uint32_t* get_pmc() {
#ifdef ALLOY_UART_MOCK_PMC
        // In tests, use the mock PMC pointer
        return ALLOY_UART_MOCK_PMC();
#else
        return reinterpret_cast<volatile uint32_t*>(0x400E0610);
#endif
    }

    /**
     * @brief Constructor
     *
     * Note: Does NOT initialize hardware - call open() first
     */
    constexpr Uart() = default;

    /**
     * @brief Open UART (enable clocks, configure peripheral)
     *
     * This method:
     * 1. Enables peripheral clock via PMC
     * 2. Resets receiver and transmitter
     * 3. Sets default configuration (8N1, no parity)
     * 4. Enables receiver and transmitter
     *
     * @return Result<void> Ok() if successful, Err() if already open
     */
    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* hw = get_hw();
        auto* pmc = get_pmc();

        // Enable peripheral clock
        *pmc = (1u << IRQ_ID);

        // Reset and disable receiver and transmitter
        hw->CR = uart0::cr::RSTRX::mask | uart0::cr::RSTTX::mask |
                 uart0::cr::RXDIS::mask | uart0::cr::TXDIS::mask;

        // Configure mode: 8-bit, no parity, normal channel mode
        hw->MR = (uart0::mr::par::NO << uart0::mr::PAR_Pos) |
                 (uart0::mr::chmode::NORMAL << uart0::mr::CHMODE_Pos);

        // Set default baudrate (115200)
        setBaudrateInternal(Baudrate::e115200);

        // Reset status bits
        hw->CR = uart0::cr::RSTSTA::mask;

        // Enable receiver and transmitter
        hw->CR = uart0::cr::RXEN::mask | uart0::cr::TXEN::mask;

        m_opened = true;
        return Result<void>::ok();
    }

    /**
     * @brief Close UART (disable peripheral, save power)
     *
     * @return Result<void> Ok() if successful, Err() if not open
     */
    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Disable receiver and transmitter
        hw->CR = uart0::cr::RXDIS::mask | uart0::cr::TXDIS::mask;

        // Note: We don't disable peripheral clock here
        // (would affect other UARTs sharing the same clock domain)

        m_opened = false;
        return Result<void>::ok();
    }

    /**
     * @brief Write data to UART (blocking)
     *
     * Sends bytes one at a time, waiting for TXRDY flag.
     * This is a blocking operation.
     *
     * @param data Pointer to data buffer
     * @param size Number of bytes to write
     * @return Result<size_t> Number of bytes written, or error
     */
    Result<size_t> write(const uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();

        // Send each byte
        for (size_t i = 0; i < size; ++i) {
            // Wait for transmitter ready
            while (!(hw->SR & uart0::sr::TXRDY::mask)) {
                // Busy wait (TODO: add timeout)
            }

            // Write byte to THR
            hw->THR = data[i];

#ifdef ALLOY_UART_TEST_HOOK_THR
            // Test hook: notify mock of THR write
            ALLOY_UART_TEST_HOOK_THR(hw, data[i]);
#endif
        }

        return Result<size_t>::ok(size);
    }

    /**
     * @brief Read data from UART (blocking, with timeout)
     *
     * Reads bytes one at a time, waiting for RXRDY flag.
     * This is a blocking operation with simple timeout.
     *
     * @param data Pointer to buffer for received data
     * @param size Maximum number of bytes to read
     * @return Result<size_t> Number of bytes read, or error
     */
    Result<size_t> read(uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();
        size_t bytes_read = 0;

        for (size_t i = 0; i < size; ++i) {
            // Simple timeout counter (not accurate, but prevents infinite loop)
            uint32_t timeout = 100000;

            // Wait for receiver ready
            while (!(hw->SR & uart0::sr::RXRDY::mask)) {
                if (--timeout == 0) {
                    // Timeout - return bytes read so far
                    return Result<size_t>::ok(bytes_read);
                }
            }

            // Read byte from RHR
            data[i] = static_cast<uint8_t>(hw->RHR & 0xFF);

#ifdef ALLOY_UART_TEST_HOOK_RHR
            // Test hook: notify mock of RHR read
            ALLOY_UART_TEST_HOOK_RHR(hw);
#endif
            bytes_read++;
        }

        return Result<size_t>::ok(bytes_read);
    }

    /**
     * @brief Set UART baudrate
     *
     * Calculates and sets the baud rate divisor register (BRGR).
     *
     * Formula: BRGR = (peripheral_clock / (16 * baudrate))
     * Assuming peripheral clock = 150 MHz (default SAME70)
     *
     * @param baudrate Desired baudrate (from Baudrate enum)
     * @return Result<void> Ok() if successful, Err() if invalid
     */
    Result<void> setBaudrate(Baudrate baudrate) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        return setBaudrateInternal(baudrate);
    }

    /**
     * @brief Get current UART baudrate
     *
     * Reads BRGR and calculates actual baudrate.
     *
     * @return Result<Baudrate> Current baudrate, or error if not open
     */
    Result<Baudrate> getBaudrate() const {
        if (!m_opened) {
            return Result<Baudrate>::error(ErrorCode::NotInitialized);
        }

        // For now, return cached value
        // TODO: Calculate from BRGR register
        return Result<Baudrate>::ok(m_current_baudrate);
    }

    /**
     * @brief Check if UART is open
     *
     * @return true if UART is opened, false otherwise
     */
    bool isOpen() const {
        return m_opened;
    }

private:
    /**
     * @brief Internal baudrate setter (used by open and setBaudrate)
     *
     * @param baudrate Desired baudrate
     * @return Result<void> Ok() if successful
     */
    Result<void> setBaudrateInternal(Baudrate baudrate) {
        // SAME70 peripheral clock is typically 150 MHz
        // (assuming main clock = 300 MHz, and UART uses MCK/2)
        constexpr uint32_t PERIPHERAL_CLOCK = 150000000;

        // Calculate BRGR value
        // BRGR = peripheral_clock / (16 * baudrate)
        uint32_t baud_value = static_cast<uint32_t>(baudrate);
        uint32_t brgr = PERIPHERAL_CLOCK / (16 * baud_value);

        // Validate BRGR (must be > 0 and < 65536)
        if (brgr == 0 || brgr > 0xFFFF) {
            return Result<void>::error(ErrorCode::OutOfRange);
        }

        auto* hw = get_hw();

        // Set baud rate generator register
        hw->BRGR = brgr;

        m_current_baudrate = baudrate;
        return Result<void>::ok();
    }

    // State
    bool m_opened = false;
    Baudrate m_current_baudrate = Baudrate::e115200;
};

// ============================================================================
// Type Aliases for SAME70 UARTs
// ============================================================================

/// UART0 (base address: 0x400E0800, IRQ ID: 7)
using Uart0 = Uart<0x400E0800, 7>;

/// UART1 (base address: 0x400E0A00, IRQ ID: 8)
using Uart1 = Uart<0x400E0A00, 8>;

/// UART2 (base address: 0x400E1A00, IRQ ID: 44)
using Uart2 = Uart<0x400E1A00, 44>;

/// UART3 (base address: 0x400E1C00, IRQ ID: 45)
using Uart3 = Uart<0x400E1C00, 45>;

/// UART4 (base address: 0x400E1E00, IRQ ID: 46)
using Uart4 = Uart<0x400E1E00, 46>;

// ============================================================================
// Compile-Time Validation
// ============================================================================

// Verify that our template satisfies UartConcept
#if __cplusplus >= 202002L
static_assert(concepts::UartConcept<Uart0>, "Uart0 does not satisfy UartConcept!");
static_assert(concepts::UartConcept<Uart1>, "Uart1 does not satisfy UartConcept!");
#else
static_assert(concepts::is_uart_v<Uart0>, "Uart0 does not satisfy UartConcept!");
static_assert(concepts::is_uart_v<Uart1>, "Uart1 does not satisfy UartConcept!");
#endif

} // namespace alloy::hal::same70
