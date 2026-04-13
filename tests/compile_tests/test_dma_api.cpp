#include "device/dma.hpp"
#include "hal/claim.hpp"
#include "hal/dma.hpp"

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
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART2, SignalId::signal_TX>::kPresent);
using UartTxDma = alloy::hal::dma::channel_handle<PeripheralId::USART2, SignalId::signal_TX>;
using UartTxDmaClaim = alloy::hal::claim::dma_claim<PeripheralId::USART2, SignalId::signal_TX>;
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART0, SignalId::signal_TX>::kPresent);
using UartTxDma = alloy::hal::dma::channel_handle<PeripheralId::USART0, SignalId::signal_TX>;
using UartTxDmaClaim = alloy::hal::claim::dma_claim<PeripheralId::USART0, SignalId::signal_TX>;
#endif

static_assert(UartTxDma::valid);
static_assert(UartTxDma::descriptor().request_line_id != alloy::hal::dma::DmaRequestLineId::none);
static_assert(UartTxDmaClaim::request_line_id != alloy::hal::dma::DmaRequestLineId::none);
#endif

int main() { return 0; }
