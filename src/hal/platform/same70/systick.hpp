/// ATSAME70Q21B SysTick Timer Implementation
///
/// Uses ARM Cortex-M7 SysTick timer for microsecond time tracking.
/// Integrates with Clock HAL to configure timing based on actual MCK frequency.
///
/// Features:
/// - Microsecond precision time tracking
/// - Automatic configuration based on MCK frequency
/// - Interrupt-driven counter for accurate timing (1kHz)
/// - 32-bit microsecond counter (overflows after ~71 minutes)
/// - Sub-millisecond interpolation from hardware counter
/// - Used by RTOS scheduler for task switching
///
/// Configuration:
/// - Uses processor clock (MCK) as source
/// - Configures for 1kHz tick rate (1ms interrupt)
/// - Provides microsecond resolution through interpolation
/// - Enables SysTick interrupt for counter updates
///
/// Memory Usage:
/// - 12 bytes RAM (counter + reload value storage)
/// - ~600 bytes code
///
/// Example:
/// @code
/// #include "hal/platform/same70/systick.hpp"
/// #include "hal/platform/same70/clock.hpp"
///
/// // Initialize clock first
/// Clock::initialize(config);
///
/// // Initialize SysTick
/// SystemTick::init();
///
/// // Get current time
/// uint32_t now = SystemTick::micros();
///
/// // Measure elapsed time
/// uint32_t start = SystemTick::micros();
/// do_work();
/// uint32_t elapsed = SystemTick::micros() - start;
/// @endcode

#ifndef ALLOY_HAL_SAME70_SYSTICK_HPP
#define ALLOY_HAL_SAME70_SYSTICK_HPP

#include "hal/interface/systick.hpp"
#include "hal/platform/same70/clock.hpp"
#include "hal/vendors/atmel/same70/bitfields/systick_bitfields.hpp"
#include "hal/vendors/atmel/same70/registers/systick_registers.hpp"

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal::same70 {

/// ATSAME70Q21B SysTick implementation using ARM Cortex-M hardware timer
///
/// This class provides microsecond-precision time tracking using the
/// SysTick timer. The timer is configured based on the current MCK frequency
/// obtained from the Clock HAL.
///
/// Thread safety:
/// - micros() is interrupt-safe (atomic read)
/// - ISR updates counter atomically
///
/// @note Call Clock::initialize() before SystemTick::init()
class SystemTick {
   public:
    /// Initialize SysTick timer
    ///
    /// Configures SysTick for 1kHz tick rate based on current MCK.
    /// Enables SysTick interrupt for counter updates.
    ///
    /// Requirements:
    /// - Clock::initialize() must be called first
    /// - MCK must be stable
    ///
    /// @return Ok on success, HardwareError if clock not initialized
    static core::Result<void, core::ErrorCode> init() {
        using namespace alloy::hal::atmel::same70::systick;
        namespace csr_bits = alloy::hal::atmel::same70::systick::csr;
        namespace rvr_bits = alloy::hal::atmel::same70::systick::rvr;

        // Get current MCK frequency from Clock HAL
        auto mck_hz = Clock::getMasterClockFrequency();
        if (mck_hz == 0) {
            return core::Err(core::ErrorCode::HardwareError);
        }

        // Calculate reload value for 1ms tick (1000 Hz)
        // SysTick counts down from reload value to 0, then reloads
        // For 1ms tick: reload = (MCK_Hz / 1kHz) - 1
        // 1kHz is much more reasonable than 1MHz for interrupt rate
        uint32_t reload_value = (mck_hz / 1000) - 1;

        // Maximum reload value is 24-bit (0xFFFFFF)
        if (reload_value > 0xFFFFFF) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        auto* systick = SysTick();

        // 1. Disable SysTick during configuration
        systick->CSR = 0;

        // 2. Clear current value
        systick->CVR = 0;

        // 3. Set reload value for 1ms tick
        systick->RVR = rvr_bits::RELOAD::write(0, reload_value);

        // Store reload value for micros() calculation
        reload_value_ = reload_value;

        // Reset counters BEFORE enabling interrupts
        millis_counter_ = 0;
        initialized_ = true;

        // 4. Configure and enable SysTick
        // CSR bits: ENABLE(0) | TICKINT(1) | CLKSOURCE(2)
        // Following LUG pattern: TICKINT | CLKSOURCE | ENABLE
        // Bit 0: ENABLE    = 0x1
        // Bit 1: TICKINT   = 0x2
        // Bit 2: CLKSOURCE = 0x4
        // Total: 0x7 = 0b111
        systick->CSR = 0x7;

        return core::Ok();
    }

    /// Get current time in microseconds
    ///
    /// Returns microseconds elapsed since init().
    /// Combines millisecond counter (from ISR) with hardware counter for sub-ms precision.
    ///
    /// Thread-safe: Yes (atomic reads on ARM Cortex-M)
    ///
    /// @return Microseconds since init
    static core::u32 micros() {
        if (!initialized_) {
            return 0;
        }

        using namespace alloy::hal::atmel::same70::systick;
        namespace cvr_bits = alloy::hal::atmel::same70::systick::cvr;
        auto* systick = SysTick();

        // Read millisecond counter (updated by ISR every 1ms)
        uint32_t millis = millis_counter_;

        // Read current hardware counter value
        // SysTick counts DOWN from reload_value to 0
        uint32_t current = cvr_bits::CURRENT::read(systick->CVR);

        // Calculate microseconds within current millisecond
        // elapsed_ticks = reload_value - current
        // Each tick = (1ms / reload_value) = (1000us / reload_value)
        uint32_t elapsed_ticks = reload_value_ - current;
        uint32_t sub_millis_us = (elapsed_ticks * 1000) / reload_value_;

        // Total microseconds = (milliseconds * 1000) + sub-millisecond part
        return (millis * 1000) + sub_millis_us;
    }

    /// Reset counter to zero
    ///
    /// Resets the millisecond counter. Useful for testing or
    /// when overflow needs to be managed explicitly.
    ///
    /// @return Ok on success
    static core::Result<void, core::ErrorCode> reset() {
        millis_counter_ = 0;
        return core::Ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called successfully
    static bool is_initialized() { return initialized_; }

    /// SysTick interrupt handler
    ///
    /// Called every 1ms to increment the millisecond counter.
    /// This must be linked to the SysTick_Handler in the vector table.
    ///
    /// @note This is called from interrupt context (1000 Hz)
    static void irq_handler() {
        using namespace alloy::hal::atmel::same70::systick;
        auto* systick = SysTick();

        // IMPORTANT: Read CTRL register to clear COUNTFLAG
        // This is required by ARM Cortex-M specification
        volatile uint32_t ctrl = systick->CSR;
        (void)ctrl;  // Suppress unused warning

        // Increment millisecond counter
        // Will overflow after ~49 days (2^32 ms)
        ++millis_counter_;
    }

   private:
    /// Millisecond counter (incremented by ISR every 1ms)
    /// Volatile because it's modified by interrupt handler
    static inline volatile core::u32 millis_counter_ = 0;

    /// Reload value for sub-millisecond interpolation
    static inline core::u32 reload_value_ = 0;

    /// Initialization flag
    static inline bool initialized_ = false;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "ATSAME70Q21B SystemTick must satisfy SystemTick concept");

}  // namespace alloy::hal::same70

/// Global namespace implementation for ATSAME70Q21B
namespace alloy::systick::detail {
inline core::u32 get_micros() {
    return alloy::hal::same70::SystemTick::micros();
}
}  // namespace alloy::systick::detail

#endif  // ALLOY_HAL_SAME70_SYSTICK_HPP