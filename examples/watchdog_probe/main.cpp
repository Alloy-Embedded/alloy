#include BOARD_HEADER

#ifndef BOARD_WATCHDOG_HEADER
    #error "watchdog_probe requires BOARD_WATCHDOG_HEADER for the selected board"
#endif

#include BOARD_WATCHDOG_HEADER

#include "hal/systick.hpp"

// ---------------------------------------------------------------------------
// Watchdog probe — demonstrates the public watchdog HAL
//
// Strategy:
//   • Open a typed watchdog handle via board::make_watchdog().
//   • Static-assert that the selected peripheral is present in the published
//     device descriptor (compile-time safety check).
//   • On boards that support software disable (same70 WDT), pass
//     Config{.disable_on_configure = true} to make_watchdog() so the
//     disable path is exercised at construction time.
//   • Call refresh() in the main loop — safe on every platform:
//       STM32 IWDG:  reload key write (0xAAAA) is a no-op when IWDG has
//                    not been started (no 0xCCCC write)
//       same70 WDT:  restart write is benign when WDT is disabled
//   • enable() is intentionally NOT called: IWDG cannot be stopped once
//     armed and the probe must compile and run on all boards without
//     causing an unrecoverable reset-loop.
// ---------------------------------------------------------------------------

static_assert(board::BoardWatchdog::valid,
              "BoardWatchdog peripheral is not present in the published "
              "device descriptor — check board_watchdog.hpp");

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

    // On platforms that support software disable (same70 WDT / RSWDT),
    // open the handle with disable_on_configure = true.  On STM32 IWDG the
    // disable step is skipped because Config::disable_on_configure is false.
    constexpr alloy::hal::watchdog::Config kOpenConfig{
        .disable_on_configure = board::kBoardWatchdogCanDisable,
        .refresh_on_configure = false,
    };

    auto wdt = board::make_watchdog(kOpenConfig);

    // Explicit configure() call to demonstrate the reconfigure path and
    // surface any error.
    if (const auto r = wdt.configure({.refresh_on_configure = true}); !r.is_ok()) {
        blink_error(200);
    }

    // Main loop — keep refreshing to demonstrate a real keep-alive pattern
    // without actually arming the watchdog.
    std::uint32_t count = 0u;
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);

        // Refresh every second (every other 500 ms toggle).
        if ((count & 1u) == 0u) {
            [[maybe_unused]] const auto r = wdt.refresh();
        }
        ++count;
    }
}
