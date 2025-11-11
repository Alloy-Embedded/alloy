/**
 * @file spi_dma.hpp
 * @brief SPI with DMA Integration
 *
 * Provides type-safe SPI with DMA support, building on the
 * multi-level API infrastructure.
 *
 * Design Principles:
 * - Type-safe DMA channel allocation
 * - Compile-time conflict detection
 * - Automatic peripheral address setup
 * - Integration with SPI Expert API
 * - Zero runtime overhead for configuration
 *
 * Example Usage:
 * @code
 * // Define DMA connections for SPI0
 * using Spi0TxDma = DmaConnection<
 *     PeripheralId::SPI0,
 *     DmaRequest::SPI0_TX,
 *     DmaStream::Stream3
 * >;
 * using Spi0RxDma = DmaConnection<
 *     PeripheralId::SPI0,
 *     DmaRequest::SPI0_RX,
 *     DmaStream::Stream2
 * >;
 *
 * // Create SPI with DMA configuration
 * constexpr auto config = SpiDmaConfig<Spi0TxDma, Spi0RxDma>::create(
 *     PinId::PA7,  // MOSI
 *     PinId::PA6,  // MISO
 *     PinId::PA5,  // SCK
 *     2000000      // 2 MHz
 * );
 *
 * // Validate at compile-time
 * static_assert(config.is_valid(), config.error_message());
 *
 * // Transfer with DMA
 * auto result = spi_dma_transfer<Spi0TxDma, Spi0RxDma>(tx_buffer, rx_buffer, size);
 * @endcode
 *
 * @note Part of Phase 6.2: SPI with DMA Integration
 * @see openspec/changes/modernize-peripheral-architecture/specs/dma-integration/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/dma_config.hpp"
#include "hal/dma_connection.hpp"
#include "hal/dma_registry.hpp"
#include "hal/api/spi_expert.hpp"

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// SPI DMA Configuration
// ============================================================================

/**
 * @brief SPI configuration with DMA support
 *
 * Extends SpiExpertConfig with type-safe DMA channel allocation.
 *
 * @tparam TxDmaConnection DMA connection for TX (can be void for RX-only)
 * @tparam RxDmaConnection DMA connection for RX (can be void for TX-only)
 */
template <typename TxDmaConnection = void, typename RxDmaConnection = void>
struct SpiDmaConfig {
    SpiExpertConfig spi_config;

    // Type aliases for DMA connections
    using TxDma = TxDmaConnection;
    using RxDma = RxDmaConnection;

    /**
     * @brief Check if TX DMA is enabled
     */
    static constexpr bool has_tx_dma() {
        return !std::is_void_v<TxDmaConnection>;
    }

    /**
     * @brief Check if RX DMA is enabled
     */
    static constexpr bool has_rx_dma() {
        return !std::is_void_v<RxDmaConnection>;
    }

    /**
     * @brief Create SPI DMA configuration
     *
     * Factory method for creating a validated configuration.
     *
     * @param mosi_pin MOSI pin ID
     * @param miso_pin MISO pin ID
     * @param sck_pin SCK pin ID
     * @param clock_speed Clock speed in Hz
     * @param mode SPI mode
     * @return SPI DMA configuration
     */
    static constexpr SpiDmaConfig create(
        PinId mosi_pin,
        PinId miso_pin,
        PinId sck_pin,
        u32 clock_speed,
        SpiMode mode = SpiMode::Mode0) {

        // Validate DMA connections at compile-time
        if constexpr (has_tx_dma()) {
            static_assert(TxDmaConnection::is_compatible(),
                         "Invalid TX DMA connection");
        }

        if constexpr (has_rx_dma()) {
            static_assert(RxDmaConnection::is_compatible(),
                         "Invalid RX DMA connection");
        }

        // Get peripheral ID from DMA connection
        constexpr PeripheralId peripheral = []() {
            if constexpr (has_tx_dma()) {
                return TxDmaConnection::peripheral;
            } else if constexpr (has_rx_dma()) {
                return RxDmaConnection::peripheral;
            } else {
                return PeripheralId::SPI0; // Default
            }
        }();

        return SpiDmaConfig{
            .spi_config = {
                .peripheral = peripheral,
                .mosi_pin = mosi_pin,
                .miso_pin = miso_pin,
                .sck_pin = sck_pin,
                .nss_pin = PinId::PA0,  // Unused
                .mode = mode,
                .clock_speed = clock_speed,
                .bit_order = SpiBitOrder::MsbFirst,
                .data_size = SpiDataSize::Bits8,
                .enable_mosi = has_tx_dma() || mosi_pin != PinId::PA0,
                .enable_miso = has_rx_dma() || miso_pin != PinId::PA0,
                .enable_nss = false,
                .enable_interrupts = has_tx_dma() || has_rx_dma(), // DMA needs interrupts
                .enable_dma_tx = has_tx_dma(),
                .enable_dma_rx = has_rx_dma(),
                .enable_crc = false,
                .crc_polynomial = 0,
                .enable_ti_mode = false,
                .enable_motorola = true
            }
        };
    }

    /**
     * @brief Validate configuration
     *
     * Checks both SPI and DMA configurations.
     *
     * @return true if valid, false otherwise
     */
    constexpr bool is_valid() const {
        // Check SPI configuration
        if (!spi_config.is_valid()) {
            return false;
        }

        // Check that DMA is enabled if connections are provided
        if constexpr (has_tx_dma()) {
            if (!spi_config.enable_dma_tx) {
                return false;
            }
        }

        if constexpr (has_rx_dma()) {
            if (!spi_config.enable_dma_rx) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Get error message
     *
     * @return Error message string
     */
    constexpr const char* error_message() const {
        if (!spi_config.is_valid()) {
            return spi_config.error_message();
        }

        if constexpr (has_tx_dma()) {
            if (!spi_config.enable_dma_tx) {
                return "TX DMA connection provided but DMA TX not enabled";
            }
        }

        if constexpr (has_rx_dma()) {
            if (!spi_config.enable_dma_rx) {
                return "RX DMA connection provided but DMA RX not enabled";
            }
        }

        return "Valid SPI DMA configuration";
    }
};

// ============================================================================
// SPI DMA Operations
// ============================================================================

/**
 * @brief Transfer data via SPI using DMA (full-duplex)
 *
 * Type-safe DMA transfer with automatic channel setup.
 *
 * @tparam TxConnection The TX DMA connection
 * @tparam RxConnection The RX DMA connection
 * @param tx_data Buffer containing data to transmit
 * @param rx_data Buffer to store received data
 * @param size Number of bytes to transfer
 * @return Result indicating success or error
 */
template <typename TxConnection, typename RxConnection>
inline Result<void, ErrorCode> spi_dma_transfer(
    const void* tx_data,
    void* rx_data,
    usize size) {

    // Validate connections at compile-time
    static_assert(TxConnection::is_compatible(), "Invalid DMA connection");
    static_assert(RxConnection::is_compatible(), "Invalid DMA connection");

    // Create DMA configurations
    auto tx_config = DmaTransferConfig<TxConnection>::memory_to_peripheral(
        tx_data, size, DmaDataWidth::Bits8);
    auto rx_config = DmaTransferConfig<RxConnection>::peripheral_to_memory(
        rx_data, size, DmaDataWidth::Bits8);

    // Validate
    auto tx_validation = tx_config.validate();
    if (!tx_validation.is_ok()) {
        ErrorCode error_copy = tx_validation.error();
        return Err(std::move(error_copy));
    }

    auto rx_validation = rx_config.validate();
    if (!rx_validation.is_ok()) {
        ErrorCode error_copy = rx_validation.error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfers
    // - Configure DMA streams for both TX and RX
    // - Enable SPI DMA requests
    // - Start transfers simultaneously
    // - Wait for completion or use callbacks

    return Ok();
}

/**
 * @brief Transmit data via SPI using DMA (TX-only)
 *
 * Type-safe DMA transmission with automatic channel setup.
 *
 * @tparam Connection The TX DMA connection
 * @param data Buffer containing data to transmit
 * @param size Number of bytes to transmit
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> spi_dma_transmit(const void* data, usize size) {
    // Validate connection at compile-time
    static_assert(Connection::is_compatible(), "Invalid DMA connection");

    // Create DMA configuration
    auto dma_config = DmaTransferConfig<Connection>::memory_to_peripheral(
        data, size, DmaDataWidth::Bits8);

    // Validate
    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = validation.error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    // - Configure DMA stream
    // - Set source address
    // - Set transfer size
    // - Enable DMA channel
    // - Enable SPI TX DMA request

    return Ok();
}

/**
 * @brief Receive data via SPI using DMA (RX-only)
 *
 * Type-safe DMA reception with automatic channel setup.
 *
 * @tparam Connection The RX DMA connection
 * @param buffer Buffer to store received data
 * @param size Number of bytes to receive
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> spi_dma_receive(void* buffer, usize size) {
    // Validate connection at compile-time
    static_assert(Connection::is_compatible(), "Invalid DMA connection");

    // Create DMA configuration
    auto dma_config = DmaTransferConfig<Connection>::peripheral_to_memory(
        buffer, size, DmaDataWidth::Bits8);

    // Validate
    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = validation.error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    // - Configure DMA stream
    // - Set destination address
    // - Set transfer size
    // - Enable DMA channel
    // - Enable SPI RX DMA request

    return Ok();
}

/**
 * @brief Stop SPI TX DMA transfer
 *
 * @tparam Connection The TX DMA connection
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> spi_dma_stop_tx() {
    // TODO: Stop DMA transfer
    // - Disable DMA channel
    // - Disable SPI TX DMA request
    // - Clear any pending interrupts

    return Ok();
}

/**
 * @brief Stop SPI RX DMA transfer
 *
 * @tparam Connection The RX DMA connection
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> spi_dma_stop_rx() {
    // TODO: Stop DMA transfer
    // - Disable DMA channel
    // - Disable SPI RX DMA request
    // - Clear any pending interrupts

    return Ok();
}

// ============================================================================
// Preset Configurations
// ============================================================================

/**
 * @brief Create full-duplex SPI with DMA
 *
 * Convenience function for common full-duplex SPI with DMA setup.
 *
 * @tparam TxDma TX DMA connection
 * @tparam RxDma RX DMA connection
 * @param mosi_pin MOSI pin ID
 * @param miso_pin MISO pin ID
 * @param sck_pin SCK pin ID
 * @param clock_speed Clock speed in Hz
 * @return SPI DMA configuration
 */
template <typename TxDma, typename RxDma>
constexpr auto create_spi_full_duplex_dma(
    PinId mosi_pin,
    PinId miso_pin,
    PinId sck_pin,
    u32 clock_speed) {

    return SpiDmaConfig<TxDma, RxDma>::create(
        mosi_pin,
        miso_pin,
        sck_pin,
        clock_speed,
        SpiMode::Mode0
    );
}

/**
 * @brief Create TX-only SPI with DMA
 *
 * Optimized for output-only applications (displays, DACs).
 *
 * @tparam TxDma TX DMA connection
 * @param mosi_pin MOSI pin ID
 * @param sck_pin SCK pin ID
 * @param clock_speed Clock speed in Hz
 * @return SPI DMA configuration
 */
template <typename TxDma>
constexpr auto create_spi_tx_only_dma(
    PinId mosi_pin,
    PinId sck_pin,
    u32 clock_speed) {

    return SpiDmaConfig<TxDma, void>::create(
        mosi_pin,
        PinId::PA0,  // Unused MISO
        sck_pin,
        clock_speed,
        SpiMode::Mode0
    );
}

/**
 * @brief Create high-speed SPI with DMA
 *
 * For fast devices requiring DMA for performance.
 *
 * @tparam TxDma TX DMA connection
 * @tparam RxDma RX DMA connection
 * @param mosi_pin MOSI pin ID
 * @param miso_pin MISO pin ID
 * @param sck_pin SCK pin ID
 * @param clock_speed Clock speed in Hz (typically 10+ MHz)
 * @return SPI DMA configuration
 */
template <typename TxDma, typename RxDma>
constexpr auto create_spi_high_speed_dma(
    PinId mosi_pin,
    PinId miso_pin,
    PinId sck_pin,
    u32 clock_speed) {

    return SpiDmaConfig<TxDma, RxDma>::create(
        mosi_pin,
        miso_pin,
        sck_pin,
        clock_speed,
        SpiMode::Mode0
    );
}

}  // namespace alloy::hal
