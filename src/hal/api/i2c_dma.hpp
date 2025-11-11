/**
 * @file i2c_dma.hpp
 * @brief I2C with DMA Integration
 *
 * Provides type-safe I2C with DMA support for large transfers.
 *
 * Example Usage:
 * @code
 * using I2c0TxDma = DmaConnection<
 *     PeripheralId::I2C0,
 *     DmaRequest::I2C0_TX,
 *     DmaStream::Stream6
 * >;
 * 
 * auto config = I2cDmaConfig<I2c0TxDma, I2c0RxDma>::create(
 *     PinId::PA10, PinId::PA9, I2cSpeed::Fast
 * );
 * @endcode
 *
 * @note Part of Phase 6.3: I2C Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/dma_config.hpp"
#include "hal/dma_connection.hpp"
#include "hal/i2c_expert.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief I2C configuration with DMA support
 */
template <typename TxDmaConnection = void, typename RxDmaConnection = void>
struct I2cDmaConfig {
    I2cExpertConfig i2c_config;

    using TxDma = TxDmaConnection;
    using RxDma = RxDmaConnection;

    static constexpr bool has_tx_dma() {
        return !std::is_void_v<TxDmaConnection>;
    }

    static constexpr bool has_rx_dma() {
        return !std::is_void_v<RxDmaConnection>;
    }

    static constexpr I2cDmaConfig create(
        PinId sda_pin,
        PinId scl_pin,
        I2cSpeed speed) {

        if constexpr (has_tx_dma()) {
            static_assert(TxDmaConnection::is_compatible(),
                         "Invalid TX DMA connection");
        }

        if constexpr (has_rx_dma()) {
            static_assert(RxDmaConnection::is_compatible(),
                         "Invalid RX DMA connection");
        }

        constexpr PeripheralId peripheral = []() {
            if constexpr (has_tx_dma()) {
                return TxDmaConnection::peripheral;
            } else if constexpr (has_rx_dma()) {
                return RxDmaConnection::peripheral;
            } else {
                return PeripheralId::I2C0;
            }
        }();

        return I2cDmaConfig{
            .i2c_config = {
                .peripheral = peripheral,
                .sda_pin = sda_pin,
                .scl_pin = scl_pin,
                .speed = speed,
                .addressing = I2cAddressing::SevenBit,
                .enable_interrupts = has_tx_dma() || has_rx_dma(),
                .enable_dma_tx = has_tx_dma(),
                .enable_dma_rx = has_rx_dma(),
                .enable_analog_filter = true,
                .enable_digital_filter = false,
                .digital_filter_coefficient = 0
            }
        };
    }

    constexpr bool is_valid() const {
        return i2c_config.is_valid();
    }

    constexpr const char* error_message() const {
        return i2c_config.error_message();
    }
};

/**
 * @brief Write data via I2C using DMA
 */
template <typename TxConnection>
inline Result<void, ErrorCode> i2c_dma_write(
    u16 address,
    const void* data,
    usize size) {

    static_assert(TxConnection::is_compatible(), "Invalid DMA connection");

    auto dma_config = DmaTransferConfig<TxConnection>::memory_to_peripheral(
        data, size, DmaDataWidth::Bits8);

    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = std::move(validation).error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    (void)address;

    return Ok();
}

/**
 * @brief Read data via I2C using DMA
 */
template <typename RxConnection>
inline Result<void, ErrorCode> i2c_dma_read(
    u16 address,
    void* buffer,
    usize size) {

    static_assert(RxConnection::is_compatible(), "Invalid DMA connection");

    auto dma_config = DmaTransferConfig<RxConnection>::peripheral_to_memory(
        buffer, size, DmaDataWidth::Bits8);

    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = std::move(validation).error();
        return Err(std::move(error_copy));
    }

    // TODO: Start DMA transfer
    (void)address;

    return Ok();
}

/**
 * @brief Preset: Create fast I2C with DMA
 */
template <typename TxDma, typename RxDma>
constexpr auto create_i2c_fast_dma(
    PinId sda_pin,
    PinId scl_pin) {

    return I2cDmaConfig<TxDma, RxDma>::create(
        sda_pin,
        scl_pin,
        I2cSpeed::Fast
    );
}

}  // namespace alloy::hal
