#pragma once

/**
 * @file board_config.hpp
 * @brief SAME70 Xplained Ultra Board Configuration
 *
 * Modern C++23 board abstraction with compile-time configuration.
 * Uses generated peripheral addresses and type-safe GPIO pins.
 *
 * Features:
 * - Compile-time pin/peripheral configuration
 * - Zero runtime overhead
 * - Type-safe board resources using actual HAL types
 * - No magic numbers - all addresses from generated peripherals.hpp
 */

#include <cstdint>

#include "device/system_clock.hpp"
#include "hal/connect/tags.hpp"

namespace board::same70_xplained {

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_safe_internal_12mhz;
    using SystemClockProfile =
        alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr uint32_t cpu_freq_hz = SystemClockProfile::kSysclkHz;
    static constexpr uint32_t hclk_freq_hz = SystemClockProfile::kHclkHz;
    static constexpr uint32_t pclk_freq_hz = SystemClockProfile::kPclkHz;
    static constexpr uint32_t mck_prescaler = SystemClockProfile::kMckPrescaler;
    static constexpr uint32_t flash_latency = SystemClockProfile::kFlashLatency;
};

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    using led_green = alloy::hal::pin<"PC8">;

    static constexpr bool led_green_active_high = false;  // Active LOW

    // LED1 could be added here for future expansion
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    using button0 = alloy::hal::pin<"PA11">;

    static constexpr bool button0_active_high = false;  // Active LOW
};

// =============================================================================
// SysTick Configuration
// =============================================================================

struct SysTickConfig {
    static constexpr uint32_t tick_freq_hz = 1'000;  // 1 kHz (1ms ticks)
};

// =============================================================================
// Board Info
// =============================================================================

struct BoardInfo {
    static constexpr const char* name = "SAME70 Xplained Ultra";
    static constexpr const char* mcu = "ATSAME70Q21B";
    static constexpr const char* vendor = "Microchip/Atmel";
    static constexpr const char* architecture = "ARM Cortex-M7";
};

}  // namespace board::same70_xplained
