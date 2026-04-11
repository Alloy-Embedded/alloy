/**
 * @file board.cpp
 * @brief Nucleo-G071RB Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-G071RB
 * development board (MB1360).
 */

#include "board.hpp"

#include <cstdint>

#include "hal/systick.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/flash_bitfields.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/rcc_registers.hpp"

using namespace alloy::hal::st::stm32g0;
using namespace alloy::generated::stm32g0b1;
using namespace alloy::hal;

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

// =============================================================================
// RCC (Reset and Clock Control) Helper Functions
// =============================================================================

static inline void configure_system_clock() {
    using namespace rcc;  // Use RCC bitfields namespace

    // 1. Enable HSI16 (should already be enabled after reset)
    rcc::RCC()->CR |= cr::HSION::mask;
    while (!(rcc::RCC()->CR & cr::HSIRDY::mask))
        ;  // Wait for HSIRDY

    // 2. Configure flash latency BEFORE increasing frequency
    // For 64 MHz on STM32G0, we need 2 wait states
    flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, 2);

    // 3. Configure PLL: HSI16 as source, M=/1, N=×8, R=/2
    // PLLSRC=2 (HSI16), PLLM=0 (div by 1), PLLN=8 (mul by 8), PLLR=0 (div by 2)
    rcc::RCC()->PLLCFGR = pllcfgr::PLLSRC::write(0, 2) |  // HSI16 = 0b10
                          pllcfgr::PLLM::write(0, 0) |    // /1 = 0
                          pllcfgr::PLLN::write(0, 8) |    // ×8 = 8
                          pllcfgr::PLLR::write(0, 0) |    // /2 = 0
                          pllcfgr::PLLREN::mask;          // Enable PLLR output

    // 4. Enable PLL
    rcc::RCC()->CR |= cr::PLLON::mask;
    while (!(rcc::RCC()->CR & cr::PLLRDY::mask))
        ;  // Wait for PLLRDY

    // 5. Switch system clock to PLL (SW = 2 means PLL)
    rcc::RCC()->CFGR = cfgr::SW::write(rcc::RCC()->CFGR, 2);
    while (cfgr::SWS::read(rcc::RCC()->CFGR) != 2)
        ;  // Wait for SWS = PLL
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
