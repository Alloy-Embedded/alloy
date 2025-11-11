/**
 * @file timer_simple.hpp
 * @brief Level 1 Simple API for Timer
 * @note Part of Phase 6.5: Timer Implementation
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/interface/timer.hpp"
#include "hal/signals.hpp"

namespace alloy::hal {

using namespace alloy::hal::signals;
using namespace alloy::core;

struct TimerDefaults {
    static constexpr TimerMode mode = TimerMode::Periodic;
    static constexpr u32 period_us = 1000;  // 1ms default
    static constexpr u32 prescaler = 1;
};

template <PeripheralId PeriphId>
class Timer {
public:
    static constexpr auto quick_setup(
        TimerMode mode = TimerDefaults::mode,
        u32 period_us = TimerDefaults::period_us) {
        
        return TimerConfig{
            mode,
            period_us,
            TimerDefaults::prescaler,
            CaptureEdge::Rising,
            0  // compare_value
        };
    }

    // Preset: 1ms periodic timer
    static constexpr auto ms_1() {
        return TimerConfig{TimerMode::Periodic, 1000, 1, CaptureEdge::Rising, 0};
    }

    // Preset: 10ms periodic timer
    static constexpr auto ms_10() {
        return TimerConfig{TimerMode::Periodic, 10000, 1, CaptureEdge::Rising, 0};
    }

    // Preset: 100ms periodic timer
    static constexpr auto ms_100() {
        return TimerConfig{TimerMode::Periodic, 100000, 1, CaptureEdge::Rising, 0};
    }

    // Preset: 1s periodic timer
    static constexpr auto s_1() {
        return TimerConfig{TimerMode::Periodic, 1000000, 1, CaptureEdge::Rising, 0};
    }

    // Preset: One-shot timer
    static constexpr auto one_shot(u32 period_us) {
        return TimerConfig{TimerMode::OneShot, period_us, 1, CaptureEdge::Rising, 0};
    }

    // Preset: Free-running counter
    static constexpr auto counter() {
        return TimerConfig{TimerMode::Counter, 0, 1, CaptureEdge::Rising, 0};
    }
};

}  // namespace alloy::hal
