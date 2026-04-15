/**
 * @file board.cpp
 * @brief SAME70 Xplained Ultra Board Implementation
 *
 * Implements hardware initialization and support for the SAME70 Xplained Ultra
 * development board. This file provides the concrete implementation of the
 * board interface defined in board.hpp.
 */

#include "board.hpp"

#include <cstdint>

#include "device/system_clock.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"
#include "hal/watchdog.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"
#include "hal/vendors/atmel/same70/watchdog_hardware_policy.hpp"

using namespace alloy::generated::atsame70q21b;
using namespace alloy::hal;

namespace board {

namespace {

using BoardLed = alloy::hal::pin<"PC8">;

auto& led_handle() {
    static auto handle = alloy::hal::gpio::open<BoardLed>({
        .direction = PinDirection::Output,
        .drive = PinDrive::PushPull,
        .pull = PinPull::None,
        .initial_state = LedConfig::led_green_active_high ? PinState::Low : PinState::High,
    });
    return handle;
}

}  // namespace

// =============================================================================
// Internal State
// =============================================================================

// Initialization flag to prevent double-init
static bool board_initialized = false;

namespace led {

void init() {
    led_handle().configure().unwrap();
    off();
}

void on() {
    if constexpr (LedConfig::led_green_active_high) {
        led_handle().set_high().unwrap();
    } else {
        led_handle().set_low().unwrap();
    }
}

void off() {
    if constexpr (LedConfig::led_green_active_high) {
        led_handle().set_low().unwrap();
    } else {
        led_handle().set_high().unwrap();
    }
}

void toggle() {
    led_handle().toggle().unwrap();
}

}  // namespace led


// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;
    }

    // Step 1: Disable watchdog timers
    // SAME70 has two independent watchdogs that must both be disabled for development
    using WDT_Policy = atmel::same70::Same70WatchdogHardwarePolicy<peripherals::WDT>;
    using RSWDT_Policy = atmel::same70::Same70WatchdogHardwarePolicy<peripherals::RSWDT>;
    Watchdog::disable<WDT_Policy>();
    Watchdog::disable<RSWDT_Policy>();

    // Step 2: Configure system clock from the published device contract
    if (!alloy::device::system_clock::apply_default()) {
        // Clock initialization failed - system will continue at default frequency
    }

    // Step 3: Initialize SysTick timer (1ms period)
    SysTickTimer::init_ms<BoardSysTick>(1);

    // Step 4: Initialize board peripherals
    led::init();

    // Step 5: Enable interrupts
    __asm volatile("cpsie i" ::: "memory");

    // Step 6: Call platform-specific late initialization hook
    alloy::hal::arm::late_init();

    board_initialized = true;
}

}  // namespace board

// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * @brief SysTick timer interrupt handler
 *
 * Called automatically every 1ms by the SysTick timer.
 * Updates the system time counter used by timing functions.
 * If RTOS is enabled, also forwards tick to RTOS scheduler.
 *
 * @note This overrides the weak default handler in startup code.
 */
/// SysTick Interrupt Handler
///
/// Called every 1ms by hardware SysTick timer.
/// Updates HAL tick counter and forwards to RTOS scheduler if enabled.
extern "C" void SysTick_Handler() {
    // Update HAL tick (always - required for HAL timing functions)
    board::BoardSysTick::increment_tick();

// Forward to RTOS scheduler (if enabled at compile time)
#ifdef ALLOY_RTOS_ENABLED
    // RTOS::tick() returns Result<void, RTOSError>
    // In ISR context, we can't handle errors gracefully, so we unwrap
    // If tick fails, it indicates a serious system error
    alloy::rtos::RTOS::tick().unwrap();
#endif
}
