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

#include "device/runtime.hpp"
#include "device/system_clock.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"

using namespace alloy::hal;

namespace board {

namespace {

using alloy::hal::detail::runtime::field_bits;
using alloy::hal::detail::runtime::field_ref;
using alloy::hal::detail::runtime::register_ref;
using alloy::hal::detail::runtime::write_register;

using BoardLed = alloy::hal::pin<"PC8">;
template <alloy::device::runtime::RegisterId RegisterId,
          alloy::device::runtime::FieldId DisableFieldId,
          alloy::device::runtime::FieldId GuardFieldId>
void disable_watchdog() {
    constexpr auto reg = register_ref<RegisterId>();
    constexpr auto disable_field = field_ref<DisableFieldId>();
    constexpr auto guard_field = field_ref<GuardFieldId>();
    constexpr auto kGuardValue = 0xFFFu;

    static_assert(reg.valid, "Selected SAME70 runtime contract must publish watchdog register.");
    static_assert(disable_field.valid,
                  "Selected SAME70 runtime contract must publish watchdog disable field.");
    static_assert(guard_field.valid,
                  "Selected SAME70 runtime contract must publish watchdog guard field.");

    const auto disable_bits = field_bits(disable_field, 1u).unwrap();
    const auto guard_bits = field_bits(guard_field, kGuardValue).unwrap();
    write_register(reg, disable_bits | guard_bits).unwrap();
}

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
    // SAME70 mode registers are write-once after reset, so disable them before other bring-up.
    disable_watchdog<alloy::device::runtime::RegisterId::register_wdt_mr,
                     alloy::device::runtime::FieldId::field_wdt_mr_wddis,
                     alloy::device::runtime::FieldId::field_wdt_mr_wdd>();
    disable_watchdog<alloy::device::runtime::RegisterId::register_rswdt_mr,
                     alloy::device::runtime::FieldId::field_rswdt_mr_wddis,
                     alloy::device::runtime::FieldId::field_rswdt_mr_allones>();

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
