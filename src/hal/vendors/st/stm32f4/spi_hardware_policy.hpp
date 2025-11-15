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
 * Auto-generated from: stm32f4/spi.json
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
#include "hal/vendors/st/stm32f4/generated/registers/spi1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f4/generated/bitfields/spi1_bitfields.hpp"

namespace alloy::hal::stm32f4 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfields
namespace spi = st::stm32f4::spi1;

/**
 * @brief Hardware Policy for SPI on STM32F4
 *
 * This policy provides all platform-specific hardware access methods
 * for SPI. It is designed to be used as a template
 * parameter in generic SPI implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: SPI peripheral base address
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

    using RegisterType = SPI1_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t SPI_TIMEOUT = 100000;  ///< SPI timeout in loop iterations

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
        #ifdef ALLOY_SPI_MOCK_HW
            return ALLOY_SPI_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Reset SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_SPI_TEST_HOOK_RESET
            ALLOY_SPI_TEST_HOOK_RESET();
        #endif

        hw()->CR1 &= ~spi::cr1::SPE::mask;
    }

    /**
     * @brief Enable SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_ENABLE
     */
    static inline void enable() {
        #ifdef ALLOY_SPI_TEST_HOOK_ENABLE
            ALLOY_SPI_TEST_HOOK_ENABLE();
        #endif

        hw()->CR1 |= spi::cr1::SPE::mask;
    }

    /**
     * @brief Disable SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_DISABLE
     */
    static inline void disable() {
        #ifdef ALLOY_SPI_TEST_HOOK_DISABLE
            ALLOY_SPI_TEST_HOOK_DISABLE();
        #endif

        hw()->CR1 &= ~spi::cr1::SPE::mask;
    }

    /**
     * @brief Configure SPI in master mode
     *
     * @param clock_div Clock divider
     * @param mode SPI mode (0-3)
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_MASTER
     */
    static inline void configure_master(uint8_t clock_div, uint8_t mode) {
        #ifdef ALLOY_SPI_TEST_HOOK_MASTER
            ALLOY_SPI_TEST_HOOK_MASTER(clock_div, mode);
        #endif

        uint32_t cr1 = spi::cr1::MSTR::mask | spi::cr1::SSM::mask | spi::cr1::SSI::mask | spi::cr1::BR::write(0, clock_div); if (mode & 0x01) cr1 |= spi::cr1::CPHA::mask; if (mode & 0x02) cr1 |= spi::cr1::CPOL::mask; hw()->CR1 = cr1;
    }

    /**
     * @brief Check if TX buffer is empty
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_TX_READY
     */
    static inline bool is_tx_ready() const {
        #ifdef ALLOY_SPI_TEST_HOOK_TX_READY
            ALLOY_SPI_TEST_HOOK_TX_READY();
        #endif

        return (hw()->SR & spi::sr::TXE::mask) != 0;
    }

    /**
     * @brief Check if RX buffer has data
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RX_READY
     */
    static inline bool is_rx_ready() const {
        #ifdef ALLOY_SPI_TEST_HOOK_RX_READY
            ALLOY_SPI_TEST_HOOK_RX_READY();
        #endif

        return (hw()->SR & spi::sr::RXNE::mask) != 0;
    }

    /**
     * @brief Write byte to data register
     *
     * @param byte Byte to transmit
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_WRITE
     */
    static inline void write_byte(uint8_t byte) {
        #ifdef ALLOY_SPI_TEST_HOOK_WRITE
            ALLOY_SPI_TEST_HOOK_WRITE(byte);
        #endif

        hw()->DR = byte;
    }

    /**
     * @brief Read byte from data register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_READ
     */
    static inline uint8_t read_byte() const {
        #ifdef ALLOY_SPI_TEST_HOOK_READ
            ALLOY_SPI_TEST_HOOK_READ();
        #endif

        return static_cast<uint8_t>(hw()->DR & 0xFF);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Spi1
using Spi1Hardware = Stm32f4UartHardwarePolicy<0x40013000, 84000000>;

/// @brief Hardware policy for Spi2
using Spi2Hardware = Stm32f4UartHardwarePolicy<0x40003800, 84000000>;

/// @brief Hardware policy for Spi3
using Spi3Hardware = Stm32f4UartHardwarePolicy<0x40003C00, 84000000>;


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