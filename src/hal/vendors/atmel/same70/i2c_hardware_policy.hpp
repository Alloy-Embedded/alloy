/**
 * @file twihs_hardware_policy.hpp
 * @brief Hardware Policy for TWIHS on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for TWIHS using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_TWIHS_MOCK_HW)
 *
 * Auto-generated from: same70/twihs.json
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
#include "hal/vendors/atmel/same70/generated/registers/twihs0_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/generated/bitfields/twihs0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace twihs = alloy::hal::atmel::same70::twihs0;

/**
 * @brief Hardware Policy for TWIHS on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for TWIHS. It is designed to be used as a template
 * parameter in generic TWIHS implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: I2C peripheral base address
 * - IRQ_ID: I2C interrupt ID for clock enable
 *
 * @tparam BASE_ADDR I2C peripheral base address
 * @tparam IRQ_ID I2C interrupt ID for clock enable
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
struct Same70TWIHSHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = TWIHS0_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;
    static constexpr uint32_t I2C_TIMEOUT = 100000;  ///< I2C timeout in loop iterations (~10ms at 150MHz)

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
        #ifdef ALLOY_I2C_MOCK_HW
            return ALLOY_I2C_MOCK_HW();  // Test hook
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
     * @brief Software reset I2C peripheral
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RESET
     */
    static inline void reset() {
        #ifdef ALLOY_I2C_TEST_HOOK_RESET
            ALLOY_I2C_TEST_HOOK_RESET();
        #endif

        hw()->CR = twihs::cr::SWRST::mask;
    }

    /**
     * @brief Enable I2C in master mode
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_ENABLE
     */
    static inline void enable_master() {
        #ifdef ALLOY_I2C_TEST_HOOK_ENABLE
            ALLOY_I2C_TEST_HOOK_ENABLE();
        #endif

        hw()->CR = twihs::cr::MSEN::mask | twihs::cr::SVDIS::mask;
    }

    /**
     * @brief Disable I2C peripheral
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_DISABLE
     */
    static inline void disable() {
        #ifdef ALLOY_I2C_TEST_HOOK_DISABLE
            ALLOY_I2C_TEST_HOOK_DISABLE();
        #endif

        hw()->CR = twihs::cr::MSDIS::mask;
    }

    /**
     * @brief Configure I2C clock speed
     * @param speed_hz Desired clock speed in Hz
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_CLOCK
     */
    static inline void set_clock(uint32_t speed_hz) {
        #ifdef ALLOY_I2C_TEST_HOOK_CLOCK
            ALLOY_I2C_TEST_HOOK_CLOCK(speed_hz);
        #endif

        uint32_t ckdiv = 0;
uint32_t cldiv = (PERIPH_CLOCK_HZ / (2 * speed_hz)) - 4;
while (cldiv > 255 && ckdiv < 7) {
    ckdiv++;
    cldiv = (PERIPH_CLOCK_HZ / (2 * speed_hz * (1 << ckdiv))) - 4;
}
uint32_t cwgr = twihs::cwgr::CLDIV::write(0, cldiv);
cwgr = twihs::cwgr::CHDIV::write(cwgr, cldiv);
cwgr = twihs::cwgr::CKDIV::write(cwgr, ckdiv);
hw()->CWGR = cwgr;
    }

    /**
     * @brief Start I2C write transaction
     * @param device_addr 7-bit device address
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_START_WRITE
     */
    static inline void start_write(uint8_t device_addr) {
        #ifdef ALLOY_I2C_TEST_HOOK_START_WRITE
            ALLOY_I2C_TEST_HOOK_START_WRITE(device_addr);
        #endif

        uint32_t mmr = 0;
mmr = twihs::mmr::DADR::write(mmr, device_addr);
mmr = twihs::mmr::MREAD::clear(mmr);
hw()->MMR = mmr;
    }

    /**
     * @brief Start I2C read transaction
     * @param device_addr 7-bit device address
     * @param read_count Number of bytes to read
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_START_READ
     */
    static inline void start_read(uint8_t device_addr, uint8_t read_count) {
        #ifdef ALLOY_I2C_TEST_HOOK_START_READ
            ALLOY_I2C_TEST_HOOK_START_READ(device_addr, read_count);
        #endif

        uint32_t mmr = 0;
mmr = twihs::mmr::DADR::write(mmr, device_addr);
mmr = twihs::mmr::MREAD::set(mmr);
hw()->MMR = mmr;
if (read_count == 1) {
    hw()->CR = twihs::cr::START::mask | twihs::cr::STOP::mask;
} else {
    hw()->CR = twihs::cr::START::mask;
}
    }

    /**
     * @brief Send STOP condition
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_STOP
     */
    static inline void send_stop() {
        #ifdef ALLOY_I2C_TEST_HOOK_STOP
            ALLOY_I2C_TEST_HOOK_STOP();
        #endif

        hw()->CR = twihs::cr::STOP::mask;
    }

    /**
     * @brief Check if TX buffer is ready
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TX_READY
     */
    static inline bool is_tx_ready() const {
        #ifdef ALLOY_I2C_TEST_HOOK_TX_READY
            ALLOY_I2C_TEST_HOOK_TX_READY();
        #endif

        return (hw()->SR & twihs::sr::TXRDY::mask) != 0;
    }

    /**
     * @brief Check if RX data is ready
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RX_READY
     */
    static inline bool is_rx_ready() const {
        #ifdef ALLOY_I2C_TEST_HOOK_RX_READY
            ALLOY_I2C_TEST_HOOK_RX_READY();
        #endif

        return (hw()->SR & twihs::sr::RXRDY::mask) != 0;
    }

    /**
     * @brief Check if transmission is complete
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TX_COMPLETE
     */
    static inline bool is_tx_complete() const {
        #ifdef ALLOY_I2C_TEST_HOOK_TX_COMPLETE
            ALLOY_I2C_TEST_HOOK_TX_COMPLETE();
        #endif

        return (hw()->SR & twihs::sr::TXCOMP::mask) != 0;
    }

    /**
     * @brief Check if NACK was received
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_NACK
     */
    static inline bool has_nack() const {
        #ifdef ALLOY_I2C_TEST_HOOK_NACK
            ALLOY_I2C_TEST_HOOK_NACK();
        #endif

        return (hw()->SR & twihs::sr::NACK::mask) != 0;
    }

    /**
     * @brief Write byte to TX register
     * @param byte Byte to write
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_WRITE
     */
    static inline void write_byte(uint8_t byte) {
        #ifdef ALLOY_I2C_TEST_HOOK_WRITE
            ALLOY_I2C_TEST_HOOK_WRITE(byte);
        #endif

        hw()->THR = byte;
    }

    /**
     * @brief Read byte from RX register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_READ
     */
    static inline uint8_t read_byte() const {
        #ifdef ALLOY_I2C_TEST_HOOK_READ
            ALLOY_I2C_TEST_HOOK_READ();
        #endif

        return static_cast<uint8_t>(hw()->RHR);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for I2c0
using I2c0Hardware = Same70TWIHSHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TWIHS0, >;
/// @brief Hardware policy for I2c1
using I2c1Hardware = Same70TWIHSHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TWIHS1, >;
/// @brief Hardware policy for I2c2
using I2c2Hardware = Same70TWIHSHardwarePolicy<alloy::generated::atsame70q21b::peripherals::TWIHS2, >;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic TWIHS API:
 *
 * @code
 * #include "hal/api/twihs_simple.hpp"
 * #include "hal/platform/same70/twihs.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create TWIHS with hardware policy
 * using Instance0 = I2c0Hardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */