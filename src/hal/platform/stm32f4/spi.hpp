/**
 * @file spi.hpp
 * @brief Template-based SPI implementation for STM32F4 (Platform Layer)
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
 * Auto-generated from: stm32f4
 * Generator: generate_platform_spi.py
 * Generated: 2025-11-07 17:27:07
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
#include "hal/vendors/st/stm32f4/registers/spi1_registers.hpp"

// Bitfields (family-level)
#include "hal/vendors/st/stm32f4/bitfields/spi1_bitfields.hpp"


namespace alloy::hal::stm32f4 {

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types
using namespace alloy::hal::st::stm32f4;

// Namespace alias for bitfield access
namespace spi = alloy::hal::st::stm32f4::spi1;

// Note: SPI uses common SpiMode from hal/types.hpp:
// - Mode0: CPOL=0, CPHA=0
// - Mode1: CPOL=0, CPHA=1
// - Mode2: CPOL=1, CPHA=0
// - Mode3: CPOL=1, CPHA=1


/**
 * @brief Template-based SPI peripheral for STM32F4
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
 * using MySpi = Spi<SPI1_BASE, SPI1_IRQ>;
 * auto spi = MySpi{};
 * spi.open();
 * spi.configure(SpiMode::Mode0, 3);
 * uint8_t tx[] = {0x01, 0x02, 0x03};
 * uint8_t rx[3];
 * spi.transfer(tx, rx, 3);
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
    static constexpr uint32_t SPI_TIMEOUT =
        100000;  ///< SPI timeout in loop iterations (~10ms at 168MHz)
    static constexpr size_t STACK_BUFFER_SIZE =
        256;  ///< Stack buffer size for dummy data in write/read operations

    /**
     * @brief Get SPI peripheral registers
     *
     * Returns pointer to SPI registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile alloy::hal::st::stm32f4::spi1::SPI1_Registers* get_hw() {
#ifdef ALLOY_SPI_MOCK_HW
        // In tests, use the mock hardware pointer
        return ALLOY_SPI_MOCK_HW();
#else
        return reinterpret_cast<volatile alloy::hal::st::stm32f4::spi1::SPI1_Registers*>(BASE_ADDR);
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

        // Enable peripheral clock (RCC)
        // TODO: Enable peripheral clock via RCC

        // Configure CR1 for master mode
        uint32_t cr1 = 0;
        cr1 = spi::cr1::MSTR::set(cr1);       // Master mode
        cr1 = spi::cr1::SSM::set(cr1);        // Software slave management
        cr1 = spi::cr1::SSI::set(cr1);        // Internal slave select
        cr1 = spi::cr1::BR::write(cr1, 0x3);  // Default baud rate = f_PCLK/16
        hw->CR1 = cr1;

        // Enable SPI
        hw->CR1 |= spi::cr1::SPE::mask;

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

        // Disable SPI
        hw->CR1 &= ~spi::cr1::SPE::mask;

        m_opened = false;

        return Ok();
    }

    /**
     * @brief Configure SPI mode and baud rate
     *
     * @param mode SPI mode (polarity and phase)
     * @param baud_rate_div Baud rate divider (0-7: /2, /4, /8, /16, /32, /64, /128, /256)
     * @return Result<void, ErrorCode>     */
    Result<void, ErrorCode> configure(SpiMode mode, uint8_t baud_rate_div) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (baud_rate_div > 7) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Disable SPI before configuration
        hw->CR1 &= ~spi::cr1::SPE::mask;

        // Configure mode and baud rate
        uint32_t cr1 = hw->CR1;

        // Configure SPI mode (CPOL and CPHA)
        uint8_t mode_val = static_cast<uint8_t>(mode);
        if (mode_val & 0x02) {  // CPOL bit
            cr1 = spi::cr1::CPOL::set(cr1);
        } else {
            cr1 = spi::cr1::CPOL::clear(cr1);
        }
        if (mode_val & 0x01) {  // CPHA bit
            cr1 = spi::cr1::CPHA::set(cr1);
        } else {
            cr1 = spi::cr1::CPHA::clear(cr1);
        }

        // Configure baud rate
        cr1 = spi::cr1::BR::write(cr1, baud_rate_div);

        hw->CR1 = cr1;

        // Re-enable SPI
        hw->CR1 |= spi::cr1::SPE::mask;

        return Ok();
    }

    /**
     * @brief Full-duplex SPI transfer (send and receive simultaneously)
     *
     * @param tx_data Transmit data buffer
     * @param rx_data Receive data buffer
     * @param size Number of bytes to transfer
     * @return Result<size_t, ErrorCode> Full-duplex SPI transfer (send and receive simultaneously)
     */
    Result<size_t, ErrorCode> transfer(const uint8_t* tx_data, uint8_t* rx_data, size_t size) {
        auto* hw = get_hw();

        if (!m_opened) {
            return Err(ErrorCode::NotInitialized);
        }

        if (tx_data == nullptr || rx_data == nullptr) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Transfer data byte by byte with timeout
        for (size_t i = 0; i < size; ++i) {
            // Wait for transmit buffer empty
            uint32_t timeout = 0;
            while (!(hw->SR & spi::sr::TXE::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }

            // Write data to DR
            hw->DR = tx_data[i];

            // Wait for receive buffer not empty
            timeout = 0;
            while (!(hw->SR & spi::sr::RXNE::mask)) {
                if (++timeout > SPI_TIMEOUT) {
                    return Err(ErrorCode::Timeout);
                }
            }

            // Read received data from DR
            rx_data[i] = static_cast<uint8_t>(hw->DR);
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Write data to SPI (discard received data)
     *
     * @param data Data to write
     * @param size Number of bytes to write
     * @return Result<size_t, ErrorCode> Write data to SPI (discard received data)     */
    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size) {
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
                auto result = transfer(&data[i], &rx_dummy, 1);
                if (!result.is_ok()) {
                    return Err(result.err());
                }
            }
            return Ok(size_t(size));
        }

        auto result = transfer(data, dummy, size);
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
     * @return Result<size_t, ErrorCode> Read data from SPI (send 0xFF dummy bytes)     */
    Result<size_t, ErrorCode> read(uint8_t* data, size_t size) {
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
                auto result = transfer(&tx_dummy, &data[i], 1);
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

        auto result = transfer(dummy, data, size);
        if (!result.is_ok()) {
            return Err(result.err());
        }

        return Ok(size_t(size));
    }

    /**
     * @brief Check if SPI peripheral is open
     *
     * @return bool Check if SPI peripheral is open     */
    bool isOpen() const { return m_opened; }

   private:
    bool m_opened = false;  ///< Tracks if peripheral is initialized
};

// ==============================================================================
// Predefined SPI Instances
// ==============================================================================

constexpr uint32_t SPI1_BASE = 0x40013000;
constexpr uint32_t SPI1_IRQ = 35;

constexpr uint32_t SPI2_BASE = 0x40003800;
constexpr uint32_t SPI2_IRQ = 36;

constexpr uint32_t SPI3_BASE = 0x40003C00;
constexpr uint32_t SPI3_IRQ = 51;

using Spi1 = Spi<SPI1_BASE, SPI1_IRQ>;  ///< SPI1 instance
using Spi2 = Spi<SPI2_BASE, SPI2_IRQ>;  ///< SPI2 instance
using Spi3 = Spi<SPI3_BASE, SPI3_IRQ>;  ///< SPI3 instance

}  // namespace alloy::hal::stm32f4
