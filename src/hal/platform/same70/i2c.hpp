/**
 * @file i2c.hpp
 * @brief Template-based I2C (TWIHS) implementation for SAME70 (ARM Cortex-M7)
 *
 * This file implements the I2C peripheral for SAME70 using templates
 * with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Master mode only (for simplicity)
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 * @note SAME70 uses TWIHS (Two Wire Interface High Speed) which is I2C compatible
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/twihs0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/twihs0_bitfields.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;
namespace twihs = alloy::hal::atmel::same70::twihs0;  // Alias for easier bitfield access

// Use common I2cSpeed from hal/types.hpp

/**
 * @brief Template-based I2C peripheral for SAME70
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class I2c {
public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    static inline volatile atmel::same70::twihs0::TWIHS0_Registers* get_hw() {
#ifdef ALLOY_I2C_MOCK_HW
        return ALLOY_I2C_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::twihs0::TWIHS0_Registers*>(BASE_ADDR);
#endif
    }

    constexpr I2c() = default;

    // I2C timeout (in loop iterations, ~10000 = ~1ms at 150MHz)
    static constexpr uint32_t I2C_TIMEOUT = 100000;  // ~10ms timeout

    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* hw = get_hw();

        // Enable peripheral clock using Clock class
        auto clock_result = Clock::enablePeripheralClock(IRQ_ID);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }
        hw->CR = atmel::same70::twihs0::cr::SWRST::mask;
        hw->CR = atmel::same70::twihs0::cr::MSDIS::mask | atmel::same70::twihs0::cr::SVDIS::mask;
        hw->CR = atmel::same70::twihs0::cr::MSEN::mask;

        m_opened = true;
        return Result<void>::ok();
    }

    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->CR = atmel::same70::twihs0::cr::MSDIS::mask;

        m_opened = false;
        return Result<void>::ok();
    }

    Result<void> setSpeed(I2cSpeed speed) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        uint32_t speed_hz = static_cast<uint32_t>(speed);
        uint32_t mck = Clock::getMasterClockFrequency();
        uint32_t div = (mck / (2 * speed_hz)) - 4;

        if (div > 255) {
            return Result<void>::error(ErrorCode::InvalidParameter);
        }

        // Configure clock waveform generator using type-safe bitfields
        uint32_t cwgr = 0;
        cwgr = twihs::cwgr::CLDIV::write(cwgr, div);
        cwgr = twihs::cwgr::CHDIV::write(cwgr, div);
        cwgr = twihs::cwgr::CKDIV::write(cwgr, 0);
        hw->CWGR = cwgr;

        return Result<void>::ok();
    }

    Result<size_t> write(uint8_t device_addr, const uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr || size == 0) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();

        // Configure Master Mode Register for write using type-safe bitfields
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        // MREAD = 0 (write), IADRSZ = 0 (no internal address)
        hw->MMR = mmr;

        for (size_t i = 0; i < size; ++i) {
            hw->THR = data[i];

            uint32_t timeout = 0;
            while (!(hw->SR & atmel::same70::twihs0::sr::TXRDY::mask)) {
                if (hw->SR & atmel::same70::twihs0::sr::NACK::mask) {
                    return Result<size_t>::error(ErrorCode::CommunicationError);
                }
                if (++timeout > I2C_TIMEOUT) {
                    return Result<size_t>::error(ErrorCode::Timeout);
                }
            }
        }

        hw->CR = atmel::same70::twihs0::cr::STOP::mask;

        uint32_t timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Result<size_t>::error(ErrorCode::Timeout);
            }
        }

        return Result<size_t>::ok(size);
    }

    Result<size_t> read(uint8_t device_addr, uint8_t* data, size_t size) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr || size == 0) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();

        // Configure Master Mode Register for read using type-safe bitfields
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::MREAD::set(mmr);  // Read mode
        // IADRSZ = 0 (no internal address)
        hw->MMR = mmr;

        if (size == 1) {
            hw->CR = atmel::same70::twihs0::cr::START::mask | atmel::same70::twihs0::cr::STOP::mask;
        } else {
            hw->CR = atmel::same70::twihs0::cr::START::mask;
        }

        for (size_t i = 0; i < size; ++i) {
            if (i == size - 1 && size > 1) {
                hw->CR = atmel::same70::twihs0::cr::STOP::mask;
            }

            uint32_t timeout = 0;
            while (!(hw->SR & atmel::same70::twihs0::sr::RXRDY::mask)) {
                if (hw->SR & atmel::same70::twihs0::sr::NACK::mask) {
                    return Result<size_t>::error(ErrorCode::CommunicationError);
                }
                if (++timeout > I2C_TIMEOUT) {
                    return Result<size_t>::error(ErrorCode::Timeout);
                }
            }

            data[i] = static_cast<uint8_t>(hw->RHR);
        }

        uint32_t timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Result<size_t>::error(ErrorCode::Timeout);
            }
        }

        return Result<size_t>::ok(size);
    }

    Result<void> writeRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t value) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();

        // Configure Master Mode Register for write with internal address
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::IADRSZ::write(mmr, twihs::mmr::iadrsz::_1_BYTE);
        // MREAD = 0 (write)
        hw->MMR = mmr;
        hw->IADR = reg_addr;
        hw->THR = value;

        uint32_t timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::TXRDY::mask)) {
            if (hw->SR & atmel::same70::twihs0::sr::NACK::mask) {
                return Result<void>::error(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                return Result<void>::error(ErrorCode::Timeout);
            }
        }

        hw->CR = atmel::same70::twihs0::cr::STOP::mask;

        timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Result<void>::error(ErrorCode::Timeout);
            }
        }

        return Result<void>::ok();
    }

    Result<void> readRegister(uint8_t device_addr, uint8_t reg_addr, uint8_t* value) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        if (value == nullptr) {
            return Result<void>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();

        // Configure Master Mode Register for read with internal address
        uint32_t mmr = 0;
        mmr = twihs::mmr::DADR::write(mmr, device_addr);
        mmr = twihs::mmr::IADRSZ::write(mmr, twihs::mmr::iadrsz::_1_BYTE);
        mmr = twihs::mmr::MREAD::set(mmr);  // Read mode
        hw->MMR = mmr;
        hw->IADR = reg_addr;
        hw->CR = atmel::same70::twihs0::cr::START::mask | atmel::same70::twihs0::cr::STOP::mask;

        uint32_t timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::RXRDY::mask)) {
            if (hw->SR & atmel::same70::twihs0::sr::NACK::mask) {
                return Result<void>::error(ErrorCode::CommunicationError);
            }
            if (++timeout > I2C_TIMEOUT) {
                return Result<void>::error(ErrorCode::Timeout);
            }
        }

        *value = static_cast<uint8_t>(hw->RHR);

        timeout = 0;
        while (!(hw->SR & atmel::same70::twihs0::sr::TXCOMP::mask)) {
            if (++timeout > I2C_TIMEOUT) {
                return Result<void>::error(ErrorCode::Timeout);
            }
        }

        return Result<void>::ok();
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
};

// ============================================================================
// Predefined I2C instances for SAME70
// ============================================================================

constexpr uint32_t TWIHS0_BASE = 0x40018000;
constexpr uint32_t TWIHS0_IRQ = 19;

constexpr uint32_t TWIHS1_BASE = 0x4001C000;
constexpr uint32_t TWIHS1_IRQ = 20;

constexpr uint32_t TWIHS2_BASE = 0x40060000;
constexpr uint32_t TWIHS2_IRQ = 41;

using I2c0 = I2c<TWIHS0_BASE, TWIHS0_IRQ>;
using I2c1 = I2c<TWIHS1_BASE, TWIHS1_IRQ>;
using I2c2 = I2c<TWIHS2_BASE, TWIHS2_IRQ>;

} // namespace alloy::hal::same70
