/**
 * @file twihs.hpp
 * @brief Template-based I2C implementation for SAME70 (Platform Layer)
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
 * Auto-generated from: same70
 * Generator: generate_platform_twihs.py
 * Generated: 2025-11-14 09:58:16
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/atmel/same70/registers/twihs0_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/twihs0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace twihs = alloy::hal::atmel::same70::twihs0;

// Note: I2C uses common I2cMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1


/**
 * @brief Template-based I2C peripheral for SAME70
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
 * using MyI2c = I2c<TWIHS0_BASE, TWIHS0_IRQ>;
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
    static constexpr uint32_t I2C_TIMEOUT = 100000;  ///< I2C timeout in loop iterations (~10ms at 150MHz)

    /**
     * @brief Get TWIHS peripheral registers
     *
     * Returns pointer to TWIHS registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::atmel::same70::twihs0::TWIHS0_Registers* get_hw() {
#ifdef ALLOY_I2C_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_I2C_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::atmel::same70::twihs0::TWIHS0_Registers*>(BASE_ADDR);
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

        // Enable peripheral clock (PMC)
        // TODO: Enable peripheral clock via PMC

        // Reset and configure I2C master mode
        hw->CR = twihs::cr::SWRST::mask;
        hw->CR = twihs::cr::MSDIS::mask | twihs::cr::SVDIS::mask;
        hw->CR = twihs::cr::MSEN::mask;

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

        // Disable I2C master
        hw->CR = twihs::cr::MSDIS::mask;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Set I2C bus speed
     *
     * @param speed I2C bus speed (Standard 100kHz, Fast 400kHz, etc)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> setSpeed(I2cSpeed speed) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        // Calculate clock divider based on master clock frequency
        uint32_t speed_hz = static_cast<uint32_t>(speed);
        // Assuming 150MHz master clock for SAME70
        constexpr uint32_t MCK = 150000000;
        uint32_t div = (MCK / (2 * speed_hz)) - 4;
        
        if (div > 255) {
            return Err(ErrorCode::InvalidParameter);
        }
        
        // Configure clock waveform generator
        uint32_t cwgr = 0;
        cwgr = twihs::cwgr::CLDIV::write(cwgr, div);
        cwgr = twihs::cwgr::CHDIV::write(cwgr, div);
        cwgr = twihs::cwgr::CKDIV::write(cwgr, 0);
        hw->CWGR = cwgr;

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

        // Configure master mode for write and transmit data
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        hw->MMR = mmr;
        
        for (size_t i = 0; i < size; ++i) {
            hw->THR = data[i];
            
            uint32_t timeout = 0;
            while (!(hw->SR & twihs::sr::TXRDY::mask)) {
                if (hw->SR & twihs::sr::NACK::mask) {
                    return Err(ErrorCode::CommunicationError);
                }
                if (++timeout > I2C_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }
        }
        
        hw->CR = twihs::cr::STOP::mask;
        
        uint32_t timeout = 0;
        while (!(hw->SR & twihs::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }

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

        // Configure master mode for read and receive data
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::MREAD::set(mmr);
        hw->MMR = mmr;
        
        if (size == 1) {
            hw->CR = twihs::cr::START::mask | twihs::cr::STOP::mask;
        } else {
            hw->CR = twihs::cr::START::mask;
        }
        
        for (size_t i = 0; i < size; ++i) {
            if (i == size - 1 && size > 1) {
                hw->CR = twihs::cr::STOP::mask;
            }
            
            uint32_t timeout = 0;
            while (!(hw->SR & twihs::sr::RXRDY::mask)) {
                if (hw->SR & twihs::sr::NACK::mask) {
                    return Err(ErrorCode::CommunicationError);
                }
                if (++timeout > I2C_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }
            
            data[i] = static_cast<uint8_t>(hw->RHR);
        }
        
        uint32_t timeout = 0;
        while (!(hw->SR & twihs::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
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

        // Write to device register using internal address
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::IADRSZ::write(mmr, twihs::mmr::iadrsz::_1_BYTE);
        hw->MMR = mmr;
        hw->IADR = reg_addr;
        hw->THR = value;
        
        uint32_t timeout = 0;
        while (!(hw->SR & twihs::sr::TXRDY::mask)) {
            if (hw->SR & twihs::sr::NACK::mask) {
                return Err(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }
        
        hw->CR = twihs::cr::STOP::mask;
        
        timeout = 0;
        while (!(hw->SR & twihs::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
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

        // Read from device register using internal address
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::IADRSZ::write(mmr, twihs::mmr::iadrsz::_1_BYTE);
        mmr = twihs::mmr::MREAD::set(mmr);
        hw->MMR = mmr;
        hw->IADR = reg_addr;
        hw->CR = twihs::cr::START::mask | twihs::cr::STOP::mask;
        
        uint32_t timeout = 0;
        while (!(hw->SR & twihs::sr::RXRDY::mask)) {
            if (hw->SR & twihs::sr::NACK::mask) {
                return Err(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }
        
        *value = static_cast<uint8_t>(hw->RHR);
        
        timeout = 0;
        while (!(hw->SR & twihs::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Err(ErrorCode::Timeout);
            }
        }

        return Ok();
    }

    /**
     * @brief Check if I2C peripheral is open
     *
     * @return bool Check if I2C peripheral is open     */
    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
};

// ==============================================================================
// Predefined I2C Instances (from generated peripherals.hpp)
// ==============================================================================

constexpr uint32_t I2C0_BASE = alloy::generated::atsame70q21b::peripherals::TWIHS0;
constexpr uint32_t I2C0_IRQ = alloy::generated::atsame70q21b::id::TWIHS0;

constexpr uint32_t I2C1_BASE = alloy::generated::atsame70q21b::peripherals::TWIHS1;
constexpr uint32_t I2C1_IRQ = alloy::generated::atsame70q21b::id::TWIHS1;

constexpr uint32_t I2C2_BASE = alloy::generated::atsame70q21b::peripherals::TWIHS2;
constexpr uint32_t I2C2_IRQ = alloy::generated::atsame70q21b::id::TWIHS2;

using I2c0 = I2c<I2C0_BASE, I2C0_IRQ>;  ///< TWIHS0 instance (I2C0)
using I2c1 = I2c<I2C1_BASE, I2C1_IRQ>;  ///< TWIHS1 instance (I2C1)
using I2c2 = I2c<I2C2_BASE, I2C2_IRQ>;  ///< TWIHS2 instance (I2C2)

} // namespace alloy::hal::same70