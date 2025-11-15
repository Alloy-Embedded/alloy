/**
 * @file dma_hardware_policy.hpp
 * @brief Hardware Policy for DMA on STM32G0 (Policy-Based Design)
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
 * Auto-generated from: stm32g0/dma.json
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
#include "hal/vendors/st/stm32g0/generated/registers/dma1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/generated/bitfields/dma1_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::dma1;

/**
 * @brief Hardware Policy for DMA on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for DMA. It is designed to be used as a template
 * parameter in generic DMA implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0DMAHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = DMA1_Registers;

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
     * @brief Enable DMA channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_EN1
     */
    static inline void enable_channel_1() {
        #ifdef ALLOY_DMA_TEST_HOOK_EN1
            ALLOY_DMA_TEST_HOOK_EN1();
        #endif

        hw()->DMA_CCR1 |= (1U << 0);
    }

    /**
     * @brief Disable DMA channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_EN1
     */
    static inline void disable_channel_1() {
        #ifdef ALLOY_DMA_TEST_HOOK_EN1
            ALLOY_DMA_TEST_HOOK_EN1();
        #endif

        hw()->DMA_CCR1 &= ~(1U << 0);
    }

    /**
     * @brief Set number of data to transfer for channel 1
     * @param count Number of data items (1-65535)
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_NDT1
     */
    static inline void set_transfer_count_ch1(uint16_t count) {
        #ifdef ALLOY_DMA_TEST_HOOK_NDT1
            ALLOY_DMA_TEST_HOOK_NDT1(count);
        #endif

        hw()->DMA_CNDTR1 = count & 0xFFFF;
    }

    /**
     * @brief Set peripheral address for channel 1
     * @param address Peripheral register address
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PA1
     */
    static inline void set_peripheral_address_ch1(uint32_t address) {
        #ifdef ALLOY_DMA_TEST_HOOK_PA1
            ALLOY_DMA_TEST_HOOK_PA1(address);
        #endif

        hw()->DMA_CPAR1 = address;
    }

    /**
     * @brief Set memory address for channel 1
     * @param address Memory buffer address
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MA1
     */
    static inline void set_memory_address_ch1(uint32_t address) {
        #ifdef ALLOY_DMA_TEST_HOOK_MA1
            ALLOY_DMA_TEST_HOOK_MA1(address);
        #endif

        hw()->DMA_CMAR1 = address;
    }

    /**
     * @brief Set transfer direction peripheral-to-memory for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_DIR1
     */
    static inline void set_direction_periph_to_mem_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_DIR1
            ALLOY_DMA_TEST_HOOK_DIR1();
        #endif

        hw()->DMA_CCR1 &= ~(1U << 4);
    }

    /**
     * @brief Set transfer direction memory-to-peripheral for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_DIR1
     */
    static inline void set_direction_mem_to_periph_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_DIR1
            ALLOY_DMA_TEST_HOOK_DIR1();
        #endif

        hw()->DMA_CCR1 |= (1U << 4);
    }

    /**
     * @brief Enable circular mode for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_CIRC1
     */
    static inline void enable_circular_mode_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_CIRC1
            ALLOY_DMA_TEST_HOOK_CIRC1();
        #endif

        hw()->DMA_CCR1 |= (1U << 5);
    }

    /**
     * @brief Disable circular mode for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_CIRC1
     */
    static inline void disable_circular_mode_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_CIRC1
            ALLOY_DMA_TEST_HOOK_CIRC1();
        #endif

        hw()->DMA_CCR1 &= ~(1U << 5);
    }

    /**
     * @brief Enable memory address increment for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MINC1
     */
    static inline void enable_memory_increment_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_MINC1
            ALLOY_DMA_TEST_HOOK_MINC1();
        #endif

        hw()->DMA_CCR1 |= (1U << 7);
    }

    /**
     * @brief Disable memory address increment for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MINC1
     */
    static inline void disable_memory_increment_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_MINC1
            ALLOY_DMA_TEST_HOOK_MINC1();
        #endif

        hw()->DMA_CCR1 &= ~(1U << 7);
    }

    /**
     * @brief Enable peripheral address increment for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PINC1
     */
    static inline void enable_peripheral_increment_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PINC1
            ALLOY_DMA_TEST_HOOK_PINC1();
        #endif

        hw()->DMA_CCR1 |= (1U << 6);
    }

    /**
     * @brief Set memory data size to 8-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MSIZE1
     */
    static inline void set_memory_size_8bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_MSIZE1
            ALLOY_DMA_TEST_HOOK_MSIZE1();
        #endif

        hw()->DMA_CCR1 &= ~(0x3U << 10);
    }

    /**
     * @brief Set memory data size to 16-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MSIZE1
     */
    static inline void set_memory_size_16bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_MSIZE1
            ALLOY_DMA_TEST_HOOK_MSIZE1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 10)) | (0x1U << 10);
    }

    /**
     * @brief Set memory data size to 32-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_MSIZE1
     */
    static inline void set_memory_size_32bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_MSIZE1
            ALLOY_DMA_TEST_HOOK_MSIZE1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 10)) | (0x2U << 10);
    }

    /**
     * @brief Set peripheral data size to 8-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PSIZE1
     */
    static inline void set_peripheral_size_8bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PSIZE1
            ALLOY_DMA_TEST_HOOK_PSIZE1();
        #endif

        hw()->DMA_CCR1 &= ~(0x3U << 8);
    }

    /**
     * @brief Set peripheral data size to 16-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PSIZE1
     */
    static inline void set_peripheral_size_16bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PSIZE1
            ALLOY_DMA_TEST_HOOK_PSIZE1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 8)) | (0x1U << 8);
    }

    /**
     * @brief Set peripheral data size to 32-bit for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PSIZE1
     */
    static inline void set_peripheral_size_32bit_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PSIZE1
            ALLOY_DMA_TEST_HOOK_PSIZE1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 8)) | (0x2U << 8);
    }

    /**
     * @brief Set low priority for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PL1
     */
    static inline void set_priority_low_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PL1
            ALLOY_DMA_TEST_HOOK_PL1();
        #endif

        hw()->DMA_CCR1 &= ~(0x3U << 12);
    }

    /**
     * @brief Set medium priority for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PL1
     */
    static inline void set_priority_medium_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PL1
            ALLOY_DMA_TEST_HOOK_PL1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 12)) | (0x1U << 12);
    }

    /**
     * @brief Set high priority for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PL1
     */
    static inline void set_priority_high_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PL1
            ALLOY_DMA_TEST_HOOK_PL1();
        #endif

        hw()->DMA_CCR1 = (hw()->DMA_CCR1 & ~(0x3U << 12)) | (0x2U << 12);
    }

    /**
     * @brief Set very high priority for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_PL1
     */
    static inline void set_priority_very_high_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_PL1
            ALLOY_DMA_TEST_HOOK_PL1();
        #endif

        hw()->DMA_CCR1 |= (0x3U << 12);
    }

    /**
     * @brief Enable transfer complete interrupt for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_TCIE1
     */
    static inline void enable_transfer_complete_interrupt_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_TCIE1
            ALLOY_DMA_TEST_HOOK_TCIE1();
        #endif

        hw()->DMA_CCR1 |= (1U << 1);
    }

    /**
     * @brief Check if transfer is complete for channel 1
     * @return bool
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_TCIF1
     */
    static inline bool is_transfer_complete_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_TCIF1
            ALLOY_DMA_TEST_HOOK_TCIF1();
        #endif

        return (hw()->DMA_ISR & (1U << 1)) != 0;
    }

    /**
     * @brief Clear transfer complete flag for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_CTCIF1
     */
    static inline void clear_transfer_complete_flag_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_CTCIF1
            ALLOY_DMA_TEST_HOOK_CTCIF1();
        #endif

        hw()->DMA_IFCR = (1U << 1);
    }

    /**
     * @brief Check if transfer error occurred for channel 1
     * @return bool
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_TEIF1
     */
    static inline bool is_transfer_error_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_TEIF1
            ALLOY_DMA_TEST_HOOK_TEIF1();
        #endif

        return (hw()->DMA_ISR & (1U << 3)) != 0;
    }

    /**
     * @brief Clear transfer error flag for channel 1
     *
     * @note Test hook: ALLOY_DMA_TEST_HOOK_CTEIF1
     */
    static inline void clear_transfer_error_flag_ch1() {
        #ifdef ALLOY_DMA_TEST_HOOK_CTEIF1
            ALLOY_DMA_TEST_HOOK_CTEIF1();
        #endif

        hw()->DMA_IFCR = (1U << 3);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic DMA API:
 *
 * @code
 * #include "hal/api/dma_simple.hpp"
 * #include "hal/vendors/st/stm32g0/dma_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create DMA with hardware policy
 * using Uart0 = UartImpl<Stm32g0DMAHardwarePolicy<UART0_BASE, 150000000>>;
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