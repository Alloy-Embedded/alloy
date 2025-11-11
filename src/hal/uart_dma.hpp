/**
 * @file uart_dma.hpp
 * @brief UART with DMA Integration
 *
 * Provides type-safe UART with DMA support, building on the
 * multi-level API infrastructure.
 *
 * Design Principles:
 * - Type-safe DMA channel allocation
 * - Compile-time conflict detection
 * - Automatic peripheral address setup
 * - Integration with UART Expert API
 * - Zero runtime overhead for configuration
 *
 * Example Usage:
 * @code
 * // Define DMA connections for UART0
 * using Uart0TxDma = DmaConnection<
 *     PeripheralId::USART0,
 *     DmaRequest::USART0_TX,
 *     DmaStream::Stream0
 * >;
 * using Uart0RxDma = DmaConnection<
 *     PeripheralId::USART0,
 *     DmaRequest::USART0_RX,
 *     DmaStream::Stream1
 * >;
 *
 * // Create UART with DMA configuration
 * constexpr auto config = UartDmaConfig<Uart0TxDma, Uart0RxDma>::create(
 *     PinId::PD3,  // TX pin
 *     PinId::PD4,  // RX pin
 *     BaudRate{115200}
 * );
 *
 * // Validate at compile-time
 * static_assert(config.is_valid(), config.error_message());
 *
 * // Transmit with DMA
 * auto tx_result = uart_dma_transmit<Uart0TxDma>(buffer, size);
 * @endcode
 *
 * @note Part of Phase 5.3 & 6.1: DMA Integration with UART
 * @see openspec/changes/modernize-peripheral-architecture/specs/dma-integration/spec.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/dma_config.hpp"
#include "hal/dma_connection.hpp"
#include "hal/dma_registry.hpp"
#include "hal/uart_expert.hpp"

namespace alloy::hal {

using namespace alloy::core;

// ============================================================================
// UART DMA Configuration
// ============================================================================

/**
 * @brief UART configuration with DMA support
 *
 * Extends UartExpertConfig with type-safe DMA channel allocation.
 *
 * @tparam TxDmaConnection DMA connection for TX (can be void for TX-only UART)
 * @tparam RxDmaConnection DMA connection for RX (can be void for RX-only UART)
 */
template <typename TxDmaConnection = void, typename RxDmaConnection = void>
struct UartDmaConfig {
    UartExpertConfig uart_config;

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
     * @brief Create UART DMA configuration
     *
     * Factory method for creating a validated configuration.
     *
     * @param tx_pin TX pin ID
     * @param rx_pin RX pin ID
     * @param baudrate Baud rate
     * @param parity Parity setting
     * @return UART DMA configuration
     */
    static constexpr UartDmaConfig create(
        PinId tx_pin,
        PinId rx_pin,
        BaudRate baudrate,
        UartParity parity = UartParity::NONE) {

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
                return PeripheralId::USART0; // Default
            }
        }();

        return UartDmaConfig{
            .uart_config = {
                .peripheral = peripheral,
                .tx_pin = tx_pin,
                .rx_pin = rx_pin,
                .baudrate = baudrate,
                .data_bits = 8,
                .parity = parity,
                .stop_bits = 1,
                .flow_control = false,
                .enable_tx = has_tx_dma() || tx_pin != PinId::PA0,
                .enable_rx = has_rx_dma() || rx_pin != PinId::PA0,
                .enable_interrupts = has_tx_dma() || has_rx_dma(), // DMA needs interrupts
                .enable_dma_tx = has_tx_dma(),
                .enable_dma_rx = has_rx_dma(),
                .enable_oversampling = true,
                .enable_rx_timeout = false,
                .rx_timeout_value = 0
            }
        };
    }

    /**
     * @brief Validate configuration
     *
     * Checks both UART and DMA configurations.
     *
     * @return true if valid, false otherwise
     */
    constexpr bool is_valid() const {
        // Check UART configuration
        if (!uart_config.is_valid()) {
            return false;
        }

        // Check that DMA is enabled if connections are provided
        if constexpr (has_tx_dma()) {
            if (!uart_config.enable_dma_tx) {
                return false;
            }
        }

        if constexpr (has_rx_dma()) {
            if (!uart_config.enable_dma_rx) {
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
        if (!uart_config.is_valid()) {
            return uart_config.error_message();
        }

        if constexpr (has_tx_dma()) {
            if (!uart_config.enable_dma_tx) {
                return "TX DMA connection provided but DMA TX not enabled";
            }
        }

        if constexpr (has_rx_dma()) {
            if (!uart_config.enable_dma_rx) {
                return "RX DMA connection provided but DMA RX not enabled";
            }
        }

        return "Valid UART DMA configuration";
    }
};

// ============================================================================
// UART DMA Operations
// ============================================================================

/**
 * @brief Transmit data via UART using DMA
 *
 * Type-safe DMA transmission with automatic channel setup.
 *
 * @tparam Connection The TX DMA connection
 * @param data Buffer containing data to transmit
 * @param size Number of bytes to transmit
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> uart_dma_transmit(const void* data, usize size) {
    // Validate connection at compile-time
    static_assert(Connection::is_compatible(), "Invalid DMA connection");

    // Create DMA configuration
    auto dma_config = create_uart_tx_dma<Connection>(data, size);

    // Validate
    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = validation.error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    // - Configure DMA stream
    // - Set source/destination addresses
    // - Set transfer size
    // - Enable DMA channel
    // - Enable UART TX DMA request

    return Ok();
}

/**
 * @brief Receive data via UART using DMA
 *
 * Type-safe DMA reception with automatic channel setup.
 *
 * @tparam Connection The RX DMA connection
 * @param buffer Buffer to store received data
 * @param size Maximum number of bytes to receive
 * @param circular Use circular mode for continuous reception
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> uart_dma_receive(void* buffer, usize size, bool circular = false) {
    // Validate connection at compile-time
    static_assert(Connection::is_compatible(), "Invalid DMA connection");

    // Create DMA configuration
    auto dma_config = create_uart_rx_dma<Connection>(buffer, size, circular);

    // Validate
    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = validation.error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    // - Configure DMA stream
    // - Set source/destination addresses
    // - Set transfer size
    // - Enable DMA channel
    // - Enable UART RX DMA request

    return Ok();
}

/**
 * @brief Stop UART TX DMA transfer
 *
 * @tparam Connection The TX DMA connection
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> uart_dma_stop_tx() {
    // TODO: Stop DMA transfer
    // - Disable DMA channel
    // - Disable UART TX DMA request
    // - Clear any pending interrupts

    return Ok();
}

/**
 * @brief Stop UART RX DMA transfer
 *
 * @tparam Connection The RX DMA connection
 * @return Result indicating success or error
 */
template <typename Connection>
inline Result<void, ErrorCode> uart_dma_stop_rx() {
    // TODO: Stop DMA transfer
    // - Disable DMA channel
    // - Disable UART RX DMA request
    // - Clear any pending interrupts

    return Ok();
}

// ============================================================================
// Preset Configurations
// ============================================================================

/**
 * @brief Create full-duplex UART with DMA on both TX and RX
 *
 * Convenience function for common full-duplex UART with DMA setup.
 *
 * @tparam TxDma TX DMA connection
 * @tparam RxDma RX DMA connection
 * @param tx_pin TX pin ID
 * @param rx_pin RX pin ID
 * @param baudrate Baud rate
 * @return UART DMA configuration
 */
template <typename TxDma, typename RxDma>
constexpr auto create_uart_full_duplex_dma(
    PinId tx_pin,
    PinId rx_pin,
    BaudRate baudrate) {

    return UartDmaConfig<TxDma, RxDma>::create(
        tx_pin,
        rx_pin,
        baudrate,
        UartParity::NONE
    );
}

/**
 * @brief Create TX-only UART with DMA
 *
 * Optimized for logging or output-only applications.
 *
 * @tparam TxDma TX DMA connection
 * @param tx_pin TX pin ID
 * @param baudrate Baud rate
 * @return UART DMA configuration
 */
template <typename TxDma>
constexpr auto create_uart_tx_only_dma(PinId tx_pin, BaudRate baudrate) {
    return UartDmaConfig<TxDma, void>::create(
        tx_pin,
        PinId::PA0, // Unused RX pin
        baudrate,
        UartParity::NONE
    );
}

/**
 * @brief Create RX-only UART with DMA
 *
 * Optimized for input-only applications.
 *
 * @tparam RxDma RX DMA connection
 * @param rx_pin RX pin ID
 * @param baudrate Baud rate
 * @return UART DMA configuration
 */
template <typename RxDma>
constexpr auto create_uart_rx_only_dma(PinId rx_pin, BaudRate baudrate) {
    return UartDmaConfig<void, RxDma>::create(
        PinId::PA0, // Unused TX pin
        rx_pin,
        baudrate,
        UartParity::NONE
    );
}

}  // namespace alloy::hal
