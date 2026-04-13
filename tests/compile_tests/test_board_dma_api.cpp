#if defined(BOARD_DMA_HEADER)
    #include BOARD_DMA_HEADER
#else
    #define ALLOY_BOARD_DMA_COMPILE_SMOKE_UNAVAILABLE 1
#endif

namespace {

[[maybe_unused]] void compile_probe_board_dma_api() {
#if defined(BOARD_DMA_HEADER)
    auto tx_dma = board::make_debug_uart_tx_dma();
    auto rx_dma = board::make_debug_uart_rx_dma();
    [[maybe_unused]] const auto tx_config = tx_dma.config();
    [[maybe_unused]] const auto rx_config = rx_dma.config();
    static_assert(decltype(tx_dma)::valid);
    static_assert(decltype(rx_dma)::valid);
#endif
}

}  // namespace
