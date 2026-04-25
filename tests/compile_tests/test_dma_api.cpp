#include "hal/claim.hpp"
#include "hal/dma.hpp"

#include "device/dma.hpp"

static_assert(alloy::device::SelectedDmaBindings::available,
              "Selected device must publish typed DMA bindings.");

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
static_assert(!alloy::device::dma::bindings.empty());

using PeripheralId = alloy::hal::dma::PeripheralId;
using SignalId = alloy::hal::dma::SignalId;

    #if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART1, SignalId::signal_TX>::kPresent);
using UartTxDma = alloy::hal::dma::channel_handle<PeripheralId::USART1, SignalId::signal_TX>;
using UartTxDmaClaim = alloy::hal::claim::dma_claim<PeripheralId::USART1, SignalId::signal_TX>;
constexpr auto kDmaConfig = alloy::hal::dma::Config{};
    #elif defined(ALLOY_BOARD_NUCLEO_F401RE)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART2, SignalId::signal_TX>::kPresent);
using UartTxDma = alloy::hal::dma::channel_handle<PeripheralId::USART2, SignalId::signal_TX>;
using UartTxDmaClaim = alloy::hal::claim::dma_claim<PeripheralId::USART2, SignalId::signal_TX>;
constexpr auto kDmaConfig = alloy::hal::dma::Config{};
    #elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART1, SignalId::signal_TX>::kPresent);
using UartTxDma = alloy::hal::dma::channel_handle<PeripheralId::USART1, SignalId::signal_TX>;
using UartTxDmaClaim = alloy::hal::claim::dma_claim<PeripheralId::USART1, SignalId::signal_TX>;
constexpr auto kDmaConfig = alloy::hal::dma::Config{.channel_index = 0};
    #elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
// RP2040: DMA bindings exist; DMA semantic traits not yet fully generated
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::UART0, SignalId::signal_TX>::kPresent);
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::UART0, SignalId::signal_RX>::kPresent);
    #endif

// Full semantic-layer assertions — only for boards with complete DMA semantics
#if !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
static_assert(UartTxDma::valid);
static_assert(UartTxDma::descriptor().request_line_id != alloy::hal::dma::DmaRequestLineId::none);
static_assert(UartTxDmaClaim::request_line_id != alloy::hal::dma::DmaRequestLineId::none);
#endif
#endif

int main() {
#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE && !defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    [[maybe_unused]] auto dma =
        alloy::hal::dma::open<UartTxDma::peripheral_id, UartTxDma::signal_id>(kDmaConfig);
    [[maybe_unused]] const auto configure_result = dma.configure();
#endif
    return 0;
}
