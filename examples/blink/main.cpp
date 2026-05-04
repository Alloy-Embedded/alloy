/**
 * @file main.cpp
 * @brief Universal LED Blink — Alloy lite HAL
 *
 * Board-independent LED blink.  The same source compiles for every supported
 * board; the build system selects the right board header via ALLOY_BOARD_*.
 *
 * Build & flash:
 *   python3 scripts/alloyctl.py flash --board <board> --target blink --build-first
 */

// clang-format off
#if   defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    #include "raspberry_pi_pico/board.hpp"
#elif defined(ALLOY_BOARD_ESP32_DEVKIT)
    #include "esp32_devkit/board.hpp"
#elif defined(ALLOY_BOARD_ESP_WROVER_KIT)
    #include "esp_wrover_kit/board.hpp"
#elif defined(ALLOY_BOARD_ESP32C3_DEVKITM)
    #include "esp32c3_devkitm/board.hpp"
#elif defined(ALLOY_BOARD_ESP32S3_DEVKITC)
    #include "esp32s3_devkitc/board.hpp"
#elif defined(ALLOY_BOARD_AVR128DA32_CURIOSITY_NANO)
    #include "avr128da32_curiosity_nano/board.hpp"
#else
    #error "Unsupported board. Pass -DALLOY_BOARD=<board> or use alloyctl."
#endif
// clang-format on

#include <cstdint>

namespace {

// Busy-wait ~500 ms at typical board clock speeds.
// Replaced by a SysTick delay once that example lands.
inline void busy_delay() noexcept {
    for (volatile std::uint32_t i = 0u; i < 2'000'000u; ++i) {}
}

}  // namespace

int main() {
    board::init();

    while (true) {
        board::led::toggle();
        busy_delay();
    }
}
