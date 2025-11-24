/**
 * @file usart_hardware_policy.hpp
 * @brief Hardware Policy for USART on STM32F7 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for USART using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F7 USART Features:
 * - Compatible with STM32F4 register layout
 * - Higher clock speeds (up to 216 MHz)
 * - Enhanced FIFO support
 * - Advanced oversampling (8x or 16x)
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef MICROCORE_UART_MOCK_HW)
 *
 * @note Part of Phase 3.2: UART Platform Completeness
 * @see docs/API_TIERS.md
 */

#pragma once

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// Register definitions (STM32F7 uses same USART register layout as F4)
#include "hal/vendors/st/stm32f7/generated/registers/usart1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f7/generated/bitfields/usart1_bitfields.hpp"

namespace ucore::hal::stm32f7 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f7;

// Namespace alias for bitfields
namespace usart = st::stm32f7::usart1;

/**
 * @brief Hardware Policy for USART on STM32F7
 *
 * This policy provides all platform-specific hardware access methods
 * for USART. It is designed to be used as a template parameter in
 * generic USART implementations.
 *
 * The STM32F7 USART peripheral is register-compatible with STM32F4,
 * but supports higher clock speeds (up to 216 MHz on F722/F723).
 *
 * Template Parameters:
 * - BASE_ADDR: USART peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz (APB1/APB2)
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using Usart1 = UartImpl<Stm32f7UartHardwarePolicy<0x40011000, 216000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for baud rate calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f7UartHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = USART1_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t UART_TIMEOUT =
        100000;  ///< UART timeout in loop iterations (~10ms at 216MHz)

    // ========================================================================
    // Hardware Accessor (with Mock Hook)
    // ========================================================================

    /**
     * @brief Get pointer to hardware registers
     *
     * This method provides access to the actual hardware registers.
     * It includes a mock hook for testing purposes.
     *
     * @return Pointer to hardware registers
     */
    static inline volatile RegisterType* hw() {
#ifdef MICROCORE_UART_MOCK_HW
        return MICROCORE_UART_MOCK_HW();  // Test hook
#else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
#endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Reset USART peripheral (disable TX and RX)
     */
    static inline void reset() {
        hw()->CR1 &= ~(usart::cr1::TE::mask | usart::cr1::RE::mask | usart::cr1::UE::mask);
    }

    /**
     * @brief Configure 8 data bits, no parity, 1 stop bit (8N1)
     */
    static inline void configure_8n1() {
        // 8 data bits: M[1:0] = 00 (8-bit word length)
        // No parity: PCE = 0
        // 1 stop bit: STOP[1:0] = 00
        hw()->CR1 &= ~(usart::cr1::M::mask | usart::cr1::PCE::mask);
        hw()->CR2 &= ~(usart::cr2::STOP::mask);
    }

    /**
     * @brief Set USART baud rate using BRR register
     *
     * Baud rate calculation (16x oversampling):
     * BRR = (peripheral_clock_hz / baud_rate)
     *
     * @param baud Desired baud rate (e.g., 115200)
     */
    static inline void set_baudrate(uint32_t baud) {
        // Calculate USARTDIV with rounding
        uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
        uint32_t mantissa = usartdiv >> 4;
        uint32_t fraction = usartdiv & 0xF;
        hw()->BRR = (mantissa << 4) | fraction;
    }

    /**
     * @brief Enable transmitter
     *
     * Sets TE (Transmitter Enable) and UE (USART Enable) bits.
     */
    static inline void enable_tx() {
        hw()->CR1 |= usart::cr1::TE::mask | usart::cr1::UE::mask;
    }

    /**
     * @brief Enable receiver
     *
     * Sets RE (Receiver Enable) and UE (USART Enable) bits.
     */
    static inline void enable_rx() {
        hw()->CR1 |= usart::cr1::RE::mask | usart::cr1::UE::mask;
    }

    /**
     * @brief Disable transmitter
     */
    static inline void disable_tx() {
        hw()->CR1 &= ~usart::cr1::TE::mask;
    }

    /**
     * @brief Disable receiver
     */
    static inline void disable_rx() {
        hw()->CR1 &= ~usart::cr1::RE::mask;
    }

    /**
     * @brief Check if transmit data register is empty (TXE flag)
     *
     * @return true if TDR is empty and ready for new data
     */
    static inline bool is_tx_ready() {
        return (hw()->ISR & usart::isr::TXE::mask) != 0;
    }

    /**
     * @brief Check if receive data register is not empty (RXNE flag)
     *
     * @return true if RDR contains valid received data
     */
    static inline bool is_rx_ready() {
        return (hw()->ISR & usart::isr::RXNE::mask) != 0;
    }

    /**
     * @brief Write single byte to transmit data register
     *
     * @param byte Byte to write
     *
     * @note Caller must check is_tx_ready() before calling
     */
    static inline void write_byte(uint8_t byte) {
        hw()->TDR = byte;
    }

    /**
     * @brief Read single byte from receive data register
     *
     * @return Received byte
     *
     * @note Caller must check is_rx_ready() before calling
     */
    static inline uint8_t read_byte() {
        return static_cast<uint8_t>(hw()->RDR & 0xFF);
    }

    /**
     * @brief Wait for TX ready with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return true if TX became ready, false if timeout
     */
    static inline bool wait_tx_ready(uint32_t timeout_loops = UART_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (!is_tx_ready() && --timeout)
            ;
        return timeout != 0;
    }

    /**
     * @brief Wait for RX ready with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return true if RX became ready, false if timeout
     */
    static inline bool wait_rx_ready(uint32_t timeout_loops = UART_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (!is_rx_ready() && --timeout)
            ;
        return timeout != 0;
    }

    /**
     * @brief Check transmission complete (TC flag)
     *
     * @return true if transmission is complete
     */
    static inline bool is_transmission_complete() {
        return (hw()->ISR & usart::isr::TC::mask) != 0;
    }

    /**
     * @brief Clear transmission complete flag
     */
    static inline void clear_transmission_complete() {
        hw()->ICR = usart::icr::TCCF::mask;
    }
};

}  // namespace ucore::hal::stm32f7

/**
 * @example STM32F7 USART Example
 * @code
 * #include "hal/platform/stm32f7/uart.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto uart = Usart1::quick_setup<TxPin, RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello from STM32F722!\n";
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), 22);
 * }
 * @endcode
 */
