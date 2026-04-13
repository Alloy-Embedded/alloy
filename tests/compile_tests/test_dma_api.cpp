#include "device/dma.hpp"
#include "hal/dma.hpp"

static_assert(alloy::device::SelectedDmaBindings::available,
              "Selected device must publish typed DMA bindings.");

#if ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE
static_assert(!alloy::device::dma::bindings.empty());

using PeripheralId = alloy::hal::dma::PeripheralId;
using SignalId = alloy::hal::dma::SignalId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART1, SignalId::signal_TX>::kPresent);
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART2, SignalId::signal_TX>::kPresent);
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
static_assert(alloy::hal::dma::BindingTraits<PeripheralId::USART0, SignalId::signal_TX>::kPresent);
#endif
#endif

int main() { return 0; }
