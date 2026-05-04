#include <array>
#include <string_view>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "device/traits.hpp"

namespace {

template <typename UartHandle>
consteval auto uart_is_usable() -> bool {
    if constexpr (!UartHandle::valid) {
        return false;
    }

    return !UartHandle::peripheral_name.empty() && UartHandle::base_address() != 0u &&
           UartHandle::operations().size() >= 2;
}

template <typename UartHandle>
void exercise_uart_backend(std::uint32_t peripheral_clock_hz) {
    // ---- irq_numbers: compile smoke (task 2.5) ----
    // Verifies the method compiles and returns the correct type on all backends.
    // Size may be 0 when the device database has not yet published IRQ lines for
    // a particular peripheral (e.g. F401 USART2).
    [[maybe_unused]] const auto irq_span = UartHandle::irq_numbers();
    static_assert(std::is_same_v<decltype(irq_span), const std::span<const std::uint32_t>>);
    // When IRQ lines are published, first entry must be a plausible NVIC line.
    if constexpr (UartHandle::irq_numbers().size() > 0u) {
        static_assert(UartHandle::irq_numbers()[0] < 512u);
    }
    auto uart = UartHandle{
        {
            .baudrate = alloy::hal::Baudrate::e115200,
            .data_bits = alloy::hal::DataBits::Eight,
            .parity = alloy::hal::Parity::None,
            .stop_bits = alloy::hal::StopBits::One,
            .flow_control = alloy::hal::FlowControl::None,
            .peripheral_clock_hz = peripheral_clock_hz,
        },
    };

    std::array<std::byte, 3> tx_buffer{
        std::byte{0x41},
        std::byte{0x6Cu},
        std::byte{0x6Cu},
    };
    std::array<std::byte, 4> rx_buffer{};

    [[maybe_unused]] const auto configure_result = uart.configure();
    [[maybe_unused]] const auto write_result = uart.write(std::span<const std::byte>{tx_buffer});
    [[maybe_unused]] const auto write_byte_result = uart.write_byte(std::byte{0x79});
    [[maybe_unused]] const auto read_result = uart.read(std::span<std::byte>{rx_buffer});
    [[maybe_unused]] const auto flush_result = uart.flush();

    // ---- Phase 1: baudrate / oversampling ----
    // set_baudrate: raw Hz value — compiles on all backends.
    [[maybe_unused]] const auto baud_result = uart.set_baudrate(921'600u);
    // set_oversampling: typed enum — compiles on all backends; returns
    // NotSupported when OVER8 field absent (e.g. Microchip USART).
    [[maybe_unused]] const auto over_result =
        uart.set_oversampling(alloy::hal::uart::Oversampling::X8);
    // kernel_clock_hz: always compiles, returns config value.
    static_assert(noexcept(uart.kernel_clock_hz()));

    // ---- Phase 2: status flags ----
    [[maybe_unused]] const bool tc   = uart.tx_complete();
    [[maybe_unused]] const bool txe  = uart.tx_register_empty();
    [[maybe_unused]] const bool rxne = uart.rx_register_not_empty();
    [[maybe_unused]] const bool pe   = uart.parity_error();
    [[maybe_unused]] const bool fe   = uart.framing_error();
    [[maybe_unused]] const bool ne   = uart.noise_error();
    [[maybe_unused]] const bool ore  = uart.overrun_error();
    [[maybe_unused]] const auto clr_pe  = uart.clear_parity_error();
    [[maybe_unused]] const auto clr_fe  = uart.clear_framing_error();
    [[maybe_unused]] const auto clr_ne  = uart.clear_noise_error();
    [[maybe_unused]] const auto clr_ore = uart.clear_overrun_error();

    // ---- Phase 2: interrupts ----
    [[maybe_unused]] const auto en_tc  =
        uart.enable_interrupt(alloy::hal::uart::InterruptKind::Tc);
    [[maybe_unused]] const auto en_rxne =
        uart.enable_interrupt(alloy::hal::uart::InterruptKind::Rxne);
    [[maybe_unused]] const auto dis_txe =
        uart.disable_interrupt(alloy::hal::uart::InterruptKind::Txe);

    // ---- Phase 2: FIFO ----
    [[maybe_unused]] const auto fifo_en  = uart.enable_fifo(true);
    [[maybe_unused]] const auto tx_thr   =
        uart.set_tx_threshold(alloy::hal::uart::FifoTrigger::Half);
    [[maybe_unused]] const auto rx_thr   =
        uart.set_rx_threshold(alloy::hal::uart::FifoTrigger::Quarter);
    [[maybe_unused]] const bool txff     = uart.tx_fifo_full();
    [[maybe_unused]] const bool rxfe     = uart.rx_fifo_empty();

    // ---- Phase 3: mode setters ----
    [[maybe_unused]] const auto lin_en   = uart.enable_lin(true);
    [[maybe_unused]] const auto lin_brk  = uart.send_lin_break();
    [[maybe_unused]] const bool lbdf     = uart.lin_break_detected();
    [[maybe_unused]] const auto clr_lbdf = uart.clear_lin_break_flag();
    [[maybe_unused]] const auto hdsel    = uart.set_half_duplex(true);
    [[maybe_unused]] const auto de_en    = uart.enable_de(true);
    [[maybe_unused]] const auto deat     = uart.set_de_assertion_time(5u);
    [[maybe_unused]] const auto dedt     = uart.set_de_deassertion_time(3u);
    [[maybe_unused]] const auto sc       = uart.set_smartcard_mode(false);
    [[maybe_unused]] const auto irda     = uart.set_irda_mode(false);

    // ---- Phase 2: hardware flow control, DE polarity, error clearing ----
    [[maybe_unused]] const auto hfc      = uart.enable_hardware_flow_control(true);
    [[maybe_unused]] const auto dep      = uart.set_de_polarity(true);
    [[maybe_unused]] const auto errs     = uart.read_and_clear_errors();
    static_assert(noexcept(uart.read_and_clear_errors()) == false);
    [[maybe_unused]] const bool any_err  = errs.any();

    // ---- Phase 3 (uart-lite-full-surface): DMA enable ----
    // enable_dma_tx / enable_dma_rx: CR3 DMAT/DMAR bits.
    // Returns NotSupported when the DMA field is not published by the device DB.
    [[maybe_unused]] const auto dma_tx = uart.enable_dma_tx(true);
    [[maybe_unused]] const auto dma_rx = uart.enable_dma_rx(false);

    // ---- Phase 1: kernel clock source (task 1.4) ----
    // Returns NotSupported when kKernelClockSelectorField is invalid (all
    // current devices). Compiles on all backends regardless.
    [[maybe_unused]] const auto clk_src =
        uart.set_kernel_clock_source(alloy::device::KernelClockSource::pclk2);

    // ---- Phase 3: multiprocessor / wakeup (tasks 3.5–3.6) ----
    [[maybe_unused]] const auto set_addr =
        uart.set_address(0x42u, alloy::hal::uart::AddressLength::Bits7);
    [[maybe_unused]] const auto mute_en =
        uart.mute_until_address(true);
    [[maybe_unused]] const auto wakeup =
        uart.enable_wakeup_from_stop(alloy::hal::uart::WakeupTrigger::AddressMatch);
}

}  // namespace

static_assert(alloy::device::SelectedDeviceTraits::available);

#if defined(ALLOY_BOARD_NUCLEO_G071RB)
// NOTE: The device database for stm32g071rb currently only exposes USART1 (PB6/PB7).
// The physical Nucleo VCP uses USART2 (PA2/PA3); tracked in alloy-devices issue.
// This compile test uses USART1 to verify irq_numbers() API shape.
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PB6, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PB7, alloy::device::SignalId::signal_rx>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART1"});
static_assert(uart_is_usable<DebugUart>());
// STM32G0 USART1 shares a single combined NVIC line (IRQ 27).
static_assert(DebugUart::irq_numbers().size() == 1u);
static_assert(DebugUart::irq_numbers()[0] == 27u);
[[maybe_unused]] void compile_g071_uart_backend() {
    exercise_uart_backend<DebugUart>(64'000'000u);
}
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART2"});
static_assert(uart_is_usable<DebugUart>());
// STM32F4 USART2: IRQ lines not yet published in this device database revision.
// USART1 publishes {{42u, 67u}}; USART2 is tracked for a future alloy-devices update.
static_assert(DebugUart::irq_numbers().size() == 0u);
[[maybe_unused]] void compile_f401_uart_backend() {
    exercise_uart_backend<DebugUart>(42'000'000u);
}
#elif defined(ALLOY_BOARD_SAME70_XPLD)
// task 4.3: SAME70-targeted compile test for irq_numbers() — multi-IRQ surface check.
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PB4, alloy::device::SignalId::signal_txd1>,
    alloy::hal::connection::rx<alloy::device::PinId::PA21,
                               alloy::device::SignalId::signal_rxd1>>;
using DebugUart = decltype(alloy::hal::uart::open<DebugUartConnector>(
    {.baudrate = alloy::hal::Baudrate::e115200}));
static_assert(DebugUart::valid);
static_assert(DebugUart::peripheral_name == std::string_view{"USART1"});
static_assert(uart_is_usable<DebugUart>());
// SAME70 USART1 publishes one NVIC line (IRQ 14).
static_assert(DebugUart::irq_numbers().size() == 1u);
static_assert(DebugUart::irq_numbers()[0] == 14u);
[[maybe_unused]] void compile_same70_uart_backend() {
    exercise_uart_backend<DebugUart>(12'000'000u);
}
#endif
