/**
 * @file uart.hpp
 * @brief Template-based UART implementation for SAME70 (Platform Layer)
 *
 * This file implements UART peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - DMA support: Can work with DMA for high-speed transfers
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: same70
 * Generator: generate_platform_uart.py
 * Generated: 2025-11-14 09:58:16
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/same70/registers/uart0_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/uart0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace uart = atmel::same70::uart0;

// Note: UART uses common types from hal/types.hpp:
// - Parity (None, Even, Odd)
// - StopBits (One, Two)
// - WordLength (Bits7, Bits8, Bits9)
// - FlowControl (None, RTS, CTS, RTS_CTS)




/**
 * @brief Template-based UART peripheral for SAME70
 *
 * This class provides a template-based UART implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: UART peripheral base address
 * - IRQ_ID: UART interrupt ID for clock enable
 *
 * Example usage:
 * @code
 * // Basic UART usage example
 * using MyUart = Uart<UART0_BASE, UART0_IRQ>;
 * auto uart = MyUart{};
 * uart.open(115200);
 * const char* msg = "Hello\n";
 * uart.write(reinterpret_cast<const uint8_t*>(msg), 6);
 * uart.close();
 * @endcode
 *
 * @tparam BASE_ADDR UART peripheral base address
 * @tparam IRQ_ID UART interrupt ID for clock enable
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Uart {

public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t UART_TIMEOUT = 100000;  ///< UART timeout in loop iterations (~10ms at 150MHz)

    /**
     * @brief Get UART peripheral registers
     *
     * Returns pointer to UART registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile atmel::same70::uart0::UART0_Registers* get_hw() {
#ifdef ALLOY_UART_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_UART_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::uart0::UART0_Registers*>(BASE_ADDR);
#endif
    }


    constexpr Uart() = default;

    /**
     * @brief Initialize UART peripheral
     *
     * @param baud_rate Desired baud rate (e.g., 115200)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open(uint32_t baud_rate) {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable peripheral clock (PMC)
        // TODO: Enable peripheral clock via PMC

        // Reset and disable TX/RX
        hw->CR = uart::cr::RSTRX::mask | uart::cr::RSTTX::mask | uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;

        // Configure mode: 8N1 (8 data bits, no parity, 1 stop bit)
        hw->MR = uart::mr::PAR::write(0, uart::mr::par::NO_PARITY);

        // Calculate and set baud rate divisor (assuming 150MHz peripheral clock)
        constexpr uint32_t peripheral_clock = 150000000;
        uint32_t cd = peripheral_clock / (16 * baud_rate);
        hw->BRGR = uart::brgr::CD::write(0, cd);

        // Enable TX and RX
        hw->CR = uart::cr::RXEN::mask | uart::cr::TXEN::mask;

        m_opened = true;


        return Ok();
    }

    /**
     * @brief Deinitialize UART peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable TX and RX
        hw->CR = uart::cr::RXDIS::mask | uart::cr::TXDIS::mask;

        // Disable peripheral clock (PMC)
        // TODO: Disable peripheral clock via PMC

        m_opened = false;


        return Ok();
    }

    /**
     * @brief Write data to UART (blocking)
     *
     * @param data Data buffer to write
     * @param size Number of bytes to write
     * @return Result<size_t, ErrorCode> Write data to UART (blocking)     */
    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Transmit each byte with timeout
        for (size_t i = 0; i < size; ++i) {
            uint32_t timeout = UART_TIMEOUT;
            while (!(hw->SR & uart::sr::TXRDY::mask) && --timeout);
            if (timeout == 0) {
                return Err(ErrorCode::Timeout);
            }
            hw->THR = data[i];
        }


        return Ok(size_t(size));
    }

    /**
     * @brief Read data from UART (blocking)
     *
     * @param data Buffer for received data
     * @param size Number of bytes to read
     * @return Result<size_t, ErrorCode> Read data from UART (blocking)     */
    Result<size_t, ErrorCode> read(uint8_t* data, size_t size) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Receive each byte with timeout
        for (size_t i = 0; i < size; ++i) {
            uint32_t timeout = UART_TIMEOUT;
            while (!(hw->SR & uart::sr::RXRDY::mask) && --timeout);
            if (timeout == 0) {
                return Err(ErrorCode::Timeout);
            }
            data[i] = static_cast<uint8_t>(hw->RHR);
        }


        return Ok(size_t(size));
    }

    /**
     * @brief Write single byte to UART (blocking)
     *
     * @param byte Byte to write
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> writeByte(uint8_t byte) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Wait for TX ready with timeout
        uint32_t timeout = UART_TIMEOUT;
        while (!(hw->SR & uart::sr::TXRDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }
        hw->THR = byte;


        return Ok();
    }

    /**
     * @brief Read single byte from UART (blocking)
     *
     * @return Result<uint8_t, ErrorCode> Read single byte from UART (blocking)     */
    Result<uint8_t, ErrorCode> readByte() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Wait for RX ready with timeout
        uint32_t timeout = UART_TIMEOUT;
        while (!(hw->SR & uart::sr::RXRDY::mask) && --timeout);
        if (timeout == 0) {
            return Err(ErrorCode::Timeout);
        }


        return Ok(uint8_t(static_cast<uint8_t>(hw->RHR)));
    }

    /**
     * @brief Check if UART peripheral is open
     *
     * @return bool Check if UART peripheral is open     */
    bool isOpen() const {

        return m_opened;
    }

    /**
     * @brief Check if data is available to read
     *
     * @return bool Check if data is available to read     */
    bool isReadReady() const {

        return (hw->SR & uart::sr::RXRDY::mask) != 0;
    }

    /**
     * @brief Check if TX buffer is ready for write
     *
     * @return bool Check if TX buffer is ready for write     */
    bool isWriteReady() const {

        return (hw->SR & uart::sr::TXRDY::mask) != 0;
    }

private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
};

// ==============================================================================
// Predefined UART Instances
// ==============================================================================

constexpr uint32_t UART0_BASE = alloy::generated::atsame70q21b::peripherals::UART0;
constexpr uint32_t UART0_IRQ = alloy::generated::atsame70q21b::id::UART0;

constexpr uint32_t UART1_BASE = alloy::generated::atsame70q21b::peripherals::UART1;
constexpr uint32_t UART1_IRQ = alloy::generated::atsame70q21b::id::UART1;

constexpr uint32_t UART2_BASE = alloy::generated::atsame70q21b::peripherals::UART2;
constexpr uint32_t UART2_IRQ = alloy::generated::atsame70q21b::id::UART2;

constexpr uint32_t UART3_BASE = alloy::generated::atsame70q21b::peripherals::UART3;
constexpr uint32_t UART3_IRQ = alloy::generated::atsame70q21b::id::UART3;

constexpr uint32_t UART4_BASE = alloy::generated::atsame70q21b::peripherals::UART4;
constexpr uint32_t UART4_IRQ = alloy::generated::atsame70q21b::id::UART4;

using Uart0 = Uart<UART0_BASE, UART0_IRQ>;  ///< UART0 instance
using Uart1 = Uart<UART1_BASE, UART1_IRQ>;  ///< UART1 instance
using Uart2 = Uart<UART2_BASE, UART2_IRQ>;  ///< UART2 instance
using Uart3 = Uart<UART3_BASE, UART3_IRQ>;  ///< UART3 instance
using Uart4 = Uart<UART4_BASE, UART4_IRQ>;  ///< UART4 instance

} // namespace alloy::hal::same70