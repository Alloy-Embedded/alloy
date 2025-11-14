/**
 * @file uart_hardware_policy.hpp
 * @brief Hardware Policy for UART on STM32G0 (Policy-Based Design)
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
 * Auto-generated from: stm32g0/uart.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-14 17:36:48
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
#include "hal/vendors/st/stm32g0/registers/usart1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/bitfields/usart1_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::usart1;

/**
 * @brief Hardware Policy for UART on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for UART. It is designed to be used as a template
 * parameter in generic UART implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0UARTHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = USART1_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;

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
     * @brief Get pointer to hardware registers
     * @return volatile RegisterType*
     */
    static inline volatile RegisterType* hw_accessor() {
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    }

    /**
     * @brief Enable UART peripheral
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_UE
     */
    static inline void enable_uart() {
        #ifdef ALLOY_UART_TEST_HOOK_UE
            ALLOY_UART_TEST_HOOK_UE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 0);
    }

    /**
     * @brief Disable UART peripheral
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_UE
     */
    static inline void disable_uart() {
        #ifdef ALLOY_UART_TEST_HOOK_UE
            ALLOY_UART_TEST_HOOK_UE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 0);
    }

    /**
     * @brief Enable transmitter
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TE
     */
    static inline void enable_transmitter() {
        #ifdef ALLOY_UART_TEST_HOOK_TE
            ALLOY_UART_TEST_HOOK_TE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 3);
    }

    /**
     * @brief Enable receiver
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RE
     */
    static inline void enable_receiver() {
        #ifdef ALLOY_UART_TEST_HOOK_RE
            ALLOY_UART_TEST_HOOK_RE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 2);
    }

    /**
     * @brief Disable transmitter
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TE
     */
    static inline void disable_transmitter() {
        #ifdef ALLOY_UART_TEST_HOOK_TE
            ALLOY_UART_TEST_HOOK_TE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 3);
    }

    /**
     * @brief Disable receiver
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RE
     */
    static inline void disable_receiver() {
        #ifdef ALLOY_UART_TEST_HOOK_RE
            ALLOY_UART_TEST_HOOK_RE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 2);
    }

    /**
     * @brief Set baud rate
     * @param baudrate Desired baud rate
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_BRR
     */
    static inline void set_baudrate(uint32_t baudrate) {
        #ifdef ALLOY_UART_TEST_HOOK_BRR
            ALLOY_UART_TEST_HOOK_BRR(baudrate);
        #endif

        uint32_t usartdiv = (PERIPH_CLOCK_HZ + baudrate / 2) / baudrate;
hw()->BRR = usartdiv;
    }

    /**
     * @brief Set word length to 8 bits
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_M
     */
    static inline void set_word_length_8bit() {
        #ifdef ALLOY_UART_TEST_HOOK_M
            ALLOY_UART_TEST_HOOK_M();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 28);
hw()->CR1_FIFO_DISABLED &= ~(1U << 12);
    }

    /**
     * @brief Set word length to 9 bits
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_M
     */
    static inline void set_word_length_9bit() {
        #ifdef ALLOY_UART_TEST_HOOK_M
            ALLOY_UART_TEST_HOOK_M();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 28);
hw()->CR1_FIFO_DISABLED |= (1U << 12);
    }

    /**
     * @brief Disable parity
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_PCE
     */
    static inline void set_parity_none() {
        #ifdef ALLOY_UART_TEST_HOOK_PCE
            ALLOY_UART_TEST_HOOK_PCE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 10);
    }

    /**
     * @brief Enable even parity
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_PCE
     */
    static inline void set_parity_even() {
        #ifdef ALLOY_UART_TEST_HOOK_PCE
            ALLOY_UART_TEST_HOOK_PCE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 10);
hw()->CR1_FIFO_DISABLED &= ~(1U << 9);
    }

    /**
     * @brief Enable odd parity
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_PCE
     */
    static inline void set_parity_odd() {
        #ifdef ALLOY_UART_TEST_HOOK_PCE
            ALLOY_UART_TEST_HOOK_PCE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 10);
hw()->CR1_FIFO_DISABLED |= (1U << 9);
    }

    /**
     * @brief Set 1 stop bit
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_STOP
     */
    static inline void set_stop_bits_1() {
        #ifdef ALLOY_UART_TEST_HOOK_STOP
            ALLOY_UART_TEST_HOOK_STOP();
        #endif

        hw()->CR2 &= ~(0x3U << 12);
    }

    /**
     * @brief Set 2 stop bits
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_STOP
     */
    static inline void set_stop_bits_2() {
        #ifdef ALLOY_UART_TEST_HOOK_STOP
            ALLOY_UART_TEST_HOOK_STOP();
        #endif

        hw()->CR2 = (hw()->CR2 & ~(0x3U << 12)) | (0x2U << 12);
    }

    /**
     * @brief Write data to transmit register
     * @param data Data byte to transmit
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TDR
     */
    static inline void write_data(uint8_t data) {
        #ifdef ALLOY_UART_TEST_HOOK_TDR
            ALLOY_UART_TEST_HOOK_TDR(data);
        #endif

        hw()->TDR = data;
    }

    /**
     * @brief Read data from receive register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RDR
     */
    static inline uint8_t read_data() {
        #ifdef ALLOY_UART_TEST_HOOK_RDR
            ALLOY_UART_TEST_HOOK_RDR();
        #endif

        return static_cast<uint8_t>(hw()->RDR & 0xFF);
    }

    /**
     * @brief Check if transmit register is empty
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TXE
     */
    static inline bool is_tx_empty() {
        #ifdef ALLOY_UART_TEST_HOOK_TXE
            ALLOY_UART_TEST_HOOK_TXE();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 7)) != 0;
    }

    /**
     * @brief Check if transmission is complete
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TC
     */
    static inline bool is_tx_complete() {
        #ifdef ALLOY_UART_TEST_HOOK_TC
            ALLOY_UART_TEST_HOOK_TC();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 6)) != 0;
    }

    /**
     * @brief Check if receive register has data
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RXNE
     */
    static inline bool is_rx_not_empty() {
        #ifdef ALLOY_UART_TEST_HOOK_RXNE
            ALLOY_UART_TEST_HOOK_RXNE();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 5)) != 0;
    }

    /**
     * @brief Clear overrun error flag
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_ORECF
     */
    static inline void clear_overrun_error() {
        #ifdef ALLOY_UART_TEST_HOOK_ORECF
            ALLOY_UART_TEST_HOOK_ORECF();
        #endif

        hw()->ICR = (1U << 3);
    }

    /**
     * @brief Check for overrun error
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_ORE
     */
    static inline bool has_overrun_error() {
        #ifdef ALLOY_UART_TEST_HOOK_ORE
            ALLOY_UART_TEST_HOOK_ORE();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 3)) != 0;
    }

    /**
     * @brief Check for framing error
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_FE
     */
    static inline bool has_framing_error() {
        #ifdef ALLOY_UART_TEST_HOOK_FE
            ALLOY_UART_TEST_HOOK_FE();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 1)) != 0;
    }

    /**
     * @brief Check for parity error
     * @return bool
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_PE
     */
    static inline bool has_parity_error() {
        #ifdef ALLOY_UART_TEST_HOOK_PE
            ALLOY_UART_TEST_HOOK_PE();
        #endif

        return (hw()->ISR_FIFO_DISABLED & (1U << 0)) != 0;
    }

    /**
     * @brief Enable receive interrupt
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RXNEIE
     */
    static inline void enable_rx_interrupt() {
        #ifdef ALLOY_UART_TEST_HOOK_RXNEIE
            ALLOY_UART_TEST_HOOK_RXNEIE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 5);
    }

    /**
     * @brief Enable transmit interrupt
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TXEIE
     */
    static inline void enable_tx_interrupt() {
        #ifdef ALLOY_UART_TEST_HOOK_TXEIE
            ALLOY_UART_TEST_HOOK_TXEIE();
        #endif

        hw()->CR1_FIFO_DISABLED |= (1U << 7);
    }

    /**
     * @brief Disable receive interrupt
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_RXNEIE
     */
    static inline void disable_rx_interrupt() {
        #ifdef ALLOY_UART_TEST_HOOK_RXNEIE
            ALLOY_UART_TEST_HOOK_RXNEIE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 5);
    }

    /**
     * @brief Disable transmit interrupt
     *
     * @note Test hook: ALLOY_UART_TEST_HOOK_TXEIE
     */
    static inline void disable_tx_interrupt() {
        #ifdef ALLOY_UART_TEST_HOOK_TXEIE
            ALLOY_UART_TEST_HOOK_TXEIE();
        #endif

        hw()->CR1_FIFO_DISABLED &= ~(1U << 7);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic UART API:
 *
 * @code
 * #include "hal/api/uart_simple.hpp"
 * #include "hal/vendors/st/stm32g0/uart_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create UART with hardware policy
 * using Uart0 = UartImpl<Stm32g0UARTHardwarePolicy<UART0_BASE, 150000000>>;
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