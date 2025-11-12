/**
 * @file timer_dma.hpp
 * @brief Timer DMA Integration
 * @note Part of Phase 6.5: Timer Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/dma/connection.hpp"
#include "hal/timer_expert.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Timer DMA configuration
 * 
 * Integrates Timer with DMA for automated data transfer on timer events.
 * Common use cases:
 * - Input capture: Store captured values in buffer via DMA
 * - Output compare: Update compare values from buffer via DMA
 * - Update events: Trigger DMA transfers on timer overflow
 */
template <typename DmaConnection = void>
struct TimerDmaConfig {
    TimerExpertConfig timer_config;

    static constexpr bool has_dma() {
        return !std::is_void_v<DmaConnection>;
    }

    constexpr bool is_valid() const {
        return timer_config.is_valid();
    }

    constexpr const char* error_message() const {
        return timer_config.error_message();
    }

    /**
     * @brief Create Timer DMA configuration
     * 
     * @param peripheral Timer peripheral ID
     * @param mode Timer mode
     * @param period_us Timer period in microseconds
     * @return TimerDmaConfig instance
     */
    static constexpr TimerDmaConfig create(
        PeripheralId peripheral,
        TimerMode mode,
        u32 period_us) {
        
        if constexpr (has_dma()) {
            static_assert(DmaConnection::is_compatible(), "Invalid DMA connection");
        }

        return TimerDmaConfig{
            TimerExpertConfig{
                .peripheral = peripheral,
                .mode = mode,
                .period_us = period_us,
                .prescaler = 1,
                .capture_edge = CaptureEdge::Rising,
                .compare_value = 0,
                .enable_interrupts = false,
                .enable_dma = has_dma(),
                .auto_reload = (mode == TimerMode::Periodic)
            }
        };
    }

    /**
     * @brief Create input capture with DMA
     * 
     * @param peripheral Timer peripheral ID
     * @param edge Capture edge selection
     * @return TimerDmaConfig for input capture
     */
    static constexpr TimerDmaConfig input_capture(
        PeripheralId peripheral,
        CaptureEdge edge) {
        
        if constexpr (has_dma()) {
            static_assert(DmaConnection::is_compatible(), "Invalid DMA connection");
        }

        return TimerDmaConfig{
            TimerExpertConfig{
                .peripheral = peripheral,
                .mode = TimerMode::InputCapture,
                .period_us = 0,
                .prescaler = 1,
                .capture_edge = edge,
                .compare_value = 0,
                .enable_interrupts = false,
                .enable_dma = has_dma(),
                .auto_reload = false
            }
        };
    }
};

/**
 * @brief Start timer with DMA transfer
 * 
 * @tparam Connection DMA connection type
 * @param buffer Buffer for DMA transfer
 * @param size Buffer size
 * @return Result with error code
 */
template <typename Connection>
inline Result<void, ErrorCode> timer_dma_start(void* buffer, usize size) {
    static_assert(Connection::is_compatible(), "Invalid DMA connection");
    // Platform implementation would go here
    return Ok();
}

/**
 * @brief Stop timer DMA transfer
 * 
 * @tparam Connection DMA connection type
 * @return Result with error code
 */
template <typename Connection>
inline Result<void, ErrorCode> timer_dma_stop() {
    static_assert(Connection::is_compatible(), "Invalid DMA connection");
    // Platform implementation would go here
    return Ok();
}

}  // namespace alloy::hal
