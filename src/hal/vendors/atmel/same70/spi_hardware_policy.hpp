/**
 * @file spi_hardware_policy.hpp
 * @brief Hardware Policy for SPI on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for SPI using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_SPI_MOCK_HW)
 *
 * Auto-generated from: same70/spi.json
 * Generator: hardware_policy_generator.py
 * Generated: 2025-11-14 09:58:28
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
#include "hal/vendors/atmel/same70/registers/spi0_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/bitfields/spi0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace spi = atmel::same70::spi0;

/**
 * @brief Hardware Policy for SPI on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for SPI. It is designed to be used as a template
 * parameter in generic SPI implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: SPI peripheral base address
 * - IRQ_ID: SPI interrupt ID for clock enable
 *
 * @tparam BASE_ADDR SPI peripheral base address
 * @tparam IRQ_ID SPI interrupt ID for clock enable
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
struct Same70SPIHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = SPI0_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;
    static constexpr uint32_t SPI_TIMEOUT = 100000;  ///< SPI timeout in loop iterations (~10ms at 150MHz)
    static constexpr size_t STACK_BUFFER_SIZE = 256;  ///< Stack buffer size for dummy data in write/read operations

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
     * @brief Get pointer to hardware registers
     * @return volatile RegisterType*
     */
    static inline volatile RegisterType* hw_accessor() {
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
    }

    /**
     * @brief Reset SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_SPI_TEST_HOOK_RESET
            ALLOY_SPI_TEST_HOOK_RESET();
        #endif

        hw()->CR = spi::cr::SWRST::mask;
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

        hw()->CR = spi::cr::SPIEN::mask;
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

        hw()->CR = spi::cr::SPIDIS::mask;
    }

    /**
     * @brief Configure SPI in master mode
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_MASTER
     */
    static inline void configure_master() {
        #ifdef ALLOY_SPI_TEST_HOOK_MASTER
            ALLOY_SPI_TEST_HOOK_MASTER();
        #endif

        hw()->MR = spi::mr::MSTR::mask | spi::mr::MODFDIS::mask;
    }

    /**
     * @brief Configure chip select parameters
     * @param cs Chip select number (0-3)
     * @param scbr Serial Clock Baud Rate divider
     * @param mode SPI mode (0-3)
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_CS_CONFIG
     */
    static inline void configure_chip_select(uint8_t cs, uint32_t scbr, uint8_t mode) {
        #ifdef ALLOY_SPI_TEST_HOOK_CS_CONFIG
            ALLOY_SPI_TEST_HOOK_CS_CONFIG(cs, scbr, mode);
        #endif

        uint32_t csr_value = spi::csr::SCBR::write(0, scbr);
if (mode & 0x01) csr_value |= spi::csr::NCPHA::mask;
if (mode & 0x02) csr_value |= spi::csr::CPOL::mask;
hw()->CSR[cs] = csr_value;
    }

    /**
     * @brief Select chip (assert CS)
     * @param cs Chip select number (0-3)
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_SELECT
     */
    static inline void select_chip(uint8_t cs) {
        #ifdef ALLOY_SPI_TEST_HOOK_SELECT
            ALLOY_SPI_TEST_HOOK_SELECT(cs);
        #endif

        hw()->MR = (hw()->MR & ~spi::mr::PCS::mask) | spi::mr::PCS::write(0, ~(1u << cs));
    }

    /**
     * @brief Check if transmit buffer is ready
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_TX_READY
     */
    static inline bool is_tx_ready() const {
        #ifdef ALLOY_SPI_TEST_HOOK_TX_READY
            ALLOY_SPI_TEST_HOOK_TX_READY();
        #endif

        return (hw()->SR & spi::sr::TDRE::mask) != 0;
    }

    /**
     * @brief Check if receive data is ready
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RX_READY
     */
    static inline bool is_rx_ready() const {
        #ifdef ALLOY_SPI_TEST_HOOK_RX_READY
            ALLOY_SPI_TEST_HOOK_RX_READY();
        #endif

        return (hw()->SR & spi::sr::RDRF::mask) != 0;
    }

    /**
     * @brief Write byte to transmit register
     * @param byte Byte to transmit
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_WRITE
     */
    static inline void write_byte(uint8_t byte) {
        #ifdef ALLOY_SPI_TEST_HOOK_WRITE
            ALLOY_SPI_TEST_HOOK_WRITE(byte);
        #endif

        hw()->TDR = byte;
    }

    /**
     * @brief Read byte from receive register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_READ
     */
    static inline uint8_t read_byte() const {
        #ifdef ALLOY_SPI_TEST_HOOK_READ
            ALLOY_SPI_TEST_HOOK_READ();
        #endif

        return static_cast<uint8_t>(hw()->RDR & 0xFF);
    }

    /**
     * @brief Wait for TX ready with timeout
     * @param timeout_loops Timeout in loop iterations
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_WAIT_TX
     */
    static inline bool wait_tx_ready(uint32_t timeout_loops) {
        #ifdef ALLOY_SPI_TEST_HOOK_WAIT_TX
            ALLOY_SPI_TEST_HOOK_WAIT_TX(timeout_loops);
        #endif

        uint32_t timeout = timeout_loops;
while (!is_tx_ready() && --timeout);
return timeout != 0;
    }

    /**
     * @brief Wait for RX ready with timeout
     * @param timeout_loops Timeout in loop iterations
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_WAIT_RX
     */
    static inline bool wait_rx_ready(uint32_t timeout_loops) {
        #ifdef ALLOY_SPI_TEST_HOOK_WAIT_RX
            ALLOY_SPI_TEST_HOOK_WAIT_RX(timeout_loops);
        #endif

        uint32_t timeout = timeout_loops;
while (!is_rx_ready() && --timeout);
return timeout != 0;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Spi0
using Spi0Hardware = Same70SPIHardwarePolicy<0x40008000, >;
/// @brief Hardware policy for Spi1
using Spi1Hardware = Same70SPIHardwarePolicy<0x40058000, >;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic SPI API:
 *
 * @code
 * #include "hal/api/spi_simple.hpp"
 * #include "hal/platform/same70/spi.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create SPI with hardware policy
 * using Instance0 = Spi0Hardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */