/**
 * @file spi.hpp
 * @brief Template-based SPI implementation for SAME70 (Platform Layer)
 *
 * This file implements SPI peripheral using templates with ZERO virtual
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
 * Generator: generate_platform_spi.py
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
#include "hal/vendors/atmel/same70/registers/spi0_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/atmel/same70/bitfields/spi0_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"


namespace alloy::hal::same70 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::atmel::same70;

// Namespace alias for bitfield access
namespace spi = atmel::same70::spi0;

// Note: SPI uses common SpiMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1

// ============================================================================
// Platform-Specific Enums
// ============================================================================

/**
 * @brief SPI Chip Select (SAME70-specific)
 */
enum class SpiChipSelect : uint8_t {
    CS0 = 0,  ///< Chip select 0
    CS1 = 1,  ///< Chip select 1
    CS2 = 2,  ///< Chip select 2
    CS3 = 3,  ///< Chip select 3
};


/**
 * @brief Template-based SPI peripheral for SAME70
 *
 * This class provides a template-based SPI implementation with ZERO runtime
 * overhead. All peripheral configuration is resolved at compile-time.
 *
 * Template Parameters:
 * - BASE_ADDR: SPI peripheral base address
 * - IRQ_ID: SPI interrupt ID for clock enable
 *
 * Example usage:
 * @code
 * // Basic SPI usage example
 * using MySpi = Spi<SPI0_BASE, SPI0_IRQ>;
 * auto spi = MySpi{};
 * spi.open();
 * spi.configureChipSelect(SpiChipSelect::CS0, 10, SpiMode::Mode0);
 * uint8_t tx[] = {0x01, 0x02, 0x03};
 * uint8_t rx[3];
 * spi.transfer(tx, rx, 3, SpiChipSelect::CS0);
 * spi.close();
 * @endcode
 *
 * @tparam BASE_ADDR SPI peripheral base address
 * @tparam IRQ_ID SPI interrupt ID for clock enable
 */
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Spi {
public:
    // Compile-time constants
    static constexpr uint32_t base_addr = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    // Configuration constants
    static constexpr uint32_t SPI_TIMEOUT = 100000;  ///< SPI timeout in loop iterations (~10ms at 150MHz)
    static constexpr size_t STACK_BUFFER_SIZE = 256;  ///< Stack buffer size for dummy data in write/read operations

    /**
     * @brief Get SPI peripheral registers
     *
     * Returns pointer to SPI registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile atmel::same70::spi0::SPI0_Registers* get_hw() {
#ifdef ALLOY_SPI_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_SPI_MOCK_HW();
#else
        return reinterpret_cast<volatile atmel::same70::spi0::SPI0_Registers*>(BASE_ADDR);
#endif
    }

    constexpr Spi() = default;

    /**
     * @brief Initialize SPI peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> open() {
        auto* hw = get_hw();

        if (m_opened) {
            return Err(ErrorCode::AlreadyInitialized);
        }

        // Enable peripheral clock (PMC)
        // TODO: Enable peripheral clock via PMC

        hw->CR = spi::cr::SWRST::mask;  // Reset SPI

        // Configure Mode Register for master mode
        uint32_t mr = 0;
        mr = spi::mr::MSTR::write(mr, spi::mr::mstr::MASTER);
        mr = spi::mr::MODFDIS::set(mr);
        hw->MR = mr;

        hw->CR = spi::cr::SPIEN::mask;  // Enable SPI

        m_opened = true;

        return Ok();
    }

    /**
     * @brief Close SPI peripheral
     *
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> close() {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        hw->CR = spi::cr::SPIDIS::mask;  // Disable SPI

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure chip select timing and mode
     *
     * @param cs Chip select to configure
     * @param clock_divider SPI clock divider (1-255)
     * @param mode SPI mode (polarity and phase)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configureChipSelect(SpiChipSelect cs, uint8_t clock_divider, SpiMode mode = SpiMode::Mode0) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (clock_divider == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Configure chip select register with mode and clock divider
        uint8_t cs_num = static_cast<uint8_t>(cs);
        uint32_t csr_value = 0;
        
        // Configure SPI mode (CPOL and NCPHA)
        uint8_t mode_val = static_cast<uint8_t>(mode);
        if (mode_val & 0x02) {
            csr_value = spi::csr::CPOL::set(csr_value);
        }
        if (!(mode_val & 0x01)) {
            csr_value = spi::csr::NCPHA::set(csr_value);
        }
        
        // Chip select active after transfer
        csr_value = spi::csr::CSAAT::set(csr_value);
        
        // 8-bit transfers
        csr_value = spi::csr::BITS::write(csr_value, spi::csr::bits::_8_BIT);
        
        // Serial clock baud rate
        csr_value = spi::csr::SCBR::write(csr_value, clock_divider);
        
        hw->CSR[cs_num][0] = csr_value;

        return Ok();
    }

    /**
     * @brief Full-duplex SPI transfer (send and receive simultaneously)
     *
     * @param tx_data Transmit data buffer
     * @param rx_data Receive data buffer
     * @param size Number of bytes to transfer
     * @param cs Chip select to use
     * @return Result<size_t, ErrorCode> Full-duplex SPI transfer (send and receive simultaneously)     */
    Result<size_t, ErrorCode> transfer(const uint8_t* tx_data, uint8_t* rx_data, size_t size, SpiChipSelect cs) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (tx_data == nullptr || rx_data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Transfer data byte by byte with timeout
        uint8_t cs_num = static_cast<uint8_t>(cs);
        
        for (size_t i = 0; i < size; ++i) {
            // Wait for transmit data register empty
            uint32_t timeout = 0;
            while (!(hw->SR & spi::sr::TDRE::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }
            
            // Build TDR value with data and chip select
            uint32_t tdr_value = 0;
            tdr_value = spi::tdr::TD::write(tdr_value, tx_data[i]);
            tdr_value = spi::tdr::PCS::write(tdr_value, cs_num);
            hw->TDR = tdr_value;
            
            // Wait for receive data register full
            timeout = 0;
            while (!(hw->SR & spi::sr::RDRF::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }
            
            // Read received data
            rx_data[i] = static_cast<uint8_t>(spi::rdr::RD::read(hw->RDR));
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Write data to SPI (discard received data)
     *
     * @param data Data to write
     * @param size Number of bytes to write
     * @param cs Chip select to use
     * @return Result<size_t, ErrorCode> Write data to SPI (discard received data)     */
    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size, SpiChipSelect cs) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Use stack buffer for dummy RX data, or transfer byte-by-byte for large transfers
        uint8_t stack_buffer[STACK_BUFFER_SIZE];
        uint8_t* dummy = stack_buffer;
        
        if (size > STACK_BUFFER_SIZE) {
            // For large transfers, transfer byte-by-byte to avoid allocation
            for (size_t i = 0; i < size; ++i) {
                uint8_t rx_dummy;
                auto result = transfer(&data[i], &rx_dummy, 1, cs);
                if (!result.is_ok()) {
                    return Err(result.err());
                }
            }
            return Ok(size_t(size));
        }
        
        auto result = transfer(data, dummy, size, cs);
        if (!result.is_ok()) {
            return Err(result.err());
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Read data from SPI (send 0xFF dummy bytes)
     *
     * @param data Buffer for received data
     * @param size Number of bytes to read
     * @param cs Chip select to use
     * @return Result<size_t, ErrorCode> Read data from SPI (send 0xFF dummy bytes)     */
    Result<size_t, ErrorCode> read(uint8_t* data, size_t size, SpiChipSelect cs) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Use stack buffer for dummy TX data (0xFF), or transfer byte-by-byte for large transfers
        uint8_t stack_buffer[STACK_BUFFER_SIZE];
        uint8_t* dummy = stack_buffer;
        
        if (size > STACK_BUFFER_SIZE) {
            // For large transfers, transfer byte-by-byte
            for (size_t i = 0; i < size; ++i) {
                uint8_t tx_dummy = 0xFF;
                auto result = transfer(&tx_dummy, &data[i], 1, cs);
                if (!result.is_ok()) {
                    return Err(result.err());
                }
            }
            return Ok(size_t(size));
        }
        
        // Initialize dummy buffer with 0xFF for read
        for (size_t i = 0; i < size; ++i) {
            dummy[i] = 0xFF;
        }
        
        auto result = transfer(dummy, data, size, cs);
        if (!result.is_ok()) {
            return Err(result.err());
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Check if SPI peripheral is open
     *
     * @return bool Check if SPI peripheral is open     */
    bool isOpen() const {
        return m_opened;
    }

private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
};

// ==============================================================================
// Predefined SPI Instances (from generated peripherals.hpp)
// ==============================================================================

constexpr uint32_t SPI0_BASE = alloy::generated::atsame70q21b::peripherals::SPI0;
constexpr uint32_t SPI0_IRQ = alloy::generated::atsame70q21b::id::SPI0;

constexpr uint32_t SPI1_BASE = alloy::generated::atsame70q21b::peripherals::SPI1;
constexpr uint32_t SPI1_IRQ = alloy::generated::atsame70q21b::id::SPI1;

using Spi0 = Spi<SPI0_BASE, SPI0_IRQ>;  ///< SPI0 instance
using Spi1 = Spi<SPI1_BASE, SPI1_IRQ>;  ///< SPI1 instance

} // namespace alloy::hal::same70