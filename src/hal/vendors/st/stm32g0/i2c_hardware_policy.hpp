/**
 * @file i2c_hardware_policy.hpp
 * @brief Hardware Policy for I2C on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for I2C using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_I2C_MOCK_HW)
 *
 * Auto-generated from: stm32g0/i2c.json
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
#include "hal/vendors/st/stm32g0/registers/i2c1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/bitfields/i2c1_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::i2c1;

/**
 * @brief Hardware Policy for I2C on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for I2C. It is designed to be used as a template
 * parameter in generic I2C implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0I2CHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = I2C1_Registers;

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
     * @brief Enable I2C peripheral
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_PE
     */
    static inline void enable_i2c() {
        #ifdef ALLOY_I2C_TEST_HOOK_PE
            ALLOY_I2C_TEST_HOOK_PE();
        #endif

        hw()->I2C_CR1 |= (1U << 0);
    }

    /**
     * @brief Disable I2C peripheral
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_PE
     */
    static inline void disable_i2c() {
        #ifdef ALLOY_I2C_TEST_HOOK_PE
            ALLOY_I2C_TEST_HOOK_PE();
        #endif

        hw()->I2C_CR1 &= ~(1U << 0);
    }

    /**
     * @brief Set I2C timing register for clock configuration
     * @param timing Timing value (PRESC:SCLDEL:SDADEL:SCLH:SCLL)
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TIMINGR
     */
    static inline void set_timing(uint32_t timing) {
        #ifdef ALLOY_I2C_TEST_HOOK_TIMINGR
            ALLOY_I2C_TEST_HOOK_TIMINGR(timing);
        #endif

        hw()->I2C_TIMINGR = timing;
    }

    /**
     * @brief Set 7-bit slave address
     * @param address 7-bit slave address
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_SADD
     */
    static inline void set_slave_address_7bit(uint8_t address) {
        #ifdef ALLOY_I2C_TEST_HOOK_SADD
            ALLOY_I2C_TEST_HOOK_SADD(address);
        #endif

        hw()->I2C_CR2 = (hw()->I2C_CR2 & ~(0x3FFU << 0)) | ((address & 0x7FU) << 1);
    }

    /**
     * @brief Set transfer direction to write (master transmitter)
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RD_WRN
     */
    static inline void set_transfer_direction_write() {
        #ifdef ALLOY_I2C_TEST_HOOK_RD_WRN
            ALLOY_I2C_TEST_HOOK_RD_WRN();
        #endif

        hw()->I2C_CR2 &= ~(1U << 10);
    }

    /**
     * @brief Set transfer direction to read (master receiver)
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RD_WRN
     */
    static inline void set_transfer_direction_read() {
        #ifdef ALLOY_I2C_TEST_HOOK_RD_WRN
            ALLOY_I2C_TEST_HOOK_RD_WRN();
        #endif

        hw()->I2C_CR2 |= (1U << 10);
    }

    /**
     * @brief Set number of bytes to transfer
     * @param nbytes Number of bytes (0-255)
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_NBYTES
     */
    static inline void set_number_of_bytes(uint8_t nbytes) {
        #ifdef ALLOY_I2C_TEST_HOOK_NBYTES
            ALLOY_I2C_TEST_HOOK_NBYTES(nbytes);
        #endif

        hw()->I2C_CR2 = (hw()->I2C_CR2 & ~(0xFFU << 16)) | ((nbytes & 0xFFU) << 16);
    }

    /**
     * @brief Generate START condition
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_START
     */
    static inline void generate_start() {
        #ifdef ALLOY_I2C_TEST_HOOK_START
            ALLOY_I2C_TEST_HOOK_START();
        #endif

        hw()->I2C_CR2 |= (1U << 13);
    }

    /**
     * @brief Generate STOP condition
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_STOP
     */
    static inline void generate_stop() {
        #ifdef ALLOY_I2C_TEST_HOOK_STOP
            ALLOY_I2C_TEST_HOOK_STOP();
        #endif

        hw()->I2C_CR2 |= (1U << 14);
    }

    /**
     * @brief Write data to transmit register
     * @param data Data byte to transmit
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TXDR
     */
    static inline void write_data(uint8_t data) {
        #ifdef ALLOY_I2C_TEST_HOOK_TXDR
            ALLOY_I2C_TEST_HOOK_TXDR(data);
        #endif

        hw()->I2C_TXDR = data;
    }

    /**
     * @brief Read data from receive register
     * @return uint8_t
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RXDR
     */
    static inline uint8_t read_data() {
        #ifdef ALLOY_I2C_TEST_HOOK_RXDR
            ALLOY_I2C_TEST_HOOK_RXDR();
        #endif

        return static_cast<uint8_t>(hw()->I2C_RXDR & 0xFF);
    }

    /**
     * @brief Check if transmit data register is empty
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TXE
     */
    static inline bool is_tx_empty() {
        #ifdef ALLOY_I2C_TEST_HOOK_TXE
            ALLOY_I2C_TEST_HOOK_TXE();
        #endif

        return (hw()->I2C_ISR & (1U << 0)) != 0;
    }

    /**
     * @brief Check if receive data register has data
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RXNE
     */
    static inline bool is_rx_not_empty() {
        #ifdef ALLOY_I2C_TEST_HOOK_RXNE
            ALLOY_I2C_TEST_HOOK_RXNE();
        #endif

        return (hw()->I2C_ISR & (1U << 2)) != 0;
    }

    /**
     * @brief Check if transfer is complete
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TC
     */
    static inline bool is_transfer_complete() {
        #ifdef ALLOY_I2C_TEST_HOOK_TC
            ALLOY_I2C_TEST_HOOK_TC();
        #endif

        return (hw()->I2C_ISR & (1U << 6)) != 0;
    }

    /**
     * @brief Check if STOP condition detected
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_STOPF
     */
    static inline bool is_stop_detected() {
        #ifdef ALLOY_I2C_TEST_HOOK_STOPF
            ALLOY_I2C_TEST_HOOK_STOPF();
        #endif

        return (hw()->I2C_ISR & (1U << 5)) != 0;
    }

    /**
     * @brief Check if NACK received
     * @return bool
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_NACKF
     */
    static inline bool is_nack_received() {
        #ifdef ALLOY_I2C_TEST_HOOK_NACKF
            ALLOY_I2C_TEST_HOOK_NACKF();
        #endif

        return (hw()->I2C_ISR & (1U << 4)) != 0;
    }

    /**
     * @brief Clear STOP detection flag
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_STOPCF
     */
    static inline void clear_stop_flag() {
        #ifdef ALLOY_I2C_TEST_HOOK_STOPCF
            ALLOY_I2C_TEST_HOOK_STOPCF();
        #endif

        hw()->I2C_ICR = (1U << 5);
    }

    /**
     * @brief Clear NACK detection flag
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_NACKCF
     */
    static inline void clear_nack_flag() {
        #ifdef ALLOY_I2C_TEST_HOOK_NACKCF
            ALLOY_I2C_TEST_HOOK_NACKCF();
        #endif

        hw()->I2C_ICR = (1U << 4);
    }

    /**
     * @brief Enable DMA for reception
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RXDMAEN
     */
    static inline void enable_rx_dma() {
        #ifdef ALLOY_I2C_TEST_HOOK_RXDMAEN
            ALLOY_I2C_TEST_HOOK_RXDMAEN();
        #endif

        hw()->I2C_CR1 |= (1U << 15);
    }

    /**
     * @brief Enable DMA for transmission
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TXDMAEN
     */
    static inline void enable_tx_dma() {
        #ifdef ALLOY_I2C_TEST_HOOK_TXDMAEN
            ALLOY_I2C_TEST_HOOK_TXDMAEN();
        #endif

        hw()->I2C_CR1 |= (1U << 14);
    }

    /**
     * @brief Disable DMA for reception
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_RXDMAEN
     */
    static inline void disable_rx_dma() {
        #ifdef ALLOY_I2C_TEST_HOOK_RXDMAEN
            ALLOY_I2C_TEST_HOOK_RXDMAEN();
        #endif

        hw()->I2C_CR1 &= ~(1U << 15);
    }

    /**
     * @brief Disable DMA for transmission
     *
     * @note Test hook: ALLOY_I2C_TEST_HOOK_TXDMAEN
     */
    static inline void disable_tx_dma() {
        #ifdef ALLOY_I2C_TEST_HOOK_TXDMAEN
            ALLOY_I2C_TEST_HOOK_TXDMAEN();
        #endif

        hw()->I2C_CR1 &= ~(1U << 14);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic I2C API:
 *
 * @code
 * #include "hal/api/i2c_simple.hpp"
 * #include "hal/vendors/st/stm32g0/i2c_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create I2C with hardware policy
 * using Uart0 = UartImpl<Stm32g0I2CHardwarePolicy<UART0_BASE, 150000000>>;
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