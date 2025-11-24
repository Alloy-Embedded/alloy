/**
 * @file usart_hardware_policy.hpp
 * @brief Hardware Policy for USART on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for USART using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32G0 USART Features:
 * - Register-compatible with STM32F7/F4
 * - Low-power UART (LPUART1) with wakeup capability
 * - Advanced FIFO mode (up to 8 bytes)
 * - Auto baud rate detection
 * - Wakeup from STOP mode
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

// Register definitions (STM32G0 uses similar USART register layout)
#include "hal/vendors/st/stm32g0/generated/registers/usart1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/generated/bitfields/usart1_bitfields.hpp"

namespace ucore::hal::stm32g0 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32g0;

// Namespace alias for bitfields
namespace usart = st::stm32g0::usart1;

/**
 * @brief Hardware Policy for USART on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for USART. It is designed to be used as a template parameter in
 * generic USART implementations.
 *
 * The STM32G0 USART peripheral is register-compatible with STM32F7/F4,
 * but adds low-power features and enhanced FIFO support.
 *
 * Template Parameters:
 * - BASE_ADDR: USART peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz (typically 64 MHz)
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using Usart1 = UartImpl<Stm32g0UartHardwarePolicy<0x40013800, 64000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for baud rate calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0UartHardwarePolicy {
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
        100000;  ///< UART timeout in loop iterations (~10ms at 64MHz)

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
        hw()->CR1 &= ~(usart::cr1::M0::mask | usart::cr1::M1::mask | usart::cr1::PCE::mask);
        hw()->CR2 &= ~(usart::cr2::STOP::mask);
    }

    /**
     * @brief Set USART baud rate using BRR register
     *
     * Baud rate calculation (16x oversampling):
     * BRR = (peripheral_clock_hz / baud_rate)
     *
     * STM32G0 supports fractional baud rate generation for better accuracy.
     *
     * @param baud Desired baud rate (e.g., 115200)
     */
    static inline void set_baudrate(uint32_t baud) {
        // Calculate USARTDIV with rounding
        uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
        hw()->BRR = usartdiv;
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
     * On STM32G0, this checks ISR.TXE_TXFNF (TX Empty or TX FIFO Not Full).
     *
     * @return true if TDR is empty and ready for new data
     */
    static inline bool is_tx_ready() {
        return (hw()->ISR & usart::isr::TXE_TXFNF::mask) != 0;
    }

    /**
     * @brief Check if receive data register is not empty (RXNE flag)
     *
     * On STM32G0, this checks ISR.RXNE_RXFNE (RX Not Empty or RX FIFO Not Empty).
     *
     * @return true if RDR contains valid received data
     */
    static inline bool is_rx_ready() {
        return (hw()->ISR & usart::isr::RXNE_RXFNE::mask) != 0;
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

    // ========================================================================
    // STM32G0-Specific Low-Power Features
    // ========================================================================

    /**
     * @brief Enable wakeup from STOP mode
     *
     * Allows USART to wake up the MCU from STOP mode when data is received.
     * Only available on LPUART1.
     */
    static inline void enable_wakeup() {
        hw()->CR3 |= usart::cr3::WUFIE::mask;  // Wakeup from Stop mode interrupt enable
        hw()->CR1 |= usart::cr1::UESM::mask;   // USART enable in Stop mode
    }

    /**
     * @brief Disable wakeup from STOP mode
     */
    static inline void disable_wakeup() {
        hw()->CR3 &= ~usart::cr3::WUFIE::mask;
        hw()->CR1 &= ~usart::cr1::UESM::mask;
    }

    /**
     * @brief Enable FIFO mode (STM32G0 feature)
     *
     * Enables 8-byte TX and RX FIFOs for improved performance.
     */
    static inline void enable_fifo() {
        hw()->CR1 |= usart::cr1::FIFOEN::mask;
    }

    /**
     * @brief Disable FIFO mode
     */
    static inline void disable_fifo() {
        hw()->CR1 &= ~usart::cr1::FIFOEN::mask;
    }
};

}  // namespace ucore::hal::stm32g0

/**
 * @example STM32G0 USART Example
 * @code
 * #include "hal/platform/stm32g0/uart.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto uart = Usart2::quick_setup<TxPin, RxPin>(BaudRate{115200});
 *     uart.initialize();
 *
 *     const char* msg = "Hello from STM32G071!\n";
 *     uart.write(reinterpret_cast<const uint8_t*>(msg), 22);
 * }
 * @endcode
 */

/**
 * @example STM32G0 LPUART Low-Power Example
 * @code
 * #include "hal/platform/stm32g0/uart.hpp"
 *
 * using namespace ucore::hal::stm32g0;
 *
 * int main() {
 *     // LPUART with wakeup capability
 *     auto lpuart = Lpuart1::quick_setup<TxPin, RxPin>(BaudRate{9600});
 *     lpuart.initialize();
 *     lpuart.enable_wakeup();  // Enable wakeup from STOP mode
 *
 *     while (true) {
 *         lpuart.write_str("Entering STOP mode...\n");
 *         __WFI();  // Enter STOP mode - LPUART will wake on RX
 *
 *         // Woken up by UART RX
 *         uint8_t data;
 *         if (lpuart.read(&data, 1).is_ok()) {
 *             lpuart.write(&data, 1);  // Echo
 *         }
 *     }
 * }
 * @endcode
 */
