/// Alloy RTOS - Tickless Idle Implementation

#include "rtos/tickless_idle.hpp"
#include "rtos/scheduler.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::rtos {

// ============================================================================
// Static Members
// ============================================================================

TicklessConfig TicklessIdle::config_{};
static PowerStats power_stats_{};

// ============================================================================
// Weak Default Implementations
// ============================================================================

/// Default implementation: Enter sleep (WFI)
extern "C" void tickless_enter_sleep(SleepMode mode, core::u32 duration_us) {
    (void)mode;
    (void)duration_us;

    // Default: Just wait for interrupt (lightest sleep)
    __asm__ volatile("wfi");
}

/// Default implementation: Exit sleep (no-op)
extern "C" void tickless_exit_sleep(core::u32 actual_sleep_us) {
    (void)actual_sleep_us;
    // Default: nothing to do
}

/// Default implementation: Get next wake-up time
extern "C" core::u32 tickless_get_next_wakeup_us() {
    // Check all delayed tasks and find nearest wake time
    core::u32 current_time = hal::SysTick::micros();
    core::u32 min_wakeup = 0xFFFFFFFF;

    // This would iterate through all delayed tasks
    // For now, return a conservative estimate
    // Real implementation needs access to scheduler internals

    return min_wakeup;
}

// ============================================================================
// Tickless Idle API Implementation
// ============================================================================

core::Result<void, RTOSError> TicklessIdle::enable() noexcept {
    config_.enabled = true;
    return core::Ok();
}

core::Result<void, RTOSError> TicklessIdle::disable() noexcept {
    config_.enabled = false;
    return core::Ok();
}

bool TicklessIdle::is_enabled() noexcept {
    return config_.enabled;
}

core::Result<void, RTOSError> TicklessIdle::configure(
    SleepMode mode,
    core::u32 min_sleep_us
) noexcept {
    config_.mode = mode;
    config_.min_sleep_duration_us = min_sleep_us;
    return core::Ok();
}

core::Result<void, RTOSError> TicklessIdle::set_max_sleep_duration(
    core::u32 max_sleep_us
) noexcept {
    config_.max_sleep_duration_us = max_sleep_us;
    return core::Ok();
}

core::Result<void, RTOSError> TicklessIdle::set_wakeup_latency(
    core::u32 latency_us
) noexcept {
    config_.wakeup_latency_us = latency_us;
    return core::Ok();
}

const TicklessConfig& TicklessIdle::get_config() noexcept {
    return config_;
}

bool TicklessIdle::should_sleep() noexcept {
    if (!config_.enabled) {
        return false;
    }

    // Get next wake-up time
    core::u32 next_wakeup_us = tickless_get_next_wakeup_us();

    // Check if sleep duration is worth it
    if (next_wakeup_us < config_.min_sleep_duration_us + config_.wakeup_latency_us) {
        return false;  // Too short to bother sleeping
    }

    return true;
}

core::u32 TicklessIdle::enter_sleep() noexcept {
    // Get current time
    core::u32 sleep_start = hal::SysTick::micros();

    // Calculate sleep duration
    core::u32 next_wakeup_us = tickless_get_next_wakeup_us();
    core::u32 sleep_duration = next_wakeup_us - config_.wakeup_latency_us;

    // Clamp to max sleep duration
    if (sleep_duration > config_.max_sleep_duration_us) {
        sleep_duration = config_.max_sleep_duration_us;
    }

    // Enter sleep mode
    tickless_enter_sleep(config_.mode, sleep_duration);

    // Calculate actual sleep time
    core::u32 sleep_end = hal::SysTick::micros();
    core::u32 actual_sleep = sleep_end - sleep_start;

    // Exit sleep (user hook)
    tickless_exit_sleep(actual_sleep);

    // Update statistics
    power_stats_.total_sleep_time_us += actual_sleep;
    power_stats_.sleep_count++;
    power_stats_.wakeup_count++;

    if (actual_sleep < power_stats_.min_sleep_duration_us || power_stats_.min_sleep_duration_us == 0) {
        power_stats_.min_sleep_duration_us = actual_sleep;
    }

    if (actual_sleep > power_stats_.max_sleep_duration_us) {
        power_stats_.max_sleep_duration_us = actual_sleep;
    }

    // Update average
    power_stats_.avg_sleep_duration_us =
        power_stats_.total_sleep_time_us / power_stats_.sleep_count;

    return actual_sleep;
}

PowerStats& get_power_stats() noexcept {
    return power_stats_;
}

}  // namespace alloy::rtos
