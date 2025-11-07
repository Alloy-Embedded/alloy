/**
 * @file spi.hpp  
 * @brief Template-based SPI implementation for SAME70 (ARM Cortex-M7)
 *
 * This file implements the SPI peripheral for SAME70 using templates
 * with ZERO virtual functions and ZERO runtime overhead.
 *
 * Design Principles:
 * - Template-based: Peripheral address and IRQ resolved at compile-time
 * - Zero overhead: Fully inlined, identical assembly to manual register access
 * - Type-safe: Strong typing prevents errors
 * - Master mode only (for simplicity)
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// Include SAME70 register definitions
#include "hal/vendors/atmel/same70/registers/spi0_registers.hpp"
#include "hal/vendors/atmel/same70/bitfields/spi0_bitfields.hpp"
#include "hal/platform/same70/clock.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;
namespace spi = atmel::same70::spi0;  // Alias for easier bitfield access

// Use common SpiMode from hal/types.hpp

/**
 * @brief SPI Chip Select (SAME70-specific)
 */
enum class SpiChipSelect : uint8_t {
    CS0 = 0,
    CS1 = 1,
    CS2 = 2,
    CS3 = 3,
};

/**
 * @brief Template-based SPI peripheral for SAME70
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Spi {
public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    static inline volatile atmel::same70::spi0::SPI0_Registers* get_hw() {
#ifdef ALLOY_SPI_MOCK_HW
        return ALLOY_SPI_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::spi0::SPI0_Registers*>(BASE_ADDR);
#endif
    }

    constexpr Spi() = default;

    // SPI timeout (in loop iterations, ~10000 = ~1ms at 150MHz)
    static constexpr uint32_t SPI_TIMEOUT = 100000;  // ~10ms timeout

    Result<void> open() {
        if (m_opened) {
            return Result<void>::error(ErrorCode::AlreadyInitialized);
        }

        auto* hw = get_hw();

        // Enable peripheral clock using Clock class (replaces hardcoded PMC access)
        auto clock_result = Clock::enablePeripheralClock(IRQ_ID);
        if (!clock_result.is_ok()) {
            return Result<void>::error(clock_result.error());
        }

        // Reset SPI
        hw->CR = spi::cr::SWRST::mask;

        // Configure Mode Register using type-safe bitfields
        uint32_t mr = 0;
        mr = spi::mr::MSTR::write(mr, spi::mr::mstr::MASTER);  // Master mode
        // PS = 0 (fixed peripheral select)
        // PCSDEC = 0 (chip selects directly connected)
        mr = spi::mr::MODFDIS::set(mr);  // Mode fault detection disabled
        // LLB = 0 (loopback disabled)
        hw->MR = mr;

        // Enable SPI
        hw->CR = spi::cr::SPIEN::mask;

        m_opened = true;
        return Result<void>::ok();
    }

    Result<void> close() {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        auto* hw = get_hw();
        hw->CR = atmel::same70::spi0::cr::SPIDIS::mask;

        m_opened = false;
        return Result<void>::ok();
    }

    Result<void> configureChipSelect(SpiChipSelect cs, uint8_t clock_divider,
                                      SpiMode mode = SpiMode::Mode0) {
        if (!m_opened) {
            return Result<void>::error(ErrorCode::NotInitialized);
        }

        if (clock_divider == 0) {
            return Result<void>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();
        uint8_t cs_num = static_cast<uint8_t>(cs);
        uint32_t csr_value = 0;

        // Configure SPI mode (clock polarity and phase) using type-safe bitfields
        // SPI Mode 0: CPOL=0, NCPHA=1 (capture on leading edge, change on trailing)
        // SPI Mode 1: CPOL=0, NCPHA=0 (change on leading edge, capture on trailing)
        // SPI Mode 2: CPOL=1, NCPHA=1
        // SPI Mode 3: CPOL=1, NCPHA=0
        uint8_t mode_val = static_cast<uint8_t>(mode);
        if (mode_val & 0x02) {  // CPOL bit
            csr_value = spi::csr::CPOL::set(csr_value);
        }
        if (!(mode_val & 0x01)) {  // CPHA bit (inverted as NCPHA)
            csr_value = spi::csr::NCPHA::set(csr_value);
        }

        // Chip select active after transfer
        csr_value = spi::csr::CSAAT::set(csr_value);

        // 8-bit transfers
        csr_value = spi::csr::BITS::write(csr_value, spi::csr::bits::_8_BIT);

        // Serial clock baud rate
        csr_value = spi::csr::SCBR::write(csr_value, clock_divider);

        hw->CSR[cs_num][0] = csr_value;

        return Result<void>::ok();
    }

    Result<size_t> transfer(const uint8_t* tx_data, uint8_t* rx_data,
                            size_t size, SpiChipSelect cs) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (tx_data == nullptr || rx_data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        auto* hw = get_hw();
        uint8_t cs_num = static_cast<uint8_t>(cs);

        for (size_t i = 0; i < size; ++i) {
            // Wait for transmit data register empty
            uint32_t timeout = 0;
            while (!(hw->SR & spi::sr::TDRE::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Result<size_t>::error(ErrorCode::Timeout);
                }
            }

            // Build TDR value with data and chip select using type-safe bitfields
            uint32_t tdr_value = 0;
            tdr_value = spi::tdr::TD::write(tdr_value, tx_data[i]);
            tdr_value = spi::tdr::PCS::write(tdr_value, cs_num);
            hw->TDR = tdr_value;

            // Wait for receive data register full
            timeout = 0;
            while (!(hw->SR & spi::sr::RDRF::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Result<size_t>::error(ErrorCode::Timeout);
                }
            }

            // Read received data (lower 8 bits)
            rx_data[i] = static_cast<uint8_t>(spi::rdr::RD::read(hw->RDR));
        }

        return Result<size_t>::ok(size);
    }

    Result<size_t> write(const uint8_t* data, size_t size, SpiChipSelect cs) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        // Use stack buffer for small transfers, heap for large ones
        constexpr size_t STACK_BUFFER_SIZE = 256;
        uint8_t stack_buffer[STACK_BUFFER_SIZE];
        uint8_t* dummy = stack_buffer;

        // For larger transfers, we could allocate on heap, but for embedded systems
        // it's better to fail gracefully or do byte-by-byte transfers
        if (size > STACK_BUFFER_SIZE) {
            // For large transfers, just transfer byte-by-byte to avoid allocation
            for (size_t i = 0; i < size; ++i) {
                uint8_t rx_dummy;
                auto result = transfer(&data[i], &rx_dummy, 1, cs);
                if (!result.is_ok()) {
                    return Result<size_t>::error(result.error());
                }
            }
            return Result<size_t>::ok(size);
        }

        return transfer(data, dummy, size, cs);
    }

    Result<size_t> read(uint8_t* data, size_t size, SpiChipSelect cs) {
        if (!m_opened) {
            return Result<size_t>::error(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Result<size_t>::error(ErrorCode::InvalidParameter);
        }

        // Use stack buffer for small transfers
        constexpr size_t STACK_BUFFER_SIZE = 256;
        uint8_t stack_buffer[STACK_BUFFER_SIZE];
        uint8_t* dummy = stack_buffer;

        if (size > STACK_BUFFER_SIZE) {
            // For large transfers, do byte-by-byte to avoid allocation
            for (size_t i = 0; i < size; ++i) {
                uint8_t tx_dummy = 0xFF;
                auto result = transfer(&tx_dummy, &data[i], 1, cs);
                if (!result.is_ok()) {
                    return Result<size_t>::error(result.error());
                }
            }
            return Result<size_t>::ok(size);
        }

        // Initialize dummy buffer with 0xFF for read
        for (size_t i = 0; i < size; ++i) {
            dummy[i] = 0xFF;
        }

        return transfer(dummy, data, size, cs);
    }

    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;
};

// ============================================================================
// Predefined SPI instances for SAME70
// ============================================================================

constexpr uint32_t SPI0_BASE = 0x40008000;
constexpr uint32_t SPI0_IRQ = 21;

constexpr uint32_t SPI1_BASE = 0x40058000;
constexpr uint32_t SPI1_IRQ = 42;

using Spi0 = Spi<SPI0_BASE, SPI0_IRQ>;
using Spi1 = Spi<SPI1_BASE, SPI1_IRQ>;

} // namespace alloy::hal::same70
