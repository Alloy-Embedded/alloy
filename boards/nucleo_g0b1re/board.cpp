/**
 * @file board.cpp
 * @brief Nucleo-G0B1RE Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-G0B1RE
 * development board (MB1360).
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/flash_bitfields.hpp"
#include <cstdint>

using namespace alloy::hal::st::stm32g0;
using namespace alloy::generated::stm32g0b1;
using namespace alloy::hal;

namespace board {

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

/**
 * @brief Configure system clock to 64 MHz using PLL from HSI16
 *
 * Clock tree:
 * - HSI16 (16 MHz internal oscillator) → PLL → 64 MHz system clock
 * - PLL configuration: HSI16 / 1 (M) × 8 (N) / 2 (R) = 64 MHz
 */
static inline void configure_system_clock() {
    using namespace rcc;  // Use RCC bitfields namespace

    // 1. Enable HSI16 (should already be enabled after reset)
    rcc::RCC()->CR |= cr::HSION::mask;
    while (!(rcc::RCC()->CR & cr::HSIRDY::mask));  // Wait for HSIRDY

    // 2. Configure PLL: HSI16 as source, M=/1, N=×8, R=/2
    // Note: PLLSRC=2 (HSI16), PLLM=0 (div by 1), PLLN=8 (mul by 8), PLLR=0 (div by 2)
    rcc::RCC()->PLLCFGR = pllcfgr::PLLSRC::write(0, 2) |   // HSI16 = 0b10
                          pllcfgr::PLLM::write(0, 0) |     // /1 = 0
                          pllcfgr::PLLN::write(0, 8) |     // ×8 = 8
                          pllcfgr::PLLR::write(0, 0) |     // /2 = 0
                          pllcfgr::PLLREN::mask;           // Enable PLLR output

    // 3. Enable PLL
    rcc::RCC()->CR |= cr::PLLON::mask;
    while (!(rcc::RCC()->CR & cr::PLLRDY::mask));  // Wait for PLLRDY

    // 4. Configure flash latency for 64 MHz (2 wait states)
    flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, 2);

    // 5. Switch system clock to PLL (SW = 2 means PLL)
    rcc::RCC()->CFGR = cfgr::SW::write(rcc::RCC()->CFGR, 2);
    while (cfgr::SWS::read(rcc::RCC()->CFGR) != 2);  // Wait for SWS = PLL
}

static inline void enable_gpio_clocks() {
    using namespace rcc;  // Use RCC bitfields namespace

    // Enable all GPIO port clocks (GPIOA-GPIOF)
    rcc::RCC()->IOPENR |= iopenr::GPIOAEN::mask |
                          iopenr::GPIOBEN::mask |
                          iopenr::GPIOCEN::mask |
                          iopenr::GPIODEN::mask |
                          iopenr::GPIOEEN::mask |
                          iopenr::GPIOFEN::mask;
}

// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;
    }

    // Step 1: Configure system clock to 64 MHz
    configure_system_clock();

    // Step 2: Enable GPIO peripheral clocks
    enable_gpio_clocks();

    // Step 3: Initialize SysTick timer (1ms period)
    SysTickTimer::init_ms<BoardSysTick>(1);

    // Step 4: Initialize board peripherals
    led::init();

    // Step 5: Enable interrupts globally (PRIMASK = 0)
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
