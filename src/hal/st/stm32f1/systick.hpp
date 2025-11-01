/// STM32F1 SysTick Timer Implementation
///
/// Uses ARM Cortex-M3 SysTick peripheral for microsecond time tracking.
///
/// Hardware Resources:
/// - ARM Cortex-M SysTick peripheral (24-bit down-counter)
/// - SysTick exception (vector 15)
///
/// Configuration:
/// - Clock source: CPU / 8 (72MHz / 8 = 9MHz)
/// - Interrupt every 1ms (reload = 9000)
/// - Software counter for milliseconds
/// - Sub-millisecond from SysTick->VAL register
///
/// Memory Usage:
/// - 4 bytes RAM (volatile counter)
/// - ~200 bytes code

#ifndef ALLOY_HAL_STM32F1_SYSTICK_HPP
#define ALLOY_HAL_STM32F1_SYSTICK_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::hal::st::stm32f1 {

// ARM Cortex-M SysTick Peripheral (core peripheral, not device-specific)
namespace systick_regs {
    struct SysTick_Type {
        volatile core::u32 CTRL;   // Offset: 0x000 (R/W)  SysTick Control and Status Register
        volatile core::u32 LOAD;   // Offset: 0x004 (R/W)  SysTick Reload Value Register
        volatile core::u32 VAL;    // Offset: 0x008 (R/W)  SysTick Current Value Register
        volatile core::u32 CALIB;  // Offset: 0x00C (R/ )  SysTick Calibration Register
    };

    // SysTick Control / Status Register Definitions
    constexpr core::u32 CTRL_ENABLE_Msk     = (1UL << 0);
    constexpr core::u32 CTRL_TICKINT_Msk    = (1UL << 1);
    constexpr core::u32 CTRL_CLKSOURCE_Msk  = (1UL << 2);
    constexpr core::u32 CTRL_COUNTFLAG_Msk  = (1UL << 16);

    // SysTick base address (ARM Cortex-M standard)
    constexpr core::u32 SYSTICK_BASE = 0xE000E010UL;
    inline SysTick_Type* const SysTick = reinterpret_cast<SysTick_Type*>(SYSTICK_BASE);
}
using namespace systick_regs;

/// STM32F1 SysTick implementation
///
/// @note This class is stateless - all state is in static/volatile variables
class SystemTick {
public:
    /// Initialize SysTick timer
    ///
    /// Configures SysTick for 1MHz tick (1us resolution) using:
    /// - CPU clock / 8 = 9MHz (at 72MHz CPU)
    /// - Reload value 9000 for 1ms interrupts
    /// - Interrupt enabled
    ///
    /// @return Ok on success
    static core::Result<void> init() {
        // Disable SysTick during configuration
        SysTick->CTRL = 0;

        // Configure reload value for 1ms at 9MHz
        // 9MHz / 1000 = 9000 ticks per millisecond
        constexpr core::u32 RELOAD_VALUE = 9000 - 1;
        SysTick->LOAD = RELOAD_VALUE;

        // Reset counter
        SysTick->VAL = 0;

        // Reset software counter
        micros_counter_ = 0;

        // Enable SysTick with interrupt
        // CLKSOURCE=0 (CPU/8), TICKINT=1 (interrupt), ENABLE=1
        SysTick->CTRL = CTRL_TICKINT_Msk | CTRL_ENABLE_Msk;

        initialized_ = true;

        return core::Result<void>::ok();
    }

    /// Get current time in microseconds
    ///
    /// Combines millisecond counter with sub-millisecond interpolation
    /// from SysTick hardware counter.
    ///
    /// Thread-safe: Yes (atomic read on ARM Cortex-M)
    ///
    /// @return Microseconds since init
    static core::u32 micros() {
        if (!initialized_) {
            return 0;
        }

        // Read millisecond counter (atomic on ARM Cortex-M)
        core::u32 ms = micros_counter_ / 1000;

        // Read SysTick current value (counts down from RELOAD)
        // At 9MHz: each tick is ~111ns, so we divide by 9 to get microseconds
        core::u32 ticks_down = SysTick->VAL;
        core::u32 ticks_elapsed = 9000 - ticks_down;
        core::u32 us_frac = ticks_elapsed / 9;

        return (ms * 1000) + us_frac;
    }

    /// Reset counter to zero
    ///
    /// @return Ok on success
    static core::Result<void> reset() {
        micros_counter_ = 0;
        SysTick->VAL = 0;
        return core::Result<void>::ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called
    static bool is_initialized() {
        return initialized_;
    }

    /// SysTick interrupt handler
    ///
    /// Called every 1ms. Increments software counter.
    /// This function is called from the SysTick_Handler ISR.
    ///
    /// @note Declared public for ISR access, but users should not call directly
    static void handle_interrupt() {
        // Increment by 1000us (1ms)
        micros_counter_ += 1000;
    }

private:
    /// Software microsecond counter (incremented in ISR)
    static inline volatile core::u32 micros_counter_ = 0;

    /// Initialization flag
    static inline bool initialized_ = false;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "STM32F1 SystemTick must satisfy SystemTick concept");

} // namespace alloy::hal::st::stm32f1

/// Global namespace implementation for STM32F1
namespace alloy::systick::detail {
    inline core::u32 get_micros() {
        return alloy::hal::st::stm32f1::SystemTick::micros();
    }
}

/// SysTick interrupt handler (C linkage for vector table)
///
/// This is the actual ISR that gets called by hardware.
/// It delegates to the SystemTick class.
extern "C" {
    inline void SysTick_Handler() {
        alloy::hal::st::stm32f1::SystemTick::handle_interrupt();
    }
}

#endif // ALLOY_HAL_STM32F1_SYSTICK_HPP
