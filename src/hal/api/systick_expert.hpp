/**
 * @file systick_expert.hpp
 * @brief Level 3 Expert API for SysTick
 * @note Part of Phase 6.6: SysTick Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert SysTick configuration
 * 
 * Full control over SysTick configuration including:
 * - Frequency
 * - Interrupt priority
 * - Callback registration
 * - Clock source selection
 */
struct SysTickExpertConfig {
    u32 frequency_hz;
    u8 interrupt_priority;  // 0-255, lower = higher priority
    bool enable_interrupt;
    bool use_processor_clock;  // true = processor clock, false = external clock

    constexpr bool is_valid() const {
        // Frequency must be non-zero
        if (frequency_hz == 0) {
            return false;
        }
        // Frequency should be reasonable (between 1Hz and 100MHz)
        if (frequency_hz > 100000000) {
            return false;
        }
        return true;
    }

    constexpr const char* error_message() const {
        if (frequency_hz == 0) {
            return "Frequency cannot be zero";
        }
        if (frequency_hz > 100000000) {
            return "Frequency too high (max 100MHz)";
        }
        return "Valid";
    }

    /**
     * @brief Standard microsecond timer (1MHz, interrupts enabled)
     * 
     * @param priority Interrupt priority (0-255)
     * @return SysTickExpertConfig for 1us timing
     */
    static constexpr SysTickExpertConfig micros_timer(u8 priority = 15) {
        return SysTickExpertConfig{
            .frequency_hz = 1000000,
            .interrupt_priority = priority,
            .enable_interrupt = true,
            .use_processor_clock = true
        };
    }

    /**
     * @brief RTOS time base (1kHz, interrupts enabled)
     * 
     * @param priority Interrupt priority (0-255)
     * @return SysTickExpertConfig for RTOS
     */
    static constexpr SysTickExpertConfig rtos_timebase(u8 priority = 15) {
        return SysTickExpertConfig{
            .frequency_hz = 1000,
            .interrupt_priority = priority,
            .enable_interrupt = true,
            .use_processor_clock = true
        };
    }

    /**
     * @brief Polling mode (no interrupts)
     * 
     * @param frequency_hz Tick frequency
     * @return SysTickExpertConfig for polling
     */
    static constexpr SysTickExpertConfig polling_mode(u32 frequency_hz) {
        return SysTickExpertConfig{
            .frequency_hz = frequency_hz,
            .interrupt_priority = 0,
            .enable_interrupt = false,
            .use_processor_clock = true
        };
    }

    /**
     * @brief Custom configuration
     * 
     * @param freq_hz Frequency in Hz
     * @param priority Interrupt priority
     * @param interrupts Enable interrupts
     * @param proc_clk Use processor clock
     * @return SysTickExpertConfig with custom settings
     */
    static constexpr SysTickExpertConfig custom(
        u32 freq_hz,
        u8 priority = 15,
        bool interrupts = true,
        bool proc_clk = true) {
        
        return SysTickExpertConfig{
            .frequency_hz = freq_hz,
            .interrupt_priority = priority,
            .enable_interrupt = interrupts,
            .use_processor_clock = proc_clk
        };
    }
};

namespace expert {

/**
 * @brief Configure SysTick with expert settings
 * 
 * @param config Expert configuration
 * @return Result with error code
 */
inline Result<void, ErrorCode> configure(const SysTickExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }
    // Platform implementation would configure hardware here
    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
