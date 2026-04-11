#if defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#else
    #define ALLOY_BOARD_LED_COMPILE_SMOKE_UNAVAILABLE 1
#endif

namespace {

[[maybe_unused]] void compile_probe_board_led_api() {
#if defined(ALLOY_BOARD_NUCLEO_G0B1RE) || defined(ALLOY_BOARD_NUCLEO_G071RB) || \
    defined(ALLOY_BOARD_NUCLEO_F401RE) || defined(ALLOY_BOARD_SAME70_XPLD)
    board::led::init();
    board::led::on();
    board::led::off();
    board::led::toggle();
#endif
}

}  // namespace
