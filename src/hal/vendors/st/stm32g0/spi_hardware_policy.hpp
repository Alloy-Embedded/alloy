/**
 * @file spi_hardware_policy.hpp
 * @brief Hardware Policy for SPI on STM32G0 (Policy-Based Design)
 *
 * This file provides platform-specific hardware access for SPI using
 * the Policy-Based Design pattern. All methods are static inline for
 * zero runtime overhead.
 *
 * Design Pattern: Policy-Based Design
 * - Generic APIs accept this policy as template parameter
 * - All methods are static inline (zero overhead)
 * - Direct register access with compile-time addresses
 * - Mock hooks for testing (#ifdef ALLOY_SPI_MOCK_HW)
 *
 * Auto-generated from: stm32g0/spi.json
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
#include "hal/vendors/st/stm32g0/generated/registers/spi1_registers.hpp"

// Bitfield definitions
#include "hal/vendors/st/stm32g0/generated/bitfields/spi1_bitfields.hpp"

// Peripheral addresses (generated from SVD)
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

// Import register types
using namespace alloy::hal::st::stm32g0::spi1;

/**
 * @brief Hardware Policy for SPI on STM32G0
 *
 * This policy provides all platform-specific hardware access methods
 * for SPI. It is designed to be used as a template
 * parameter in generic SPI implementations.
 *
 * Template Parameters:
 * - BASE_ADDR: Peripheral base address
 * - PERIPH_CLOCK_HZ: Peripheral clock frequency in Hz
 *
 * @tparam BASE_ADDR Peripheral base address
 * @tparam PERIPH_CLOCK_HZ Peripheral clock frequency in Hz
 */
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0SPIHardwarePolicy {
    // ========================================================================
    // Type Definitions
    // ========================================================================

    using RegisterType = SPI1_Registers;

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
        #ifdef ALLOY_SPI_MOCK_HW
            return ALLOY_SPI_MOCK_HW();  // Test hook
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
     * @brief Enable SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_SPE
     */
    static inline void enable_spi() {
        #ifdef ALLOY_SPI_TEST_HOOK_SPE
            ALLOY_SPI_TEST_HOOK_SPE();
        #endif

        hw()->SPI_CR1 |= (1U << 6);
    }

    /**
     * @brief Disable SPI peripheral
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_SPE
     */
    static inline void disable_spi() {
        #ifdef ALLOY_SPI_TEST_HOOK_SPE
            ALLOY_SPI_TEST_HOOK_SPE();
        #endif

        hw()->SPI_CR1 &= ~(1U << 6);
    }

    /**
     * @brief Configure as SPI master
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_MSTR
     */
    static inline void set_master_mode() {
        #ifdef ALLOY_SPI_TEST_HOOK_MSTR
            ALLOY_SPI_TEST_HOOK_MSTR();
        #endif

        hw()->SPI_CR1 |= (1U << 2);
    }

    /**
     * @brief Configure as SPI slave
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_MSTR
     */
    static inline void set_slave_mode() {
        #ifdef ALLOY_SPI_TEST_HOOK_MSTR
            ALLOY_SPI_TEST_HOOK_MSTR();
        #endif

        hw()->SPI_CR1 &= ~(1U << 2);
    }

    /**
     * @brief Set clock polarity to low when idle
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_CPOL
     */
    static inline void set_clock_polarity_low() {
        #ifdef ALLOY_SPI_TEST_HOOK_CPOL
            ALLOY_SPI_TEST_HOOK_CPOL();
        #endif

        hw()->SPI_CR1 &= ~(1U << 1);
    }

    /**
     * @brief Set clock polarity to high when idle
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_CPOL
     */
    static inline void set_clock_polarity_high() {
        #ifdef ALLOY_SPI_TEST_HOOK_CPOL
            ALLOY_SPI_TEST_HOOK_CPOL();
        #endif

        hw()->SPI_CR1 |= (1U << 1);
    }

    /**
     * @brief Data captured on first clock edge
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_CPHA
     */
    static inline void set_clock_phase_first_edge() {
        #ifdef ALLOY_SPI_TEST_HOOK_CPHA
            ALLOY_SPI_TEST_HOOK_CPHA();
        #endif

        hw()->SPI_CR1 &= ~(1U << 0);
    }

    /**
     * @brief Data captured on second clock edge
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_CPHA
     */
    static inline void set_clock_phase_second_edge() {
        #ifdef ALLOY_SPI_TEST_HOOK_CPHA
            ALLOY_SPI_TEST_HOOK_CPHA();
        #endif

        hw()->SPI_CR1 |= (1U << 0);
    }

    /**
     * @brief Set baud rate prescaler (0=/2, 1=/4, 2=/8, ..., 7=/256)
     * @param prescaler Prescaler value (0-7)
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_BR
     */
    static inline void set_baud_rate_prescaler(uint8_t prescaler) {
        #ifdef ALLOY_SPI_TEST_HOOK_BR
            ALLOY_SPI_TEST_HOOK_BR(prescaler);
        #endif

        hw()->SPI_CR1 = (hw()->SPI_CR1 & ~(0x7U << 3)) | ((prescaler & 0x7U) << 3);
    }

    /**
     * @brief Set data frame format to 8 bits
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_DS
     */
    static inline void set_data_size_8bit() {
        #ifdef ALLOY_SPI_TEST_HOOK_DS
            ALLOY_SPI_TEST_HOOK_DS();
        #endif

        hw()->SPI_CR2 &= ~(0xFU << 8);\nhw()->SPI_CR2 |= (0x7U << 8);
    }

    /**
     * @brief Set data frame format to 16 bits
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_DS
     */
    static inline void set_data_size_16bit() {
        #ifdef ALLOY_SPI_TEST_HOOK_DS
            ALLOY_SPI_TEST_HOOK_DS();
        #endif

        hw()->SPI_CR2 &= ~(0xFU << 8);\nhw()->SPI_CR2 |= (0xFU << 8);
    }

    /**
     * @brief Transmit MSB first
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_LSBFIRST
     */
    static inline void set_msb_first() {
        #ifdef ALLOY_SPI_TEST_HOOK_LSBFIRST
            ALLOY_SPI_TEST_HOOK_LSBFIRST();
        #endif

        hw()->SPI_CR1 &= ~(1U << 7);
    }

    /**
     * @brief Transmit LSB first
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_LSBFIRST
     */
    static inline void set_lsb_first() {
        #ifdef ALLOY_SPI_TEST_HOOK_LSBFIRST
            ALLOY_SPI_TEST_HOOK_LSBFIRST();
        #endif

        hw()->SPI_CR1 |= (1U << 7);
    }

    /**
     * @brief Write data to SPI data register
     * @param data Data to transmit
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_DR
     */
    static inline void write_data(uint16_t data) {
        #ifdef ALLOY_SPI_TEST_HOOK_DR
            ALLOY_SPI_TEST_HOOK_DR(data);
        #endif

        hw()->SPI_DR = data;
    }

    /**
     * @brief Read data from SPI data register
     * @return uint16_t
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_DR
     */
    static inline uint16_t read_data() {
        #ifdef ALLOY_SPI_TEST_HOOK_DR
            ALLOY_SPI_TEST_HOOK_DR();
        #endif

        return hw()->SPI_DR;
    }

    /**
     * @brief Check if transmit buffer is empty
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_TXE
     */
    static inline bool is_tx_buffer_empty() {
        #ifdef ALLOY_SPI_TEST_HOOK_TXE
            ALLOY_SPI_TEST_HOOK_TXE();
        #endif

        return (hw()->SPI_SR & (1U << 1)) != 0;
    }

    /**
     * @brief Check if receive buffer has data
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RXNE
     */
    static inline bool is_rx_buffer_not_empty() {
        #ifdef ALLOY_SPI_TEST_HOOK_RXNE
            ALLOY_SPI_TEST_HOOK_RXNE();
        #endif

        return (hw()->SPI_SR & (1U << 0)) != 0;
    }

    /**
     * @brief Check if SPI is busy transmitting/receiving
     * @return bool
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_BSY
     */
    static inline bool is_busy() {
        #ifdef ALLOY_SPI_TEST_HOOK_BSY
            ALLOY_SPI_TEST_HOOK_BSY();
        #endif

        return (hw()->SPI_SR & (1U << 7)) != 0;
    }

    /**
     * @brief Enable DMA for reception
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RXDMAEN
     */
    static inline void enable_rx_dma() {
        #ifdef ALLOY_SPI_TEST_HOOK_RXDMAEN
            ALLOY_SPI_TEST_HOOK_RXDMAEN();
        #endif

        hw()->SPI_CR2 |= (1U << 0);
    }

    /**
     * @brief Enable DMA for transmission
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_TXDMAEN
     */
    static inline void enable_tx_dma() {
        #ifdef ALLOY_SPI_TEST_HOOK_TXDMAEN
            ALLOY_SPI_TEST_HOOK_TXDMAEN();
        #endif

        hw()->SPI_CR2 |= (1U << 1);
    }

    /**
     * @brief Disable DMA for reception
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_RXDMAEN
     */
    static inline void disable_rx_dma() {
        #ifdef ALLOY_SPI_TEST_HOOK_RXDMAEN
            ALLOY_SPI_TEST_HOOK_RXDMAEN();
        #endif

        hw()->SPI_CR2 &= ~(1U << 0);
    }

    /**
     * @brief Disable DMA for transmission
     *
     * @note Test hook: ALLOY_SPI_TEST_HOOK_TXDMAEN
     */
    static inline void disable_tx_dma() {
        #ifdef ALLOY_SPI_TEST_HOOK_TXDMAEN
            ALLOY_SPI_TEST_HOOK_TXDMAEN();
        #endif

        hw()->SPI_CR2 &= ~(1U << 1);
    }

};

// ============================================================================
// Type Aliases for Common Instances
// ============================================================================


}  // namespace alloy::hal::st::stm32g0

/**
 * @example
 * Using the hardware policy with generic SPI API:
 *
 * @code
 * #include "hal/api/spi_simple.hpp"
 * #include "hal/vendors/st/stm32g0/spi_hardware_policy.hpp"
 *
 * using namespace alloy::hal;
 * using namespace alloy::hal::st::stm32g0;
 *
 * // Create SPI with hardware policy
 * using Uart0 = UartImpl<Stm32g0SPIHardwarePolicy<UART0_BASE, 150000000>>;
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