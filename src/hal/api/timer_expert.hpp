/**
 * @file timer_expert.hpp
 * @brief Level 3 Expert API for Timer
 * @note Part of Phase 6.5: Timer Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/timer.hpp"

namespace alloy::hal {

using namespace alloy::core;
using namespace alloy::hal::signals;

struct TimerExpertConfig {
    PeripheralId peripheral;
    TimerMode mode;
    u32 period_us;
    u32 prescaler;
    CaptureEdge capture_edge;
    u32 compare_value;
    bool enable_interrupts;
    bool enable_dma;
    bool auto_reload;

    constexpr bool is_valid() const {
        // Basic validation
        if (mode == TimerMode::Periodic && period_us == 0) {
            return false;
        }
        if (mode == TimerMode::OneShot && period_us == 0) {
            return false;
        }
        return true;
    }

    constexpr const char* error_message() const {
        if (!is_valid()) {
            return "Invalid timer configuration";
        }
        return "Valid";
    }

    // Preset: Standard periodic timer with interrupts
    static constexpr TimerExpertConfig periodic_interrupt(
        PeripheralId periph,
        u32 period_us) {
        
        return TimerExpertConfig{
            .peripheral = periph,
            .mode = TimerMode::Periodic,
            .period_us = period_us,
            .prescaler = 1,
            .capture_edge = CaptureEdge::Rising,
            .compare_value = 0,
            .enable_interrupts = true,
            .enable_dma = false,
            .auto_reload = true
        };
    }

    // Preset: One-shot timer
    static constexpr TimerExpertConfig one_shot(
        PeripheralId periph,
        u32 period_us) {
        
        return TimerExpertConfig{
            .peripheral = periph,
            .mode = TimerMode::OneShot,
            .period_us = period_us,
            .prescaler = 1,
            .capture_edge = CaptureEdge::Rising,
            .compare_value = 0,
            .enable_interrupts = true,
            .enable_dma = false,
            .auto_reload = false
        };
    }

    // Preset: Input capture with DMA
    static constexpr TimerExpertConfig input_capture_dma(
        PeripheralId periph,
        CaptureEdge edge) {
        
        return TimerExpertConfig{
            .peripheral = periph,
            .mode = TimerMode::InputCapture,
            .period_us = 0,
            .prescaler = 1,
            .capture_edge = edge,
            .compare_value = 0,
            .enable_interrupts = false,
            .enable_dma = true,
            .auto_reload = false
        };
    }

    // Preset: Output compare
    static constexpr TimerExpertConfig output_compare(
        PeripheralId periph,
        u32 compare_value) {
        
        return TimerExpertConfig{
            .peripheral = periph,
            .mode = TimerMode::OutputCompare,
            .period_us = 0,
            .prescaler = 1,
            .capture_edge = CaptureEdge::Rising,
            .compare_value = compare_value,
            .enable_interrupts = true,
            .enable_dma = false,
            .auto_reload = false
        };
    }
};

namespace expert {

inline Result<void, ErrorCode> configure(const TimerExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }
    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
