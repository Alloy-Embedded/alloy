/**
 * @file adc_dma.hpp
 * @brief ADC with DMA and Timer Integration
 * @note Part of Phase 6.4: ADC Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/dma_config.hpp"
#include "hal/dma_connection.hpp"
#include "hal/adc_expert.hpp"

namespace alloy::hal {

using namespace alloy::core;

template <typename DmaConnection = void, typename TimerTrigger = void>
struct AdcDmaConfig {
    AdcExpertConfig adc_config;

    static constexpr bool has_dma() {
        return !std::is_void_v<DmaConnection>;
    }

    static constexpr bool has_timer_trigger() {
        return !std::is_void_v<TimerTrigger>;
    }

    static constexpr AdcDmaConfig create(
        PeripheralId peripheral,
        AdcChannel channel) {

        if constexpr (has_dma()) {
            static_assert(DmaConnection::is_compatible(), "Invalid DMA");
        }

        return AdcDmaConfig{
            .adc_config = {
                .peripheral = peripheral,
                .channel = channel,
                .resolution = AdcResolution::Bits12,
                .reference = AdcReference::Vdd,
                .sample_time = AdcSampleTime::Cycles84,
                .enable_dma = has_dma(),
                .enable_continuous = has_dma(),
                .enable_timer_trigger = has_timer_trigger()
            }
        };
    }

    constexpr bool is_valid() const {
        return adc_config.is_valid();
    }
};

template <typename DmaConn>
inline Result<void, ErrorCode> adc_dma_start(
    AdcChannel channel,
    void* buffer,
    usize size) {

    static_assert(DmaConn::is_compatible(), "Invalid DMA");

    auto dma_config = DmaTransferConfig<DmaConn>::peripheral_to_memory(
        buffer, size, DmaDataWidth::Bits16);

    auto validation = dma_config.validate();
    if (!validation.is_ok()) {
        ErrorCode error_copy = std::move(validation).error();
        return Err(std::move(error_copy));
    }

    return Ok();
}

}  // namespace alloy::hal
