/**
 * @file main.cpp
 * @brief Universal LED Blink Example — lite HAL
 *
 * Board-independent LED blink using the Alloy HAL lite API.
 * Works on any supported board by changing ALLOY_BOARD at build time.
 *
 * Supported boards:
 *   same70_xplained  — SAME70 Xplained Ultra (green LED PC8)
 *   nucleo_g0b1re    — STM32 Nucleo-G0B1RE   (green LED PA5)
 *   nucleo_g071rb    — STM32 Nucleo-G071RB   (green LED PA5)
 *   nucleo_f401re    — STM32 Nucleo-F401RE   (green LED PA5)
 *   nucleo_f722ze    — STM32 Nucleo-F722ZE   (green LED PB0)
 */

#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F722ZE)
    #include "nucleo_f722ze/board.hpp"
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    #include "raspberry_pi_pico/board.hpp"
#elif defined(ALLOY_BOARD_ESP32_DEVKIT)
    #include "esp32_devkit/board.hpp"
#elif defined(ALLOY_BOARD_ESP_WROVER_KIT)
    #include "esp_wrover_kit/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#include <cstdint>

namespace {

// ~500 ms busy-wait at typical MCU clock frequencies (adjust per board).
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

    return 0;
}
