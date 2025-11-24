/**
 * @file spi_hardware_policy.hpp
 * @brief Hardware Policy for SPI on STM32F7 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for SPI using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F7 SPI Features:
 * - Compatible with STM32F4 register layout
 * - Higher clock speeds (up to 108 MHz APB2)
 * - Multiple SPI peripherals (SPI1-6)
 * - Full-duplex synchronous communication
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef MICROCORE_SPI_MOCK_HW)
 *
 * @note Part of Phase 3.3: SPI Implementation
 * @see docs/API_TIERS.md
 */

#pragma once

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// Register definitions (STM32F7 uses same SPI register layout as F4)
#include "hal/vendors/st/stm32f7/generated/registers/spi5_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f7/generated/bitfields/spi5_bitfields.hpp"

namespace ucore::hal::stm32f7 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f7;

// Namespace alias for bitfields
namespace spi = st::stm32f7::spi5;

/**
 * @brief Hardware Policy for SPI on STM32F7
 *
 * This policy provides all platform-specific hardware access methods
 * for SPI. It is designed to be used as a template parameter in
 * generic SPI implementations.
 *
 * The STM32F7 SPI peripheral is register-compatible with STM32F4,
 * but supports higher clock speeds (up to 108 MHz on APB2).
 *
 * Template Parameters:
 * - BASE_ADDR: SPI peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz (APB1/APB2)
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using Spi1 = SpiImpl<Stm32f7SpiHardwarePolicy<0x40013000, 108000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for baud rate calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f7SpiHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = SPI5_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t SPI_TIMEOUT =
        100000;  ///< SPI timeout in loop iterations (~10ms at 216MHz)

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
#ifdef MICROCORE_SPI_MOCK_HW
        return MICROCORE_SPI_MOCK_HW();  // Test hook
#else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
#endif
    }

    // ========================================================================
    // Hardware Policy Methods
    // ========================================================================

    /**
     * @brief Reset SPI peripheral (disable)
     */
    static inline void reset() {
        hw()->CR1 &= ~spi::cr1::SPE::mask;
    }

    /**
     * @brief Enable SPI peripheral
     */
    static inline void enable() {
        hw()->CR1 |= spi::cr1::SPE::mask;
    }

    /**
     * @brief Disable SPI peripheral
     */
    static inline void disable() {
        hw()->CR1 &= ~spi::cr1::SPE::mask;
    }

    /**
     * @brief Configure SPI in master mode
     *
     * SPI modes:
     * - Mode 0: CPOL=0, CPHA=0
     * - Mode 1: CPOL=0, CPHA=1
     * - Mode 2: CPOL=1, CPHA=0
     * - Mode 3: CPOL=1, CPHA=1
     *
     * @param clock_div Clock divider (0-7: /2, /4, /8, /16, /32, /64, /128, /256)
     * @param mode SPI mode (0-3)
     */
    static inline void configure_master(uint8_t clock_div, uint8_t mode) {
        // Build CR1 configuration
        uint32_t cr1 = 0;

        // Master mode, software slave management, internal slave select
        cr1 |= spi::cr1::MSTR::mask;  // Master mode
        cr1 |= spi::cr1::SSM::mask;   // Software slave management
        cr1 |= spi::cr1::SSI::mask;   // Internal slave select high

        // Baud rate
        cr1 = spi::cr1::BR::write(cr1, clock_div & 0x7);

        // SPI mode (CPOL and CPHA)
        if (mode & 0x01) {  // CPHA
            cr1 |= spi::cr1::CPHA::mask;
        }
        if (mode & 0x02) {  // CPOL
            cr1 |= spi::cr1::CPOL::mask;
        }

        hw()->CR1 = cr1;
    }

    /**
     * @brief Check if transmit buffer is empty (TXE flag)
     *
     * @return true if TX buffer is empty and ready for new data
     */
    static inline bool is_tx_ready() {
        return (hw()->SR & spi::sr::TXE::mask) != 0;
    }

    /**
     * @brief Check if receive buffer is not empty (RXNE flag)
     *
     * @return true if RX buffer contains valid received data
     */
    static inline bool is_rx_ready() {
        return (hw()->SR & spi::sr::RXNE::mask) != 0;
    }

    /**
     * @brief Write single byte to data register
     *
     * @param byte Byte to write
     *
     * @note Caller must check is_tx_ready() before calling
     */
    static inline void write_byte(uint8_t byte) {
        hw()->DR = byte;
    }

    /**
     * @brief Read single byte from data register
     *
     * @return Received byte
     *
     * @note Caller must check is_rx_ready() before calling
     */
    static inline uint8_t read_byte() {
        return static_cast<uint8_t>(hw()->DR & 0xFF);
    }

    /**
     * @brief Wait for TX ready with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return true if TX became ready, false if timeout
     */
    static inline bool wait_tx_ready(uint32_t timeout_loops = SPI_TIMEOUT) {
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
    static inline bool wait_rx_ready(uint32_t timeout_loops = SPI_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (!is_rx_ready() && --timeout)
            ;
        return timeout != 0;
    }

    /**
     * @brief Check if SPI is busy (BSY flag)
     *
     * @return true if SPI is busy transmitting/receiving
     */
    static inline bool is_busy() {
        return (hw()->SR & spi::sr::BSY::mask) != 0;
    }

    /**
     * @brief Wait for SPI to become not busy with timeout
     *
     * @param timeout_loops Timeout in loop iterations
     * @return true if SPI became not busy, false if timeout
     */
    static inline bool wait_not_busy(uint32_t timeout_loops = SPI_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (is_busy() && --timeout)
            ;
        return timeout != 0;
    }
};

}  // namespace ucore::hal::stm32f7

/**
 * @example STM32F7 SPI Example
 * @code
 * #include "hal/platform/stm32f7/spi.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto spi = Spi1::quick_setup<MosiPin, MisoPin, SckPin>(
 *         SpiMode::Mode0, SpiSpeed::Medium);
 *     spi.initialize();
 *
 *     uint8_t tx_data[] = {0x01, 0x02, 0x03};
 *     uint8_t rx_data[3];
 *     spi.transfer(tx_data, rx_data, 3);
 * }
 * @endcode
 */
