/**
 * @file dma_hardware_policy.hpp
 * @brief Hardware Policy for DMA on SAME70 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for DMA using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_DMA_MOCK_HW)
 *
 * Auto-generated from: same70/dma.json
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
#include "hal/vendors/atmel/same70/generated/registers/xdmac_registers.hpp"

// Bitfield definitions
#include "hal/vendors/atmel/same70/generated/bitfields/xdmac_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfields
namespace xdmac = alloy::hal::atmel::same70::xdmac;

/**
 * @brief Hardware Policy for DMA on SAME70
 *
 * This policy provides all platform-specific hardware access methods
 * for DMA. It is designed to be used as a template
 * parameter in generic DMA implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: XDMAC peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency
 *
 * @tparam BASE_ADDR XDMAC peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Same70DMAHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = xdmac::XDMAC_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t periph_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint8_t NUM_CHANNELS = 24;  ///< Number of DMA channels

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
        #ifdef ALLOY_DMA_MOCK_HW
            return ALLOY_DMA_MOCK_HW();  // Test hook
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
     * @brief Enable DMA channel
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_ENABLE
     */
    static inline void enable_channel(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_ENABLE
            ALLOY_DMA_TEST_HOOK_ENABLE(channel);
        #endif

        hw()->GE = (1u << channel);
    }

    /**
     * @brief Disable DMA channel
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_DISABLE
     */
    static inline void disable_channel(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_DISABLE
            ALLOY_DMA_TEST_HOOK_DISABLE(channel);
        #endif

        hw()->GD = (1u << channel);
    }

    /**
     * @brief Suspend DMA channel
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_SUSPEND
     */
    static inline void suspend_channel(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_SUSPEND
            ALLOY_DMA_TEST_HOOK_SUSPEND(channel);
        #endif

        hw()->GRWS = (1u << channel);
    }

    /**
     * @brief Resume DMA channel
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_RESUME
     */
    static inline void resume_channel(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_RESUME
            ALLOY_DMA_TEST_HOOK_RESUME(channel);
        #endif

        hw()->GRWR = (1u << channel);
    }

    /**
     * @brief Flush DMA channel FIFO
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_FLUSH
     */
    static inline void flush_channel(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_FLUSH
            ALLOY_DMA_TEST_HOOK_FLUSH(channel);
        #endif

        hw()->GWS = (1u << channel);
    }

    /**
     * @brief Set DMA source address
     * @param channel Channel number (0-23)
     * @param address Source address
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_SET_SRC
     */
    static inline void set_source_address(uint8_t channel, uint32_t address) {
        #ifdef ALLOY_DMA_TEST_HOOK_SET_SRC
            ALLOY_DMA_TEST_HOOK_SET_SRC(channel, address);
        #endif

        if (channel < 24) { volatile uint32_t* csa = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x50 + (channel * 0x40) + 0x00); *csa = address; }
    }

    /**
     * @brief Set DMA destination address
     * @param channel Channel number (0-23)
     * @param address Destination address
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_SET_DST
     */
    static inline void set_destination_address(uint8_t channel, uint32_t address) {
        #ifdef ALLOY_DMA_TEST_HOOK_SET_DST
            ALLOY_DMA_TEST_HOOK_SET_DST(channel, address);
        #endif

        if (channel < 24) { volatile uint32_t* cda = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x50 + (channel * 0x40) + 0x04); *cda = address; }
    }

    /**
     * @brief Set DMA transfer size
     * @param channel Channel number (0-23)
     * @param size Number of transfers
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_SET_SIZE
     */
    static inline void set_transfer_size(uint8_t channel, uint32_t size) {
        #ifdef ALLOY_DMA_TEST_HOOK_SET_SIZE
            ALLOY_DMA_TEST_HOOK_SET_SIZE(channel, size);
        #endif

        if (channel < 24) { volatile uint32_t* cubc = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x50 + (channel * 0x40) + 0x10); *cubc = size & 0xFFFFFF; }
    }

    /**
     * @brief Configure DMA channel
     * @param channel Channel number (0-23)
     * @param config Configuration value
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_CONFIG
     */
    static inline void configure_channel(uint8_t channel, uint32_t config) {
        #ifdef ALLOY_DMA_TEST_HOOK_CONFIG
            ALLOY_DMA_TEST_HOOK_CONFIG(channel, config);
        #endif

        if (channel < 24) { volatile uint32_t* cc = reinterpret_cast<volatile uint32_t*>(BASE_ADDR + 0x50 + (channel * 0x40) + 0x08); *cc = config; }
    }

    /**
     * @brief Check if DMA transfer is complete
     * @param channel Channel number (0-23)
     * @return bool
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_IS_DONE
     */
    static inline bool is_transfer_complete(uint8_t channel) const {
        #ifdef ALLOY_DMA_TEST_HOOK_IS_DONE
            ALLOY_DMA_TEST_HOOK_IS_DONE(channel);
        #endif

        return (hw()->GIS & (1u << channel)) == 0;
    }

    /**
     * @brief Check if DMA channel is busy
     * @param channel Channel number (0-23)
     * @return bool
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_IS_BUSY
     */
    static inline bool is_busy(uint8_t channel) const {
        #ifdef ALLOY_DMA_TEST_HOOK_IS_BUSY
            ALLOY_DMA_TEST_HOOK_IS_BUSY(channel);
        #endif

        return (hw()->GS & (1u << channel)) != 0;
    }

    /**
     * @brief Enable DMA channel interrupt
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_INT_EN
     */
    static inline void enable_interrupt(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_INT_EN
            ALLOY_DMA_TEST_HOOK_INT_EN(channel);
        #endif

        hw()->GIE = (1u << channel);
    }

    /**
     * @brief Disable DMA channel interrupt
     * @param channel Channel number (0-23)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_INT_DIS
     */
    static inline void disable_interrupt(uint8_t channel) {
        #ifdef ALLOY_DMA_TEST_HOOK_INT_DIS
            ALLOY_DMA_TEST_HOOK_INT_DIS(channel);
        #endif

        hw()->GID = (1u << channel);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================

/// @brief Hardware policy for Dma
using DmaHardware = Same70DMAHardwarePolicy<alloy::generated::atsame70q21b::peripherals::XDMAC, 150000000>;

}  // namespace alloy::hal::same70

/**
 * @example
 * Using the hardware policy with generic DMA API:
 *
 * @code
 * #include "hal/api/dma_simple.hpp"
 * #include "hal/platform/same70/dma.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::same70;
 *
 * // Create DMA with hardware policy
 * using Instance0 = DmaHardware;
 *
 * int main() {
 *     Instance0::reset();
 *     // Use other policy methods...
 * }
 * @endcode
 */