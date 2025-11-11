/**
 * @file uart_hardware_policy.hpp
 * @brief Hardware Policy for UART on STM32F4 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for UART using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_UART_MOCK_HW)
 *
 * Auto-generated from: stm32f4/usart.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-11 07:14:44
 *
 * @note Part of Alloy HAL Vendor Layer
 * @note See ARCHITECTURE.md for Policy-Based Design rationale
 */

#pragma once

#include "core/types.hpp"
#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

// Register definitions
#include "hal/vendors/st/stm32f4/registers/usart6_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f4/bitfields/usart6_bitfields.hpp"

namespace alloy::hal::stm32f4 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfields
namespace usart = st::stm32f4::usart6;

/**
 * @brief Hardware Policy for USART on STM32F4
 *
 * This policy provides all platform-specific hardware access methods
 * for USART. It is designed to be used as a template
 * parameter in generic USART implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: USART peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * Usage:
 * @code
 * // In generic API
 * template <typename HardwarePolicy>
 * class UartImpl {
 *     void initialize() {
 *         HardwarePolicy::reset();
 *         HardwarePolicy::configure_8n1();
 *         HardwarePolicy::set_baudrate(115200);
 *         HardwarePolicy::enable_tx();
 *         HardwarePolicy::enable_rx();
 *     }
 * };
 *
 * // Platform-specific alias
 * using Uart0 = UartImpl<Stm32f4UartHardwarePolicy<UART0_BASE, 150000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for baud rate calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f4UartHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = USART6_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t UART_TIMEOUT = 100000;  ///< UART timeout in loop iterations (~10ms at 168MHz)

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
        #ifdef ALLOY_UART_MOCK_HW
            return ALLOY_UART_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Reset USART peripheral (disable TX and RX)
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_UART_TEST_HOOK_RESET
            ALLOY_UART_TEST_HOOK_RESET();
        #endif

        hw()->CR1 &= ~(usart::cr1::TE::mask | usart::cr1::RE::mask | usart::cr1::UE::mask);
    }

    /**
     * @brief Configure 8 data bits, no parity, 1 stop bit
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_CONFIGURE
     */
    static inline void configure_8n1() {
        #ifdef ALLOY_UART_TEST_HOOK_CONFIGURE
            ALLOY_UART_TEST_HOOK_CONFIGURE();
        #endif

        hw()->CR1 &= ~(usart::cr1::M::mask | usart::cr1::PCE::mask);
        hw()->CR2 &= ~(usart::cr2::STOP::mask);
    }

    /**
     * @brief Set USART baud rate using BRR register
     *
     * @param baud Desired baud rate
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_BAUDRATE
     */
    static inline void set_baudrate(uint32_t baud) {
        #ifdef ALLOY_UART_TEST_HOOK_BAUDRATE
            ALLOY_UART_TEST_HOOK_BAUDRATE(baud);
        #endif

        uint32_t usartdiv = (PERIPH_CLOCK_HZ + (baud / 2)) / baud;
        uint32_t mantissa = usartdiv >> 4;
        uint32_t fraction = usartdiv & 0xF;
        hw()->BRR = (mantissa << 4) | fraction;
    }

    /**
     * @brief Enable transmitter
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TX_ENABLE
     */
    static inline void enable_tx() {
        #ifdef ALLOY_UART_TEST_HOOK_TX_ENABLE
            ALLOY_UART_TEST_HOOK_TX_ENABLE();
        #endif

        hw()->CR1 |= usart::cr1::TE::mask | usart::cr1::UE::mask;
    }

    /**
     * @brief Enable receiver
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RX_ENABLE
     */
    static inline void enable_rx() {
        #ifdef ALLOY_UART_TEST_HOOK_RX_ENABLE
            ALLOY_UART_TEST_HOOK_RX_ENABLE();
        #endif

        hw()->CR1 |= usart::cr1::RE::mask | usart::cr1::UE::mask;
    }

    /**
     * @brief Disable transmitter
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TX_DISABLE
     */
    static inline void disable_tx() {
        #ifdef ALLOY_UART_TEST_HOOK_TX_DISABLE
            ALLOY_UART_TEST_HOOK_TX_DISABLE();
        #endif

        hw()->CR1 &= ~usart::cr1::TE::mask;
    }

    /**
     * @brief Disable receiver
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RX_DISABLE
     */
    static inline void disable_rx() {
        #ifdef ALLOY_UART_TEST_HOOK_RX_DISABLE
            ALLOY_UART_TEST_HOOK_RX_DISABLE();
        #endif

        hw()->CR1 &= ~usart::cr1::RE::mask;
    }

    /**
     * @brief Check if transmit data register is empty (TXE flag)
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TX_READY
     */
    static inline bool is_tx_ready() const {
        #ifdef ALLOY_UART_TEST_HOOK_TX_READY
            ALLOY_UART_TEST_HOOK_TX_READY();
        #endif

        return (hw()->SR & usart::sr::TXE::mask) != 0;
    }

    /**
     * @brief Check if receive data register is not empty (RXNE flag)
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RX_READY
     */
    static inline bool is_rx_ready() const {
        #ifdef ALLOY_UART_TEST_HOOK_RX_READY
            ALLOY_UART_TEST_HOOK_RX_READY();
        #endif

        return (hw()->SR & usart::sr::RXNE::mask) != 0;
    }

    /**
     * @brief Write single byte to data register
     *
     * @param byte Byte to write
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_WRITE
     */
    static inline void write_byte(uint8_t byte) {
        #ifdef ALLOY_UART_TEST_HOOK_WRITE
            ALLOY_UART_TEST_HOOK_WRITE(byte);
        #endif

        hw()->DR = byte;
    }

    /**
     * @brief Read single byte from data register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_READ
     */
    static inline uint8_t read_byte() const {
        #ifdef ALLOY_UART_TEST_HOOK_READ
            ALLOY_UART_TEST_HOOK_READ();
        #endif

        return static_cast<uint8_t>(hw()->DR & 0xFF);
    }

    /**
     * @brief Wait for TX ready with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_WAIT_TX
     */
    static inline bool wait_tx_ready(uint32_t timeout_loops) {
        #ifdef ALLOY_UART_TEST_HOOK_WAIT_TX
            ALLOY_UART_TEST_HOOK_WAIT_TX(timeout_loops);
        #endif

        uint32_t timeout = timeout_loops;
        while (!is_tx_ready() && --timeout);
        return timeout != 0;
    }

    /**
     * @brief Wait for RX ready with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_WAIT_RX
     */
    static inline bool wait_rx_ready(uint32_t timeout_loops) {
        #ifdef ALLOY_UART_TEST_HOOK_WAIT_RX
            ALLOY_UART_TEST_HOOK_WAIT_RX(timeout_loops);
        #endif

        uint32_t timeout = timeout_loops;
        while (!is_rx_ready() && --timeout);
        return timeout != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Usart1
using Usart1Hardware = Stm32f4UartHardwarePolicy<0x40011000, 84000000>;

/// @brief Hardware policy for Usart2
using Usart2Hardware = Stm32f4UartHardwarePolicy<0x40004400, 84000000>;

/// @brief Hardware policy for Usart3
using Usart3Hardware = Stm32f4UartHardwarePolicy<0x40004800, 84000000>;

/// @brief Hardware policy for Uart4
using Uart4Hardware = Stm32f4UartHardwarePolicy<0x40004C00, 84000000>;

/// @brief Hardware policy for Uart5
using Uart5Hardware = Stm32f4UartHardwarePolicy<0x40005000, 84000000>;

/// @brief Hardware policy for Usart6
using Usart6Hardware = Stm32f4UartHardwarePolicy<0x40011400, 84000000>;


}  // namespace alloy::hal::stm32f4

/**
 * @example
 * Using the hardware policy with generic UART API:
 *
 * @code
 * #include "hal/api/uart_simple.hpp"
 * #include "hal/vendors/atmel/same70/uart_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create UART with hardware policy
 * using Uart0 = UartImpl<PeripheralId::USART0, Uart0Hardware>;
 *
 * int main() {
 *     auto config = Uart0::quick_setup<TxPin, RxPin>(BaudRate{115200});
 *     config.initialize();
 *
 *     const char* msg = "Hello World\n";
 *     config.write(reinterpret_cast<const uint8_t*>(msg), 12);
 * }
 * @endcode
 */