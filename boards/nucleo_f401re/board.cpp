/**
 * @file board.cpp
 * @brief Nucleo-F401RE Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32f4/clock_platform.hpp"
#include <cstdint>

using namespace alloy::hal::st::stm32f4;
using namespace alloy::generated::stm32f401;
using namespace alloy::hal;

// Board clock type using config from board_config.hpp
using BoardClock = Stm32f4Clock<nucleo_f401re::ClockConfig>;

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

// Initialization flag to prevent double-init
static bool board_initialized = false;

// =============================================================================
// Clock Configuration (using Clock Policy)
// =============================================================================

/**
 * @brief Configure system clock using BoardClock policy
 *
 * Uses the Stm32f4Clock policy template with configuration from board_config.hpp.
 * This provides a clean, type-safe interface with compile-time validation.
 *
 * Clock configuration is defined in ClockConfig struct in board_config.hpp.
 */
static inline void configure_system_clock() {
    auto result = BoardClock::initialize();
    // Note: In a production system, we would handle the error.
    // For now, if clock init fails, the MCU will run at default HSI speed.
    (void)result;  // Suppress unused warning
}

/**
 * @brief Enable all GPIO peripheral clocks using BoardClock policy
 *
 * Enables GPIOA through GPIOE and GPIOH clocks via the clock policy interface.
 */
static inline void enable_gpio_clocks() {
    BoardClock::enable_gpio_clocks();
}

// =============================================================================
// LED Control Implementation
// =============================================================================

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

} // namespace led

// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;
    }

    // Step 1: Configure system clock to 84 MHz
    configure_system_clock();

    // Step 2: Enable GPIO peripheral clocks
    enable_gpio_clocks();

    // Step 3: Initialize SysTick timer (1ms period)
    SysTickTimer::init_ms<BoardSysTick>(1);

    // Step 4: Initialize board peripherals
    led::init();

    // Step 5: Enable interrupts globally
    __asm volatile ("cpsie i" ::: "memory");

    board_initialized = true;
}

} // namespace board

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
