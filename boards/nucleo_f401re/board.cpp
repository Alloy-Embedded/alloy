/**
 * @file board.cpp
 * @brief Nucleo-F401RE Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/platform/st/stm32f4/clock_platform.hpp"
#include <cstdint>

using namespace alloy::hal::st::stm32f4;
using namespace alloy::generated::stm32f401;
using namespace alloy::hal;

// Board clock type using config from board_config.hpp
using BoardClock = Stm32f4Clock<nucleo_f401re::ClockConfig>;

namespace board {

// =============================================================================
// Internal State
// =============================================================================

// Initialization flag to prevent double-init
static bool board_initialized = false;

// LED instance
static LedConfig::led_green led_pin;

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
    led_pin.setDirection(PinDirection::Output);
    led_pin.setPull(PinPull::None);
    led_pin.setDrive(PinDrive::PushPull);
    off();  // Start with LED off
}

void on() {
    if (LedConfig::led_green_active_high) {
        led_pin.set();
    } else {
        led_pin.clear();
    }
}

void off() {
    if (LedConfig::led_green_active_high) {
        led_pin.clear();
    } else {
        led_pin.set();
    }
}

void toggle() {
    led_pin.toggle();
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
extern "C" void SysTick_Handler() {
    // Update HAL tick (always)
    board::BoardSysTick::increment_tick();

    // Forward to RTOS scheduler (if enabled)
    #ifdef ALLOY_RTOS_ENABLED
        alloy::rtos::RTOS::tick();
    #endif
}
