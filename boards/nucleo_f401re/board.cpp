/**
 * @file board.cpp
 * @brief Nucleo-F401RE Board Implementation
 *
 * Implements hardware initialization and support for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32f4/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32f4/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32f4/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32f4/bitfields/flash_bitfields.hpp"
#include <cstdint>

using namespace alloy::hal::st::stm32f4;
using namespace alloy::generated::stm32f401;
using namespace alloy::hal;

namespace board {

// =============================================================================
// Internal State
// =============================================================================

// Initialization flag to prevent double-init
static bool board_initialized = false;

// LED instance
static LedConfig::led_green led_pin;

// =============================================================================
// RCC (Reset and Clock Control) Helper Functions
// =============================================================================

/**
 * @brief Configure system clock to 84 MHz using PLL from HSE
 *
 * Clock tree:
 * - HSE (8 MHz external crystal) → PLL → 84 MHz system clock
 * - PLL configuration: HSE (8 MHz) / M (4) × N (168) / P (2) = 84 MHz
 */
static inline void configure_system_clock() {
    using namespace rcc;  // Use RCC bitfields namespace

    // 1. Enable HSE (8 MHz external crystal)
    rcc::RCC()->CR |= cr::HSEON::mask;
    while (!(rcc::RCC()->CR & cr::HSERDY::mask));  // Wait for HSERDY

    // 2. Configure flash latency BEFORE increasing frequency
    // For 84 MHz at 3.3V, we need 2 wait states
    flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, 2);

    // 3. Configure PLL: HSE as source
    // PLLM = 4 (HSE / 4 = 2 MHz)
    // PLLN = 168 (2 MHz × 168 = 336 MHz)
    // PLLP = /4 (336 MHz / 4 = 84 MHz)
    // PLLQ = 7 (default for USB, not used here)

    // Using: HSE(8MHz) / M(4) = 2 MHz, × N(168) = 336 MHz, / P(4) = 84 MHz
    // PLLCFGR register layout:
    // - PLLM[5:0] = 4 (bits 0-5)
    // - PLLN[8:0] = 168 (bits 6-14)
    // - PLLP[1:0] = 1 for /4 (bits 16-17) - 0=/2, 1=/4, 2=/6, 3=/8
    // - PLLSRC = 1 for HSE (bit 22)
    // - PLLQ[3:0] = 7 (bits 24-27)
    uint32_t pllcfgr_value = (4 << 0) |        // PLLM = 4
                              (168 << 6) |      // PLLN = 168
                              (1 << 16) |       // PLLP = /4
                              (1 << 22) |       // PLLSRC = HSE
                              (7 << 24);        // PLLQ = 7
    rcc::RCC()->PLLCFGR = pllcfgr_value;

    // 4. Enable PLL
    rcc::RCC()->CR |= cr::PLLON::mask;
    while (!(rcc::RCC()->CR & cr::PLLRDY::mask));  // Wait for PLLRDY

    // 5. Configure bus prescalers
    // AHB = SYSCLK / 1 = 84 MHz (HPRE = 0)
    // APB1 = AHB / 2 = 42 MHz (max 42 MHz for STM32F401) (PPRE1 = 0b100 = 4)
    // APB2 = AHB / 1 = 84 MHz (PPRE2 = 0)
    // CFGR register layout:
    // - HPRE[3:0] = 0 for /1 (bits 4-7)
    // - PPRE1[2:0] = 4 for /2 (bits 10-12)
    // - PPRE2[2:0] = 0 for /1 (bits 13-15)
    uint32_t cfgr_value = rcc::RCC()->CFGR;
    cfgr_value &= ~((0xF << 4) | (0x7 << 10) | (0x7 << 13));  // Clear prescaler bits
    cfgr_value |= (0 << 4) |   // HPRE = 0 (AHB /1)
                  (4 << 10) |  // PPRE1 = 4 (APB1 /2)
                  (0 << 13);   // PPRE2 = 0 (APB2 /1)
    rcc::RCC()->CFGR = cfgr_value;

    // 6. Switch system clock to PLL (SW = 2 means PLL)
    // SW[1:0] bits 0-1, value 2 = 0b10 for PLL
    cfgr_value = rcc::RCC()->CFGR;
    cfgr_value &= ~(0x3 << 0);  // Clear SW bits
    cfgr_value |= (2 << 0);     // SW = 2 (PLL)
    rcc::RCC()->CFGR = cfgr_value;

    // Wait for SWS = 2 (bits 2-3)
    while (((rcc::RCC()->CFGR >> 2) & 0x3) != 2);
}

static inline void enable_gpio_clocks() {
    using namespace rcc;  // Use RCC bitfields namespace

    // Enable GPIO port clocks (GPIOA-GPIOE, GPIOH)
    rcc::RCC()->AHB1ENR |= ahb1enr::GPIOAEN::mask |
                           ahb1enr::GPIOBEN::mask |
                           ahb1enr::GPIOCEN::mask |
                           ahb1enr::GPIODEN::mask |
                           ahb1enr::GPIOEEN::mask |
                           ahb1enr::GPIOHEN::mask;
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
 *
 * @note This overrides the weak default handler in startup code.
 */
extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();
}
