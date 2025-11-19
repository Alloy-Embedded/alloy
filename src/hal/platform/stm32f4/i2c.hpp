/**
 * @file twihs.hpp
 * @brief Template-based I2C implementation for STM32F4 (Platform Layer)
 *
 * This file implements I2C peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Error handling: Uses Result<T, ErrorCode> for robust error handling
 * - Master mode only (for simplicity)
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: stm32f4
 * Generator: generate_platform_twihs.py
 * Generated: 2025-11-07 17:33:03
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "hal/types.hpp"

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/st/stm32f4/registers/i2c3_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/st/stm32f4/bitfields/i2c3_bitfields.hpp"


namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfield access
namespace i2c = alloy::hal::st::stm32f4::i2c3;

// Note: I2C uses common I2cMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1


/**
 * @brief Template-based I2C peripheral for STM32F4
 *
 * This class provides a template-based I2C implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: I2C peripheral base address
 * - IRQ_ID: I2C interrupt ID for clock enable
 *
 * Example usage:
 * @code
 * // Basic I2C usage example
 * using MyI2c = I2c<I2C1_BASE, I2C1_IRQ>;
 * auto i2c = MyI2c{};
 * i2c.open();
 * i2c.setSpeed(I2cSpeed::Standard100kHz);
 * uint8_t data[] = {0x01, 0x02, 0x03};
 * i2c.write(0x50, data, 3);
 * i2c.close();
 * @endcode
 *
 * @tparam BASE_ADDR I2C peripheral base address
 * @tparam IRQ_ID I2C interrupt ID for clock enable
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class I2c {
   public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t I2C_TIMEOUT =
        100000;  ///< I2C timeout in loop iterations (~10ms at 168MHz)

    /**
     * @brief Get I2C peripheral registers
     *
     * Returns pointer to I2C registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::st::stm32f4::i2c3::I2C3_Registers* get_hw() {
#ifdef ALLOY_I2C_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_I2C_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::st::stm32f4::i2c3::I2C3_Registers*>(BASE_ADDR);
#endif
    }

    constexpr I2c() = default;

    /**
     * @brief Initialize I2C peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable peripheral clock (RCC)
        // TODO: Enable peripheral clock via RCC

        // Reset and enable I2C peripheral
        hw->CR1 = i2c::cr1::SWRST::mask;
        hw->CR1 = 0;
        hw->CR1 = i2c::cr1::PE::mask;

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close I2C peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Disable I2C peripheral
        hw->CR1 &= ~i2c::cr1::PE::mask;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Set I2C bus speed
     *
     * @param speed I2C bus speed (Standard 100kHz, Fast 400kHz)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setSpeed(I2cSpeed speed) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Configure I2C clock based on APB1 frequency
        uint32_t speed_hz = static_cast<uint32_t>(speed);
        // Assuming 42MHz APB1 clock for STM32F4
        constexpr uint32_t PCLK1 = 42000000;

        // Disable peripheral during configuration
        hw->CR1 &= ~i2c::cr1::PE::mask;

        // Configure frequency (APB1 freq in MHz)
        uint32_t cr2 = hw->CR2 & ~i2c::cr2::FREQ::mask;
        cr2 = i2c::cr2::FREQ::write(cr2, PCLK1 / 1000000);
        hw->CR2 = cr2;

        // Configure CCR
        uint32_t ccr = 0;
        if (speed_hz <= 100000) {
            // Standard mode (100kHz)
            uint32_t ccr_value = PCLK1 / (speed_hz * 2);
            ccr = i2c::ccr::CCR::write(ccr, ccr_value);
            hw->TRISE = (PCLK1 / 1000000) + 1;
        } else {
            // Fast mode (400kHz)
            uint32_t ccr_value = PCLK1 / (speed_hz * 3);
            ccr = i2c::ccr::F_S::set(ccr);
            ccr = i2c::ccr::CCR::write(ccr, ccr_value);
            hw->TRISE = ((PCLK1 / 1000000) * 300) / 1000 + 1;
        }
        hw->CCR = ccr;

        // Re-enable peripheral
        hw->CR1 |= i2c::cr1::PE::mask;

        return Ok();
    }

    /**
     * @brief Write data to I2C device
     *
     * @param device_addr 7-bit I2C device address
     * @param data Data buffer to write
     * @param size Number of bytes to write
     * @return Result<size_t, ErrorCode> Write data to I2C device     */
    Result<size_t, ErrorCode> write(uint8_t device_addr, const uint8_t* data, size_t size) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr || size == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Generate START condition and send address
        hw->CR1 |= i2c::cr1::START::mask;

        uint32_t timeout = 0;
        while (!(hw->SR1 & i2c::sr1::SB::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }

        hw->DR = device_addr << 1;

        timeout = 0;
        while (!(hw->SR1 & i2c::sr1::ADDR::mask)) {
            if (hw->SR1 & i2c::sr1::AF::mask) {
                hw->CR1 |= i2c::cr1::STOP::mask;
                return Err(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                hw->CR1 |= i2c::cr1::STOP::mask;
                return Err(ErrorCode::Timeout);
            }
        }

        // Clear ADDR flag by reading SR1 and SR2
        (void)hw->SR1;
        (void)hw->SR2;

        // Send data bytes
        for (size_t i = 0; i < size; ++i) {
            hw->DR = data[i];

            timeout = 0;
            while (!(hw->SR1 & i2c::sr1::TxE::mask)) {
                if (++timeout > I2C_TIMEOUT) {
                    hw->CR1 |= i2c::cr1::STOP::mask;
                    return Err(ErrorCode::Timeout);
                }
            }
        }

        // Wait for byte transfer finished
        timeout = 0;
        while (!(hw->SR1 & i2c::sr1::BTF::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                hw->CR1 |= i2c::cr1::STOP::mask;
                return Err(ErrorCode::Timeout);
            }
        }

        // Generate STOP condition
        hw->CR1 |= i2c::cr1::STOP::mask;

        return Ok(size_t(size));
    }

    /**
     * @brief Read data from I2C device
     *
     * @param device_addr 7-bit I2C device address
     * @param data Buffer for received data
     * @param size Number of bytes to read
     * @return Result<size_t, ErrorCode> Read data from I2C device     */
    Result<size_t, ErrorCode> read(uint8_t device_addr, uint8_t* data, size_t size) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr || size == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Generate START condition and send address with read bit
        hw->CR1 |= i2c::cr1::ACK::mask;
        hw->CR1 |= i2c::cr1::START::mask;

        uint32_t timeout = 0;
        while (!(hw->SR1 & i2c::sr1::SB::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }

        hw->DR = (device_addr << 1) | 0x01;

        timeout = 0;
        while (!(hw->SR1 & i2c::sr1::ADDR::mask)) {
            if (hw->SR1 & i2c::sr1::AF::mask) {
                hw->CR1 |= i2c::cr1::STOP::mask;
                return Err(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                hw->CR1 |= i2c::cr1::STOP::mask;
                return Err(ErrorCode::Timeout);
            }
        }

        // Clear ADDR flag
        (void)hw->SR1;
        (void)hw->SR2;

        // Read data bytes
        for (size_t i = 0; i < size; ++i) {
            if (i == size - 1) {
                // Last byte - NACK and STOP
                hw->CR1 &= ~i2c::cr1::ACK::mask;
                hw->CR1 |= i2c::cr1::STOP::mask;
            }

            timeout = 0;
            while (!(hw->SR1 & i2c::sr1::RxNE::mask)) {
                if (++timeout > I2C_TIMEOUT) {
                    hw->CR1 |= i2c::cr1::STOP::mask;
                    return Err(ErrorCode::Timeout);
                }
            }

            data[i] = static_cast<uint8_t>(hw->DR);
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Write single byte to device register
     *
     * @param device_addr 7-bit I2C device address
     * @param reg_addr Register address
     * @param value Value to write
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> writeRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t value) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Write register address and value
        uint8_t buffer[2] = {reg_addr, value};
        auto result = write(device_addr, buffer, 2);
        if (!result.is_ok()) {
            return Err(result.err());
        }

        return Ok();
    }

    /**
     * @brief Read single byte from device register
     *
     * @param device_addr 7-bit I2C device address
     * @param reg_addr Register address
     * @param value Pointer to store read value
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> readRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t* value) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (value == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Write register address then read value
        auto write_result = write(device_addr, &reg_addr, 1);
        if (!write_result.is_ok()) {
            return Err(write_result.err());
        }

        auto read_result = read(device_addr, value, 1);
        if (!read_result.is_ok()) {
            return Err(read_result.err());
        }

        return Ok();
    }

    /**
     * @brief Check if I2C peripheral is open
     *
     * @return bool Check if I2C peripheral is open     */
    bool isOpen() const { return m_opened; }

   private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
};

// ==============================================================================
// Predefined I2C Instances
// ==============================================================================

constexpr uint32_t I2C1_BASE = 0x40005400;
constexpr uint32_t I2C1_IRQ = 31;

constexpr uint32_t I2C2_BASE = 0x40005800;
constexpr uint32_t I2C2_IRQ = 33;

constexpr uint32_t I2C3_BASE = 0x40005C00;
constexpr uint32_t I2C3_IRQ = 72;

using I2c1 = I2c<I2C1_BASE, I2C1_IRQ>;  ///< I2C1 instance
using I2c2 = I2c<I2C2_BASE, I2C2_IRQ>;  ///< I2C2 instance
using I2c3 = I2c<I2C3_BASE, I2C3_IRQ>;  ///< I2C3 instance

}  // namespace alloy::hal::stm32f4
