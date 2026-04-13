/**
 * @file board.cpp
 * @brief Nucleo-G0B1RE Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-G0B1RE
 * development board (MB1360).
 */

#include "board.hpp"

#include <cstdint>

#include "hal/clock.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"
#include "hal/vendors/st/stm32g0/clock_platform.hpp"

using namespace alloy::hal::st::stm32g0;
using namespace alloy::hal;

using BoardClock = Stm32g0Clock<nucleo_g0b1re::ClockConfig>;

namespace board {

namespace {

using BoardLed = alloy::hal::pin<"PA5">;

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

// SysTick instance for timing (64 MHz clock)
using BoardSysTick = SysTick<64000000>;

// Initialization flag to prevent double-init
static bool board_initialized = false;

static inline void configure_system_clock() {
    static_cast<void>(SystemClock::use_default<BoardClock>());
}

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

    // Step 1: Configure system clock to 64 MHz
    configure_system_clock();

    // Step 2: Initialize SysTick timer (1ms period)
    SysTickTimer::init_ms<BoardSysTick>(1);

    // Step 3: Initialize board peripherals
    led::init();

    // Step 4: Enable interrupts globally (PRIMASK = 0)
    __asm volatile("cpsie i" ::: "memory");

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
