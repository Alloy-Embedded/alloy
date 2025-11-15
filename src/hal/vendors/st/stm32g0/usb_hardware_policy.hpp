/**
 * @file usb_hardware_policy.hpp
 * @brief Hardware Policy for USB on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for USB using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_USB_MOCK_HW)
 *
 * Auto-generated from: stm32g0/usb.json
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
#include "hal/vendors/st/stm32g0/generated/registers/usb_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/generated/bitfields/usb_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::usb;

/**
 * @brief Hardware Policy for USB on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for USB. It is designed to be used as a template
 * parameter in generic USB implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0USBHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = USB_Registers;

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
        #ifdef ALLOY_USB_MOCK_HW
            return ALLOY_USB_MOCK_HW();  // Test hook
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
     * @brief Enable USB peripheral
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_PDWN
     */
    static inline void enable_usb() {
        #ifdef ALLOY_USB_TEST_HOOK_PDWN
            ALLOY_USB_TEST_HOOK_PDWN();
        #endif

        hw()->USB_CNTR &= ~(1U << 1);
    }

    /**
     * @brief Disable USB peripheral (power down)
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_PDWN
     */
    static inline void disable_usb() {
        #ifdef ALLOY_USB_TEST_HOOK_PDWN
            ALLOY_USB_TEST_HOOK_PDWN();
        #endif

        hw()->USB_CNTR |= (1U << 1);
    }

    /**
     * @brief Force USB reset
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_FRES
     */
    static inline void force_reset() {
        #ifdef ALLOY_USB_TEST_HOOK_FRES
            ALLOY_USB_TEST_HOOK_FRES();
        #endif

        hw()->USB_CNTR |= (1U << 0);
    }

    /**
     * @brief Clear USB reset
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_FRES
     */
    static inline void clear_reset() {
        #ifdef ALLOY_USB_TEST_HOOK_FRES
            ALLOY_USB_TEST_HOOK_FRES();
        #endif

        hw()->USB_CNTR &= ~(1U << 0);
    }

    /**
     * @brief Set USB device address
     * @param address Device address (0-127)
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_ADD
     */
    static inline void set_device_address(uint8_t address) {
        #ifdef ALLOY_USB_TEST_HOOK_ADD
            ALLOY_USB_TEST_HOOK_ADD(address);
        #endif

        hw()->USB_DADDR = (hw()->USB_DADDR & ~0x7F) | (address & 0x7F);
    }

    /**
     * @brief Enable USB function
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_EF
     */
    static inline void enable_function() {
        #ifdef ALLOY_USB_TEST_HOOK_EF
            ALLOY_USB_TEST_HOOK_EF();
        #endif

        hw()->USB_DADDR |= (1U << 7);
    }

    /**
     * @brief Disable USB function
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_EF
     */
    static inline void disable_function() {
        #ifdef ALLOY_USB_TEST_HOOK_EF
            ALLOY_USB_TEST_HOOK_EF();
        #endif

        hw()->USB_DADDR &= ~(1U << 7);
    }

    /**
     * @brief Enable USB reset interrupt
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_RESETM
     */
    static inline void enable_reset_interrupt() {
        #ifdef ALLOY_USB_TEST_HOOK_RESETM
            ALLOY_USB_TEST_HOOK_RESETM();
        #endif

        hw()->USB_CNTR |= (1U << 10);
    }

    /**
     * @brief Enable suspend mode interrupt
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_SUSPM
     */
    static inline void enable_suspend_interrupt() {
        #ifdef ALLOY_USB_TEST_HOOK_SUSPM
            ALLOY_USB_TEST_HOOK_SUSPM();
        #endif

        hw()->USB_CNTR |= (1U << 11);
    }

    /**
     * @brief Enable wakeup interrupt
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_WKUPM
     */
    static inline void enable_wakeup_interrupt() {
        #ifdef ALLOY_USB_TEST_HOOK_WKUPM
            ALLOY_USB_TEST_HOOK_WKUPM();
        #endif

        hw()->USB_CNTR |= (1U << 12);
    }

    /**
     * @brief Enable correct transfer interrupt
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_CTRM
     */
    static inline void enable_correct_transfer_interrupt() {
        #ifdef ALLOY_USB_TEST_HOOK_CTRM
            ALLOY_USB_TEST_HOOK_CTRM();
        #endif

        hw()->USB_CNTR |= (1U << 15);
    }

    /**
     * @brief Check if reset flag is set
     * @return bool
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_RESET
     */
    static inline bool is_reset_flag_set() {
        #ifdef ALLOY_USB_TEST_HOOK_RESET
            ALLOY_USB_TEST_HOOK_RESET();
        #endif

        return (hw()->USB_ISTR & (1U << 10)) != 0;
    }

    /**
     * @brief Check if suspend flag is set
     * @return bool
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_SUSP
     */
    static inline bool is_suspend_flag_set() {
        #ifdef ALLOY_USB_TEST_HOOK_SUSP
            ALLOY_USB_TEST_HOOK_SUSP();
        #endif

        return (hw()->USB_ISTR & (1U << 11)) != 0;
    }

    /**
     * @brief Check if correct transfer flag is set
     * @return bool
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_CTR
     */
    static inline bool is_correct_transfer_flag_set() {
        #ifdef ALLOY_USB_TEST_HOOK_CTR
            ALLOY_USB_TEST_HOOK_CTR();
        #endif

        return (hw()->USB_ISTR & (1U << 15)) != 0;
    }

    /**
     * @brief Clear reset interrupt flag
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_RESET
     */
    static inline void clear_reset_flag() {
        #ifdef ALLOY_USB_TEST_HOOK_RESET
            ALLOY_USB_TEST_HOOK_RESET();
        #endif

        hw()->USB_ISTR &= ~(1U << 10);
    }

    /**
     * @brief Clear suspend interrupt flag
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_SUSP
     */
    static inline void clear_suspend_flag() {
        #ifdef ALLOY_USB_TEST_HOOK_SUSP
            ALLOY_USB_TEST_HOOK_SUSP();
        #endif

        hw()->USB_ISTR &= ~(1U << 11);
    }

    /**
     * @brief Get endpoint identifier from interrupt
     * @return uint8_t
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_EP_ID
     */
    static inline uint8_t get_endpoint_id() {
        #ifdef ALLOY_USB_TEST_HOOK_EP_ID
            ALLOY_USB_TEST_HOOK_EP_ID();
        #endif

        return static_cast<uint8_t>(hw()->USB_ISTR & 0xF);
    }

    /**
     * @brief Get buffer table address offset
     * @return uint16_t
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_BTABLE
     */
    static inline uint16_t get_buffer_table_address() {
        #ifdef ALLOY_USB_TEST_HOOK_BTABLE
            ALLOY_USB_TEST_HOOK_BTABLE();
        #endif

        return static_cast<uint16_t>(hw()->USB_BTABLE & 0xFFF8);
    }

    /**
     * @brief Set buffer table address offset
     * @param offset Buffer table offset (must be 8-byte aligned)
     *
     * @note Test hook: ALLOY_USB_TEST_HOOK_BTABLE
     */
    static inline void set_buffer_table_address(uint16_t offset) {
        #ifdef ALLOY_USB_TEST_HOOK_BTABLE
            ALLOY_USB_TEST_HOOK_BTABLE(offset);
        #endif

        hw()->USB_BTABLE = offset & 0xFFF8;
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic USB API:
 *
 * @code
 * #include "hal/api/usb_simple.hpp"
 * #include "hal/vendors/st/stm32g0/usb_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create USB with hardware policy
 * using Uart0 = UartImpl<Stm32g0USBHardwarePolicy<UART0_BASE, 150000000>>;
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