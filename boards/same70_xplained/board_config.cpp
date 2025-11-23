/**
 * @file board_config.cpp
 * @brief SAME70 Xplained Ultra Board Configuration Implementation
 *
 * This file contains the implementation of interrupt handlers and
 * board initialization code.
 */

#include "board_config.hpp"
#include "hal/vendors/arm/same70/clock.hpp"
#include "hal/vendors/arm/same70/systick.hpp"

namespace alloy::boards::same70_xplained {
namespace board {

using namespace ucore::hal::same70;
using namespace detail;

// Define the initialization flag
namespace detail {
    bool initialized = false;
}

void init(ClockPreset preset) {
    // CRITICAL: Disable Watchdog Timer first!
    // The watchdog is enabled by default and will reset the system
    // Following modm pattern for SAME70
    volatile uint32_t* WDT_MR = (volatile uint32_t*)0x400E1854;
    *WDT_MR = (1 << 15);  // WDDIS bit - disable watchdog

    if (initialized) {
        return;  // Already initialized
    }

    // Enable PIOC early for LED
    volatile uint32_t* PMC_PCER0 = (volatile uint32_t*)0x400E0610;
    *PMC_PCER0 = (1u << 12);  // PIOC

    led0_instance.setDirection(PinDirection::Output);
    led0_instance.set();  // OFF (active-low)

    /**
     * SAME70 Clock Configuration
     *
     * Crystal: 12 MHz (external XTAL on board)
     * Target: 300 MHz CPU clock (PLLA output)
     * Master Clock (MCK): 150 MHz (PLLA/2)
     *
     * PLL Configuration:
     *   Input:  12 MHz crystal
     *   PLLA:   12 MHz × (24+1) / 1 = 300 MHz
     *   MCK:    300 MHz / 2 = 150 MHz
     *
     * Flash Wait States: Configured automatically by hardware for 150 MHz
     * Per ATSAME70 datasheet Table 59-50:
     *   - 150 MHz @ 3.3V requires 6 wait states
     *
     * Performance: 300 MHz core, 150 MHz bus
     * Note: MCK limited to 150 MHz per datasheet, but PLLA can run at 300 MHz
     */
    ClockConfig clock_config = Clock::CLOCK_CONFIG_HIGH_PERFORMANCE;  // 300 MHz PLLA -> 150 MHz MCK
    auto clock_result = Clock::initialize(clock_config);
    if (!clock_result.is_ok()) {
        // Clock failed - blink fast forever
        while (1) {
            led0_instance.toggle();
            for (volatile int i = 0; i < 50000; i++);
        }
    }

    // Initialize SysTick Timer
    auto systick_result = SystemTick::init();
    if (!systick_result.is_ok()) {
        // SysTick failed - blink very fast forever
        while (1) {
            led0_instance.toggle();
            for (volatile int i = 0; i < 25000; i++);
        }
    }

    // Enable All Peripheral Clocks for GPIO
    *PMC_PCER0 |= (1u << 10)  // PIOA
                | (1u << 11)  // PIOB
                // PIOC already enabled
                | (1u << 13)  // PIOD
                | (1u << 14); // PIOE

    // Configure LEDs (both must be set up for debug)
    led0_instance.setDirection(PinDirection::Output);
    led0_instance.set();  // OFF (active-low)

    led1_instance.setDirection(PinDirection::Output);
    led1_instance.set();  // OFF (active-low) - will toggle in SysTick ISR for debug

    // Configure Buttons
    button0_instance.setDirection(PinDirection::Input);
    button0_instance.setPull(PinPull::PullUp);

    button1_instance.setDirection(PinDirection::Input);
    button1_instance.setPull(PinPull::PullUp);

    // CRITICAL: Enable global interrupts at CPU level
    // This must be done AFTER all initialization is complete
    // Without this, SysTick interrupts will never fire
    __asm volatile("cpsie i" ::: "memory");

    initialized = true;
}

} // namespace board
} // namespace alloy::boards::same70_xplained

// SysTick interrupt handler
// This overrides the weak alias in startup.cpp
extern "C" void SysTick_Handler() {
    ucore::hal::same70::SystemTick::irq_handler();
}
