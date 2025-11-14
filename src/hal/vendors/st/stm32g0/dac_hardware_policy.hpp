/**
 * @file dac_hardware_policy.hpp
 * @brief Hardware Policy for DAC on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for DAC using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_DAC_MOCK_HW)
 *
 * Auto-generated from: stm32g0/dac.json
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
#include "hal/vendors/st/stm32g0/registers/dac_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/bitfields/dac_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::dac;

/**
 * @brief Hardware Policy for DAC on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for DAC. It is designed to be used as a template
 * parameter in generic DAC implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0DACHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = DAC_Registers;

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
        #ifdef ALLOY_DAC_MOCK_HW
            return ALLOY_DAC_MOCK_HW();  // Test hook
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
     * @brief Enable DAC channel 1
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_EN1
     */
    static inline void enable_channel_1() {
        #ifdef ALLOY_DAC_TEST_HOOK_EN1
            ALLOY_DAC_TEST_HOOK_EN1();
        #endif

        hw()->DAC_CR |= (1U << 0);
    }

    /**
     * @brief Disable DAC channel 1
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_EN1
     */
    static inline void disable_channel_1() {
        #ifdef ALLOY_DAC_TEST_HOOK_EN1
            ALLOY_DAC_TEST_HOOK_EN1();
        #endif

        hw()->DAC_CR &= ~(1U << 0);
    }

    /**
     * @brief Enable DAC channel 2
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_EN2
     */
    static inline void enable_channel_2() {
        #ifdef ALLOY_DAC_TEST_HOOK_EN2
            ALLOY_DAC_TEST_HOOK_EN2();
        #endif

        hw()->DAC_CR |= (1U << 16);
    }

    /**
     * @brief Disable DAC channel 2
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_EN2
     */
    static inline void disable_channel_2() {
        #ifdef ALLOY_DAC_TEST_HOOK_EN2
            ALLOY_DAC_TEST_HOOK_EN2();
        #endif

        hw()->DAC_CR &= ~(1U << 16);
    }

    /**
     * @brief Set 12-bit data for channel 1 (right-aligned)
     * @param data 12-bit data value
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DHR12R1
     */
    static inline void set_data_channel_1_12bit(uint16_t data) {
        #ifdef ALLOY_DAC_TEST_HOOK_DHR12R1
            ALLOY_DAC_TEST_HOOK_DHR12R1(data);
        #endif

        hw()->DAC_DHR12R1 = data & 0xFFF;
    }

    /**
     * @brief Set 12-bit data for channel 2 (right-aligned)
     * @param data 12-bit data value
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DHR12R2
     */
    static inline void set_data_channel_2_12bit(uint16_t data) {
        #ifdef ALLOY_DAC_TEST_HOOK_DHR12R2
            ALLOY_DAC_TEST_HOOK_DHR12R2(data);
        #endif

        hw()->DAC_DHR12R2 = data & 0xFFF;
    }

    /**
     * @brief Set 8-bit data for channel 1
     * @param data 8-bit data value
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DHR8R1
     */
    static inline void set_data_channel_1_8bit(uint8_t data) {
        #ifdef ALLOY_DAC_TEST_HOOK_DHR8R1
            ALLOY_DAC_TEST_HOOK_DHR8R1(data);
        #endif

        hw()->DAC_DHR8R1 = data;
    }

    /**
     * @brief Set 8-bit data for channel 2
     * @param data 8-bit data value
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DHR8R2
     */
    static inline void set_data_channel_2_8bit(uint8_t data) {
        #ifdef ALLOY_DAC_TEST_HOOK_DHR8R2
            ALLOY_DAC_TEST_HOOK_DHR8R2(data);
        #endif

        hw()->DAC_DHR8R2 = data;
    }

    /**
     * @brief Software trigger for channel 1
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_SWTRIG1
     */
    static inline void trigger_channel_1() {
        #ifdef ALLOY_DAC_TEST_HOOK_SWTRIG1
            ALLOY_DAC_TEST_HOOK_SWTRIG1();
        #endif

        hw()->DAC_SWTRIGR |= (1U << 0);
    }

    /**
     * @brief Software trigger for channel 2
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_SWTRIG2
     */
    static inline void trigger_channel_2() {
        #ifdef ALLOY_DAC_TEST_HOOK_SWTRIG2
            ALLOY_DAC_TEST_HOOK_SWTRIG2();
        #endif

        hw()->DAC_SWTRIGR |= (1U << 1);
    }

    /**
     * @brief Enable DMA for channel 1
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DMAEN1
     */
    static inline void enable_dma_channel_1() {
        #ifdef ALLOY_DAC_TEST_HOOK_DMAEN1
            ALLOY_DAC_TEST_HOOK_DMAEN1();
        #endif

        hw()->DAC_CR |= (1U << 12);
    }

    /**
     * @brief Enable DMA for channel 2
     *
     * @note Test hook: ALLOY_DAC_TEST_HOOK_DMAEN2
     */
    static inline void enable_dma_channel_2() {
        #ifdef ALLOY_DAC_TEST_HOOK_DMAEN2
            ALLOY_DAC_TEST_HOOK_DMAEN2();
        #endif

        hw()->DAC_CR |= (1U << 28);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic DAC API:
 *
 * @code
 * #include "hal/api/dac_simple.hpp"
 * #include "hal/vendors/st/stm32g0/dac_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create DAC with hardware policy
 * using Uart0 = UartImpl<Stm32g0DACHardwarePolicy<UART0_BASE, 150000000>>;
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