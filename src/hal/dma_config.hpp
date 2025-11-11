/**
 * @file dma_config.hpp
 * @brief Type-Safe DMA Configuration
 *
 * Provides type-safe DMA configuration with compile-time validation
 * and automatic register address setup.
 *
 * Design Principles:
 * - Type safety for source/destination addresses
 * - Automatic peripheral address setup
 * - Compile-time validation of transfer parameters
 * - Clear error messages
 * - Integration with DMA registry
 *
 * Example Usage:
 * @code
 * // Configure UART TX with DMA
 * using Uart0TxDma = DmaConnection<
 *     PeripheralId::USART0,
 *     DmaRequest::USART0_TX,
 *     DmaStream::Stream0
 * >;
 *
 * // Create type-safe configuration
 * auto config = DmaTransferConfig<Uart0TxDma>::memory_to_peripheral(
 *     buffer_address,
 *     buffer_size,
 *     DmaDataWidth::Bits8
 * );
 *
 * // Peripheral address set automatically!
 * static_assert(config.direction == DmaDirection::MemoryToPeripheral);
 * @endcode
 *
 * @note Part of Phase 5.3: Type-Safe DMA Configuration
 * @see openspec/changes/modernize-peripheral-architecture/specs/dma-integration/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/dma_connection.hpp"
#include "hal/interface/dma.hpp"

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// Peripheral Address Helpers
// ============================================================================

/**
 * @brief Get peripheral data register address for DMA
 *
 * Returns the memory-mapped address of the peripheral's data register
 * for DMA transfers.
 *
 * @param peripheral The peripheral ID
 * @param request The DMA request type (TX or RX)
 * @return Peripheral register address
 *
 * @note Platform-specific implementations will provide actual addresses
 */
constexpr volatile void* get_peripheral_dma_address(PeripheralId peripheral, DmaRequest request) {
    // TODO: Platform-specific address mapping
    // For now, return nullptr as placeholder
    // Real implementation will use register maps from vendor headers

    // Example for SAME70:
    // if (peripheral == PeripheralId::USART0) {
    //     return &USART0->US_THR; // TX
    //     return &USART0->US_RHR; // RX
    // }

    return nullptr;
}

// ============================================================================
// Type-Safe DMA Transfer Configuration
// ============================================================================

/**
 * @brief Type-safe DMA transfer configuration
 *
 * Wraps DmaConfig with compile-time validation and automatic
 * peripheral address setup based on the DMA connection type.
 *
 * @tparam Connection The DMA connection type
 */
template <typename Connection>
struct DmaTransferConfig {
    DmaConfig config;
    const void* source_address;
    void* destination_address;
    usize transfer_size;

    /**
     * @brief Create memory-to-peripheral transfer
     *
     * Automatically sets peripheral address based on connection type.
     *
     * @param source Source buffer address
     * @param size Number of elements to transfer
     * @param width Data width per element
     * @param mode Transfer mode (Normal or Circular)
     * @param priority Channel priority
     * @return Transfer configuration
     */
    static constexpr DmaTransferConfig memory_to_peripheral(
        const void* source,
        usize size,
        DmaDataWidth width = DmaDataWidth::Bits8,
        DmaMode mode = DmaMode::Normal,
        DmaPriority priority = DmaPriority::Medium) {

        // Validate connection
        static_assert(Connection::is_compatible(),
                     "Invalid DMA connection");

        DmaConfig dma_conf(
            DmaDirection::MemoryToPeripheral,
            mode,
            priority,
            width,
            get_peripheral_dma_address(Connection::peripheral, Connection::request)
        );

        return DmaTransferConfig{
            dma_conf,
            source,
            nullptr, // Peripheral address in config
            size
        };
    }

    /**
     * @brief Create peripheral-to-memory transfer
     *
     * Automatically sets peripheral address based on connection type.
     *
     * @param destination Destination buffer address
     * @param size Number of elements to transfer
     * @param width Data width per element
     * @param mode Transfer mode (Normal or Circular)
     * @param priority Channel priority
     * @return Transfer configuration
     */
    static constexpr DmaTransferConfig peripheral_to_memory(
        void* destination,
        usize size,
        DmaDataWidth width = DmaDataWidth::Bits8,
        DmaMode mode = DmaMode::Normal,
        DmaPriority priority = DmaPriority::Medium) {

        // Validate connection
        static_assert(Connection::is_compatible(),
                     "Invalid DMA connection");

        DmaConfig dma_conf(
            DmaDirection::PeripheralToMemory,
            mode,
            priority,
            width,
            get_peripheral_dma_address(Connection::peripheral, Connection::request)
        );

        return DmaTransferConfig{
            dma_conf,
            nullptr, // Peripheral address in config
            destination,
            size
        };
    }

    /**
     * @brief Create memory-to-memory transfer
     *
     * For fast memory copy operations.
     *
     * @param source Source buffer address
     * @param destination Destination buffer address
     * @param size Number of elements to transfer
     * @param width Data width per element
     * @param priority Channel priority
     * @return Transfer configuration
     */
    static constexpr DmaTransferConfig memory_to_memory(
        const void* source,
        void* destination,
        usize size,
        DmaDataWidth width = DmaDataWidth::Bits32,
        DmaPriority priority = DmaPriority::Low) {

        DmaConfig dma_conf(
            DmaDirection::MemoryToMemory,
            DmaMode::Normal, // Always normal for mem-to-mem
            priority,
            width,
            nullptr
        );

        return DmaTransferConfig{
            dma_conf,
            source,
            destination,
            size
        };
    }

    /**
     * @brief Validate configuration
     *
     * Checks alignment and parameter validity.
     *
     * @return Result indicating success or error
     */
    Result<void, ErrorCode> validate() const {
        // Check alignment for source
        if (source_address != nullptr) {
            if (!is_dma_aligned(source_address, config.data_width)) {
                return Err(ErrorCode::DmaAlignmentError);
            }
        }

        // Check alignment for destination
        if (destination_address != nullptr) {
            if (!is_dma_aligned(destination_address, config.data_width)) {
                return Err(ErrorCode::DmaAlignmentError);
            }
        }

        // Check transfer size
        if (transfer_size == 0) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Memory-to-memory needs both addresses
        if (config.direction == DmaDirection::MemoryToMemory) {
            if (source_address == nullptr || destination_address == nullptr) {
                return Err(ErrorCode::InvalidParameter);
            }
        }

        return Ok();
    }

    /**
     * @brief Check if configuration is valid
     *
     * @return true if valid, false otherwise
     */
    bool is_valid() const {
        return validate().is_ok();
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Create UART TX DMA configuration
 *
 * Convenience function for common UART TX DMA setup.
 *
 * @tparam Connection The DMA connection for UART TX
 * @param buffer Source buffer with data to transmit
 * @param size Number of bytes to transmit
 * @return DMA transfer configuration
 */
template <typename Connection>
constexpr auto create_uart_tx_dma(const void* buffer, usize size) {
    return DmaTransferConfig<Connection>::memory_to_peripheral(
        buffer,
        size,
        DmaDataWidth::Bits8,
        DmaMode::Normal,
        DmaPriority::Medium
    );
}

/**
 * @brief Create UART RX DMA configuration
 *
 * Convenience function for common UART RX DMA setup.
 *
 * @tparam Connection The DMA connection for UART RX
 * @param buffer Destination buffer for received data
 * @param size Maximum number of bytes to receive
 * @param circular Use circular mode for continuous reception
 * @return DMA transfer configuration
 */
template <typename Connection>
constexpr auto create_uart_rx_dma(void* buffer, usize size, bool circular = false) {
    return DmaTransferConfig<Connection>::peripheral_to_memory(
        buffer,
        size,
        DmaDataWidth::Bits8,
        circular ? DmaMode::Circular : DmaMode::Normal,
        DmaPriority::Medium
    );
}

/**
 * @brief Create ADC DMA configuration
 *
 * Convenience function for ADC sampling with DMA.
 *
 * @tparam Connection The DMA connection for ADC
 * @param buffer Destination buffer for ADC samples
 * @param sample_count Number of samples to collect
 * @param circular Use circular mode for continuous sampling
 * @return DMA transfer configuration
 */
template <typename Connection>
constexpr auto create_adc_dma(void* buffer, usize sample_count, bool circular = false) {
    return DmaTransferConfig<Connection>::peripheral_to_memory(
        buffer,
        sample_count,
        DmaDataWidth::Bits16, // Most ADCs use 16-bit samples
        circular ? DmaMode::Circular : DmaMode::Normal,
        DmaPriority::High // ADC often needs high priority
    );
}

}  // namespace alloy::hal
