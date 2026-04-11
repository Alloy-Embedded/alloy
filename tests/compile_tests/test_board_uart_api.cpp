#if defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#else
    #define ALLOY_BOARD_UART_COMPILE_SMOKE_UNAVAILABLE 1
#endif

namespace {

[[maybe_unused]] void compile_probe_board_uart_api() {
#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_F401RE) || \
    defined(ALLOY_BOARD_SAME70_XPLD)
    auto uart = board::make_debug_uart();
    [[maybe_unused]] const auto configure_result = uart.configure();
#endif
}

}  // namespace
