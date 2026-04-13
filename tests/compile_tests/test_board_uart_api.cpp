#if defined(BOARD_UART_HEADER)
    #include BOARD_UART_HEADER
#else
    #define ALLOY_BOARD_UART_COMPILE_SMOKE_UNAVAILABLE 1
#endif

namespace {

[[maybe_unused]] void compile_probe_board_uart_api() {
#if defined(BOARD_UART_HEADER)
    auto uart = board::make_debug_uart();
    [[maybe_unused]] const auto configure_result = uart.configure();
#endif
}

}  // namespace
