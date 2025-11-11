/**
 * @file systick_simple.hpp
 * @brief Level 1 Simple API for SysTick
 * @note Part of Phase 6.6: SysTick Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::hal {

using namespace alloy::core;

struct SysTickDefaults {
    static constexpr u32 frequency_hz = 1000000;  // 1MHz = 1us resolution
};

/**
 * @brief Simple SysTick API
 * 
 * Provides one-liner setup and common presets for SysTick timer.
 * SysTick is typically used for:
 * - RTOS time base (1ms ticks)
 * - Microsecond timing
 * - Delay functions
 * - Timeout handling
 */
class SysTick {
public:
    /**
     * @brief Quick setup with default 1MHz (1us resolution)
     * 
     * @return SysTickConfig with 1us resolution
     */
    static constexpr auto quick_setup() {
        return SysTickConfig{SysTickDefaults::frequency_hz};
    }

    /**
     * @brief Setup for RTOS use (1kHz = 1ms ticks)
     * 
     * Standard configuration for RTOS time base.
     * 
     * @return SysTickConfig with 1ms resolution
     */
    static constexpr auto rtos_1ms() {
        return SysTickConfig{1000};  // 1kHz = 1ms ticks
    }

    /**
     * @brief Setup for microsecond timing (1MHz = 1us)
     * 
     * High resolution timing for precise delays.
     * 
     * @return SysTickConfig with 1us resolution
     */
    static constexpr auto micros_1us() {
        return SysTickConfig{1000000};  // 1MHz = 1us ticks
    }

    /**
     * @brief Setup for millisecond timing (1kHz = 1ms)
     * 
     * @return SysTickConfig with 1ms resolution
     */
    static constexpr auto millis_1ms() {
        return SysTickConfig{1000};  // 1kHz = 1ms ticks
    }

    /**
     * @brief Custom frequency setup
     * 
     * @param frequency_hz Desired tick frequency in Hz
     * @return SysTickConfig with custom frequency
     */
    static constexpr auto custom(u32 frequency_hz) {
        return SysTickConfig{frequency_hz};
    }
};

}  // namespace alloy::hal
