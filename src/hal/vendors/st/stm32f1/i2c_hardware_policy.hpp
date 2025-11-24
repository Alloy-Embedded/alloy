/**
 * @file i2c_hardware_policy.hpp
 * @brief Hardware Policy for I2C on STM32F1 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for I2C using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * STM32F1 I2C Features:
 * - Multi-master capability
 * - 7-bit and 10-bit addressing
 * - Standard mode (100 kHz) and Fast mode (400 kHz)
 * - Programmable setup and hold times
 * - DMA support for transmission and reception
 * - SMBus and PMBus support
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

// Register definitions
#include "hal/vendors/st/stm32f1/generated/registers/i2c1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32f1/generated/bitfields/i2c1_bitfields.hpp"

namespace ucore::hal::stm32f1 {

using namespace ucore::core;

// Import register types
using namespace ucore::hal::st::stm32f1;

// Namespace alias for bitfields
namespace i2c = st::stm32f1::i2c1;

/**
 * @brief Hardware Policy for I2C on STM32F1
 *
 * This policy provides all platform-specific hardware access methods
 * for I2C. It is designed to be used as a template parameter in
 * generic I2C implementations.
 *
 * The STM32F1 I2C peripheral uses legacy register layout (CR1/CR2/SR1/SR2/DR)
 * similar to STM32F4, which is different from modern STM32F7/G0.
 *
 * Template Parameters:
 * - BASE_ADDR: I2C peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz (APB1, typically 36 MHz)
 *
 * Usage:
 * @code
 * // Platform-specific alias
 * using I2c1 = I2cImpl<Stm32f1I2cHardwarePolicy<0x40005400, 36000000>>;
 * @endcode
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency (for timing calculation)
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32f1I2cHardwarePolicy {
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
        100000;  ///< I2C timeout in loop iterations (~10ms at 72MHz)

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
    // Hardware Policy Methods (STM32F1 Legacy Registers)
    // ========================================================================

    /**
     * @brief Enable I2C peripheral
     */
    static inline void enable_i2c() {
        hw()->CR1 |= i2c::cr1::PE::mask;
    }

    /**
     * @brief Disable I2C peripheral
     */
    static inline void disable_i2c() {
        hw()->CR1 &= ~i2c::cr1::PE::mask;
    }

    /**
     * @brief Software reset I2C peripheral
     */
    static inline void reset() {
        hw()->CR1 |= i2c::cr1::SWRST::mask;
        hw()->CR1 &= ~i2c::cr1::SWRST::mask;
    }

    /**
     * @brief Configure I2C clock (CCR register)
     *
     * For Standard mode (100 kHz):
     * - CCR = PERIPH_CLOCK_HZ / (2 * 100000)
     *
     * For Fast mode (400 kHz):
     * - CCR = PERIPH_CLOCK_HZ / (3 * 400000) (duty cycle 2:1)
     *
     * @param ccr Clock control register value
     * @param fast_mode true for Fast mode (400 kHz), false for Standard (100 kHz)
     */
    static inline void configure_clock(uint16_t ccr, bool fast_mode = false) {
        uint32_t ccr_value = ccr & 0xFFF;
        if (fast_mode) {
            ccr_value |= i2c::ccr::F_S::mask;  // Fast mode
        }
        hw()->CCR = ccr_value;
    }

    /**
     * @brief Set TRISE register (maximum rise time)
     *
     * For Standard mode: TRISE = (PERIPH_CLOCK_HZ / 1000000) + 1
     * For Fast mode: TRISE = ((PERIPH_CLOCK_HZ * 300) / 1000000000) + 1
     *
     * @param trise Rise time value
     */
    static inline void set_rise_time(uint8_t trise) {
        hw()->TRISE = trise & 0x3F;
    }

    /**
     * @brief Set own 7-bit address
     *
     * @param address 7-bit address
     */
    static inline void set_own_address(uint8_t address) {
        hw()->OAR1 = (address << 1) | (1 << 14);  // Bit 14 must be kept at 1
    }

    /**
     * @brief Generate START condition
     */
    static inline void generate_start() {
        hw()->CR1 |= i2c::cr1::START::mask;
    }

    /**
     * @brief Generate STOP condition
     */
    static inline void generate_stop() {
        hw()->CR1 |= i2c::cr1::STOP::mask;
    }

    /**
     * @brief Enable ACK generation
     */
    static inline void enable_ack() {
        hw()->CR1 |= i2c::cr1::ACK::mask;
    }

    /**
     * @brief Disable ACK generation
     */
    static inline void disable_ack() {
        hw()->CR1 &= ~i2c::cr1::ACK::mask;
    }

    /**
     * @brief Send 7-bit slave address
     *
     * @param address 7-bit slave address
     * @param read true for read operation, false for write
     */
    static inline void send_address(uint8_t address, bool read) {
        uint8_t addr = (address << 1) | (read ? 1 : 0);
        hw()->DR = addr;
    }

    /**
     * @brief Write data byte
     *
     * @param data Data byte to write
     */
    static inline void write_data(uint8_t data) {
        hw()->DR = data;
    }

    /**
     * @brief Read data byte
     *
     * @return Received byte
     */
    static inline uint8_t read_data() {
        return static_cast<uint8_t>(hw()->DR & 0xFF);
    }

    /**
     * @brief Check if START condition generated (SB flag)
     *
     * @return true if START condition generated
     */
    static inline bool is_start_generated() {
        return (hw()->SR1 & i2c::sr1::SB::mask) != 0;
    }

    /**
     * @brief Check if address sent (ADDR flag)
     *
     * @return true if address sent and ACK received
     */
    static inline bool is_address_sent() {
        return (hw()->SR1 & i2c::sr1::ADDR::mask) != 0;
    }

    /**
     * @brief Check if byte transfer finished (BTF flag)
     *
     * @return true if byte transfer finished
     */
    static inline bool is_byte_transfer_finished() {
        return (hw()->SR1 & i2c::sr1::BTF::mask) != 0;
    }

    /**
     * @brief Check if transmit buffer empty (TXE flag)
     *
     * @return true if TX buffer is empty
     */
    static inline bool is_tx_empty() {
        return (hw()->SR1 & i2c::sr1::TXE::mask) != 0;
    }

    /**
     * @brief Check if receive buffer not empty (RXNE flag)
     *
     * @return true if RX buffer has data
     */
    static inline bool is_rx_not_empty() {
        return (hw()->SR1 & i2c::sr1::RXNE::mask) != 0;
    }

    /**
     * @brief Check for acknowledge failure (AF flag)
     *
     * @return true if NACK received
     */
    static inline bool is_nack_received() {
        return (hw()->SR1 & i2c::sr1::AF::mask) != 0;
    }

    /**
     * @brief Clear address sent flag
     *
     * This is done by reading SR1 followed by reading SR2.
     */
    static inline void clear_addr_flag() {
        volatile uint32_t dummy;
        dummy = hw()->SR1;
        dummy = hw()->SR2;
        (void)dummy;  // Prevent unused variable warning
    }

    /**
     * @brief Clear acknowledge failure flag
     */
    static inline void clear_nack_flag() {
        hw()->SR1 &= ~i2c::sr1::AF::mask;
    }

    /**
     * @brief Check if bus is busy (BUSY flag in SR2)
     *
     * @return true if bus is busy
     */
    static inline bool is_bus_busy() {
        return (hw()->SR2 & i2c::sr2::BUSY::mask) != 0;
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

}  // namespace ucore::hal::stm32f1

/**
 * @example STM32F1 I2C Example
 * @code
 * #include "hal/platform/stm32f1/i2c.hpp"
 *
 * using namespace ucore::hal::stm32f1;
 *
 * int main() {
 *     // Simple API - uses hardware policy internally
 *     auto i2c = I2c1::quick_setup<SdaPin, SclPin>(I2cSpeed::Standard_100kHz);
 *     i2c.initialize();
 *
 *     // Write to EEPROM at address 0x50
 *     uint8_t data[] = {0x00, 0x00, 0xAA, 0xBB};  // Address + data
 *     i2c.write(0x50, data, sizeof(data));
 *
 *     // Read from EEPROM
 *     uint8_t buffer[2];
 *     i2c.read(0x50, buffer, sizeof(buffer));
 * }
 * @endcode
 */
