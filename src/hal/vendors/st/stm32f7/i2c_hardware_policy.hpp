/**
 * @file i2c_hardware_policy.hpp
 * @brief Hardware Policy for I2C on STM32F7 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for I2C using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F7 I2C Features:
 * - Compatible with STM32G0 modern register layout (TIMINGR, ISR, ICR)
 * - Multi-master capability
 * - 7-bit and 10-bit addressing
 * - Standard mode (100 kHz), Fast mode (400 kHz), Fast mode Plus (1 MHz)
 * - Independent clock source
 * - DMA support for transmission and reception
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef MICROCORE_I2C_MOCK_HW)
 *
 * @note Part of Phase 3.4: I2C Implementation
 * @see docs/API_TIERS.md
 */

#pragma once

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// Register definitions (STM32F7 uses modern I2C registers like STM32G0)
#include "hal/vendors/st/stm32f7/generated/registers/i2c1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f7/generated/bitfields/i2c1_bitfields.hpp"

namespace ucore::hal::stm32f7 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f7;

// Namespace alias for bitfields
namespace i2c = st::stm32f7::i2c1;

/**
 * @brief Hardware Policy for I2C on STM32F7
 *
 * This policy provides all platform-specific hardware access methods
 * for I2C. It is designed to be used as a template parameter in
 * generic I2C implementations.
 *
 * The STM32F7 I2C peripheral uses modern register layout (TIMINGR/ISR/ICR)
 * similar to STM32G0, which is different from STM32F4 legacy registers.
 *
 * Template Parameters:
 * - BASE_ADDR: I2C peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using I2c1 = I2cImpl<Stm32f7I2cHardwarePolicy<0x40005400, 54000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for timing calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f7I2cHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = I2C1_Registers;

    // ========================================================================
    // Compile-Time Constants
    // ========================================================================

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;
    static constexpr uint32_t I2C_TIMEOUT =
        100000;  ///< I2C timeout in loop iterations (~10ms at 216MHz)

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
#ifdef MICROCORE_I2C_MOCK_HW
        return MICROCORE_I2C_MOCK_HW();  // Test hook
#else
        return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
#endif
    }

    // ========================================================================
    // Hardware Policy Methods (STM32F7 Modern Registers)
    // ========================================================================

    /**
     * @brief Enable I2C peripheral
     */
    static inline void enable_i2c() {
        hw()->CR1 |= (1U << 0);  // PE bit
    }

    /**
     * @brief Disable I2C peripheral
     */
    static inline void disable_i2c() {
        hw()->CR1 &= ~(1U << 0);  // PE bit
    }

    /**
     * @brief Set I2C timing register for clock configuration
     *
     * The TIMINGR register configures:
     * - PRESC: Timing prescaler
     * - SCLDEL: Data setup time
     * - SDADEL: Data hold time
     * - SCLH: SCL high period
     * - SCLL: SCL low period
     *
     * @param timing Timing value (calculated based on clock and speed)
     */
    static inline void set_timing(uint32_t timing) {
        hw()->TIMINGR = timing;
    }

    /**
     * @brief Set 7-bit slave address
     *
     * @param address 7-bit slave address
     */
    static inline void set_slave_address_7bit(uint8_t address) {
        hw()->CR2 = (hw()->CR2 & ~(0x3FFU << 0)) | ((address & 0x7FU) << 1);
    }

    /**
     * @brief Set transfer direction to write (master transmitter)
     */
    static inline void set_transfer_direction_write() {
        hw()->CR2 &= ~(1U << 10);  // RD_WRN bit = 0 for write
    }

    /**
     * @brief Set transfer direction to read (master receiver)
     */
    static inline void set_transfer_direction_read() {
        hw()->CR2 |= (1U << 10);  // RD_WRN bit = 1 for read
    }

    /**
     * @brief Set number of bytes to transfer
     *
     * @param nbytes Number of bytes (0-255)
     */
    static inline void set_number_of_bytes(uint8_t nbytes) {
        hw()->CR2 = (hw()->CR2 & ~(0xFFU << 16)) | ((nbytes & 0xFFU) << 16);
    }

    /**
     * @brief Generate START condition
     */
    static inline void generate_start() {
        hw()->CR2 |= (1U << 13);  // START bit
    }

    /**
     * @brief Generate STOP condition
     */
    static inline void generate_stop() {
        hw()->CR2 |= (1U << 14);  // STOP bit
    }

    /**
     * @brief Write data to transmit register
     *
     * @param data Data byte to transmit
     */
    static inline void write_data(uint8_t data) {
        hw()->TXDR = data;
    }

    /**
     * @brief Read data from receive register
     *
     * @return Received byte
     */
    static inline uint8_t read_data() {
        return static_cast<uint8_t>(hw()->RXDR & 0xFF);
    }

    /**
     * @brief Check if transmit data register is empty (TXIS flag)
     *
     * @return true if TX buffer is empty
     */
    static inline bool is_tx_empty() {
        return (hw()->ISR & (1U << 1)) != 0;  // TXIS bit
    }

    /**
     * @brief Check if receive data register has data (RXNE flag)
     *
     * @return true if RX buffer has data
     */
    static inline bool is_rx_not_empty() {
        return (hw()->ISR & (1U << 2)) != 0;  // RXNE bit
    }

    /**
     * @brief Check if transfer is complete (TC flag)
     *
     * @return true if transfer complete
     */
    static inline bool is_transfer_complete() {
        return (hw()->ISR & (1U << 6)) != 0;  // TC bit
    }

    /**
     * @brief Check if STOP condition detected (STOPF flag)
     *
     * @return true if STOP detected
     */
    static inline bool is_stop_detected() {
        return (hw()->ISR & (1U << 5)) != 0;  // STOPF bit
    }

    /**
     * @brief Check if NACK received (NACKF flag)
     *
     * @return true if NACK received
     */
    static inline bool is_nack_received() {
        return (hw()->ISR & (1U << 4)) != 0;  // NACKF bit
    }

    /**
     * @brief Clear STOP detection flag
     */
    static inline void clear_stop_flag() {
        hw()->ICR = (1U << 5);  // STOPCF bit
    }

    /**
     * @brief Clear NACK detection flag
     */
    static inline void clear_nack_flag() {
        hw()->ICR = (1U << 4);  // NACKCF bit
    }

    /**
     * @brief Check if bus is busy (BUSY flag)
     *
     * @return true if bus is busy
     */
    static inline bool is_bus_busy() {
        return (hw()->ISR & (1U << 15)) != 0;  // BUSY bit
    }

    /**
     * @brief Enable DMA for reception
     */
    static inline void enable_rx_dma() {
        hw()->CR1 |= (1U << 15);  // RXDMAEN bit
    }

    /**
     * @brief Enable DMA for transmission
     */
    static inline void enable_tx_dma() {
        hw()->CR1 |= (1U << 14);  // TXDMAEN bit
    }

    /**
     * @brief Disable DMA for reception
     */
    static inline void disable_rx_dma() {
        hw()->CR1 &= ~(1U << 15);
    }

    /**
     * @brief Disable DMA for transmission
     */
    static inline void disable_tx_dma() {
        hw()->CR1 &= ~(1U << 14);
    }

    /**
     * @brief Wait for event with timeout
     *
     * @param check_func Function to check for event
     * @param timeout_loops Timeout in loop iterations
     * @return true if event occurred, false if timeout
     */
    template <typename CheckFunc>
    static inline bool wait_for_event(CheckFunc check_func,
                                      uint32_t timeout_loops = I2C_TIMEOUT) {
        uint32_t timeout = timeout_loops;
        while (!check_func() && --timeout)
            ;
        return timeout != 0;
    }
};

}  // namespace ucore::hal::stm32f7

/**
 * @example STM32F7 I2C Example
 * @code
 * #include "hal/platform/stm32f7/i2c.hpp"
 *
 * using namespace ucore::hal::stm32f7;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto i2c = I2c1::quick_setup<SdaPin, SclPin>(I2cSpeed::Standard_100kHz);
 *     i2c.initialize();
 *
 *     // Write to I2C sensor at address 0x76 (BME280)
 *     uint8_t cmd[] = {0xF7};  // Read register 0xF7
 *     i2c.write(0x76, cmd, sizeof(cmd));
 *
 *     // Read temperature data
 *     uint8_t data[6];
 *     i2c.read(0x76, data, sizeof(data));
 * }
 * @endcode
 */
