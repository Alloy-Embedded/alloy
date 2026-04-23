#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"
#include "host_mmio/framework/runtime_mmio.hpp"
#include "host_mmio/framework/stm32_gpio_uart_expect.hpp"

#include "boards/nucleo_g071rb/board_uart.hpp"
#include "device/interrupt_stubs.hpp"
#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/dma.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/types.hpp"
#include "hal/uart.hpp"
#include "async.hpp"
#include "runtime/dma_event.hpp"
#include "runtime/interrupt_event.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace alloy::test::mmio {
namespace {

namespace rt = alloy::hal::detail::runtime;
namespace gpio_detail = alloy::hal::gpio::detail;
namespace stm32_mmio = alloy::test::mmio::stm32;

using PeripheralId = alloy::device::runtime::PeripheralId;
using FieldId = alloy::device::runtime::FieldId;

constexpr auto kRccIopenrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_iopenr>();
constexpr auto kRccIoprstrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_ioprstr>();
constexpr auto kRccApbenr1Address =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_apbenr1>();
constexpr auto kRccApbrstr1Address =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_apbrstr1>();
constexpr auto kGpioaModerAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_gpioa_moder>();
constexpr auto kGpioaOtyperAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_gpioa_otyper>();
constexpr auto kGpioaPupdrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_gpioa_pupdr>();
constexpr auto kGpioaBsrrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_gpioa_bsrr>();
constexpr auto kGpioaAfrlAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_gpioa_afrl>();
constexpr auto kUsart2Cr1Address =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_cr1>();
constexpr auto kUsart2BrrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_brr>();
constexpr auto kUsart2IsrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_isr>();
constexpr auto kUsart2TdrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_tdr>();
constexpr auto kUsart1Cr3Address =
    stm32_mmio::register_address(rt::find_runtime_register_ref_by_suffix(PeripheralId::USART1, "cr3"));
constexpr auto kDma1IsrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_dma1_isr>();
constexpr auto kDma1IfcrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_dma1_ifcr>();
constexpr auto kDma1BaseAddress =
    alloy::device::PeripheralInstanceTraits<PeripheralId::DMA1>::kBaseAddress;
constexpr auto kDma1Ccr1Address = kDma1BaseAddress + 0x08u;
constexpr auto kDma1Cndtr1Address = kDma1BaseAddress + 0x0Cu;
constexpr auto kDma1Cpar1Address = kDma1BaseAddress + 0x10u;
constexpr auto kDma1Cmar1Address = kDma1BaseAddress + 0x14u;
constexpr auto kDma1Ccr2Address = kDma1BaseAddress + 0x1Cu;
constexpr auto kDma1Cndtr2Address = kDma1BaseAddress + 0x20u;
constexpr auto kDma1Cpar2Address = kDma1BaseAddress + 0x24u;
constexpr auto kDma1Cmar2Address = kDma1BaseAddress + 0x28u;

using Uart1Connector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PA9, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA10, alloy::device::SignalId::signal_rx>>;

struct stm32g0_led_pin_handle {
    static constexpr bool valid = true;
    static constexpr auto schema = rt::GpioSchema::st_gpio;
    static constexpr auto peripheral_id = PeripheralId::GPIOA;
    [[maybe_unused]] static constexpr auto mode_field = rt::field_ref<FieldId::field_gpioa_moder_moder5>();
    [[maybe_unused]] static constexpr auto output_type_field = rt::field_ref<FieldId::field_gpioa_otyper_ot5>();
    [[maybe_unused]] static constexpr auto pull_field = rt::field_ref<FieldId::field_gpioa_pupdr_pupdr5>();
    [[maybe_unused]] static constexpr auto input_field = rt::field_ref<FieldId::field_gpioa_idr_idr5>();
    [[maybe_unused]] static constexpr auto output_set_field = rt::field_ref<FieldId::field_gpioa_bsrr_bs5>();
    [[maybe_unused]] static constexpr auto output_reset_field = rt::field_ref<FieldId::field_gpioa_bsrr_br5>();
};

}  // namespace

TEST_CASE("host mmio covers descriptor-driven STM32G0 gpio and uart bring-up",
          "[host-mmio][bring-up][stm32g0]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    mmio.preload(kRccIoprstrAddress, rt::field_mask(rt::field_ref<FieldId::field_rcc_ioprstr_gpioarst>()));
    mmio.preload(kRccApbrstr1Address,
                 rt::field_mask(rt::field_ref<FieldId::field_rcc_apbrstr1_usart2rst>()));
    mmio.preload(kUsart2IsrAddress, rt::field_bits(rt::field_ref<FieldId::field_usart2_isr_txe>(), 1u).unwrap());

    const auto gpio_config = alloy::hal::GpioConfig{
        .direction = alloy::hal::PinDirection::Output,
        .drive = alloy::hal::PinDrive::PushPull,
        .pull = alloy::hal::PinPull::None,
        .initial_state = alloy::hal::PinState::High,
    };
    const auto gpio_result = gpio_detail::configure_gpio<stm32g0_led_pin_handle>(gpio_config);
    REQUIRE(gpio_result.is_ok());

    const auto gpio_write_result =
        gpio_detail::write_gpio<stm32g0_led_pin_handle>(gpio_config, alloy::hal::PinState::Low);
    REQUIRE(gpio_write_result.is_ok());

    const auto clock_result = rt::enable_peripheral_runtime_typed<PeripheralId::USART2>();
    REQUIRE(clock_result.is_ok());

    auto uart_handle = board::make_debug_uart();
    const auto uart_result = uart_handle.configure();
    REQUIRE(uart_result.is_ok());

    const auto tx_result = uart_handle.write_byte(std::byte{0x41});
    REQUIRE(tx_result.is_ok());

    stm32_mmio::require_gpio_uart_bringup(
        mmio, trace,
        {
            .gpio_enable_address = kRccIopenrAddress,
            .gpio_enable_value = rt::field_mask(rt::field_ref<FieldId::field_rcc_iopenr_gpioaen>()),
            .gpio_reset_address = kRccIoprstrAddress,
            .gpio_reset_mask = rt::field_mask(rt::field_ref<FieldId::field_rcc_ioprstr_gpioarst>()),
            .uart_enable_address = kRccApbenr1Address,
            .uart_enable_value = rt::field_mask(
                rt::field_ref<FieldId::field_rcc_apbenr1_usart2en>()),
            .uart_reset_address = kRccApbrstr1Address,
            .uart_reset_mask = rt::field_mask(
                rt::field_ref<FieldId::field_rcc_apbrstr1_usart2rst>()),
            .gpio_moder_address = kGpioaModerAddress,
            .gpio_moder_value = 0x0000'04A0u,
            .gpio_otyper_address = kGpioaOtyperAddress,
            .gpio_otyper_value = 0u,
            .gpio_pupdr_address = kGpioaPupdrAddress,
            .gpio_pupdr_value = 0u,
            .gpio_afrl_address = kGpioaAfrlAddress,
            .gpio_afrl_value = 0x0000'1100u,
            .gpio_bsrr_address = kGpioaBsrrAddress,
            .gpio_set_value = 1u << 5u,
            .gpio_reset_value = 1u << (5u + 16u),
            .uart_cr1_address = kUsart2Cr1Address,
            .uart_cr1_initial_value = 0x0000'000Cu,
            .uart_cr1_enabled_value = 0x0000'000Du,
            .uart_brr_address = kUsart2BrrAddress,
            .uart_brr_value = 139u,
            .uart_data_address = kUsart2TdrAddress,
            .uart_data_value = 0x41u,
        });
}

TEST_CASE("host mmio bridges STM32G0 DMA channel 1 IRQ into runtime events",
          "[host-mmio][bring-up][stm32g0][event]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using InterruptId = alloy::device::interrupt_stubs::InterruptId;
    using DmaRxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_RX>;
    using DmaCh1Interrupt = alloy::runtime::interrupt_event::token<InterruptId::DMA_Channel1>;

    DmaRxCompletion::reset();
    DmaCh1Interrupt::reset();

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif1>()));

    DMA_Channel1_IRQHandler();

    REQUIRE(DmaCh1Interrupt::ready());
    REQUIRE(DmaRxCompletion::ready());
    REQUIRE(mmio.peek(kDma1IfcrAddress) ==
            rt::field_mask(rt::field_ref<FieldId::field_dma1_ifcr_ctcif1>()));
}

TEST_CASE("host mmio bridges STM32G0 DMA channel 2 IRQ into UART TX completion",
          "[host-mmio][bring-up][stm32g0][event]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using InterruptId = alloy::device::interrupt_stubs::InterruptId;
    using DmaTxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_TX>;
    using DmaCh23Interrupt =
        alloy::runtime::interrupt_event::token<InterruptId::DMA_Channel2_3>;

    DmaTxCompletion::reset();
    DmaCh23Interrupt::reset();

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif2>()));

    DMA_Channel2_3_IRQHandler();

    REQUIRE(DmaCh23Interrupt::ready());
    REQUIRE(DmaTxCompletion::ready());
    REQUIRE(mmio.peek(kDma1IfcrAddress) ==
            rt::field_mask(rt::field_ref<FieldId::field_dma1_ifcr_ctcif5>()));
}

TEST_CASE("host mmio starts STM32G0 USART1 TX DMA transfers through the UART API",
          "[host-mmio][bring-up][stm32g0][dma]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using DmaTxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_TX>;

    auto uart = alloy::hal::uart::open<Uart1Connector>({
        .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
    });
    auto tx_dma = alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART1,
                                        alloy::hal::dma::SignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
    });
    auto payload = std::to_array<std::byte>({std::byte{0x41}, std::byte{0x42}, std::byte{0x43}});

    DmaTxCompletion::signal();

    const auto uart_result = uart.configure();
    REQUIRE(uart_result.is_ok());

    const auto tx_result = uart.write_dma(tx_dma, payload);
    REQUIRE(tx_result.is_ok());

    REQUIRE_FALSE(DmaTxCompletion::ready());
    REQUIRE(mmio.peek(kUsart1Cr3Address) == (1u << 7u));
    REQUIRE(mmio.peek(kDma1Cpar2Address) ==
            static_cast<std::uint32_t>(decltype(uart)::tx_data_register_address()));
    REQUIRE(mmio.peek(kDma1Cmar2Address) ==
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(payload.data())));
    REQUIRE(mmio.peek(kDma1Cndtr2Address) == payload.size());
    REQUIRE(mmio.peek(kDma1Ccr2Address) == 0x0000'1093u);

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif2>()));
    DMA_Channel2_3_IRQHandler();

    REQUIRE(DmaTxCompletion::ready());
}

TEST_CASE("host mmio exposes STM32G0 UART TX DMA as a polling async operation",
          "[host-mmio][bring-up][stm32g0][async]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using DmaTxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_TX>;
    using PollStatus = alloy::async::poll_status;

    auto uart = alloy::hal::uart::open<Uart1Connector>({
        .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
    });
    auto tx_dma = alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART1,
                                        alloy::hal::dma::SignalId::signal_TX>({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
    });
    auto payload = std::to_array<std::byte>({std::byte{0x31}, std::byte{0x32}});

    REQUIRE(uart.configure().is_ok());

    const auto operation = alloy::async::uart::write_dma(uart, tx_dma, payload);
    REQUIRE(operation.is_ok());
    REQUIRE(operation.unwrap().poll() == PollStatus::pending);

    bool callback_called = false;
    REQUIRE_FALSE(operation.unwrap().notify_if_ready([&callback_called] { callback_called = true; }));
    REQUIRE_FALSE(callback_called);

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif2>()));
    DMA_Channel2_3_IRQHandler();

    REQUIRE(operation.unwrap().poll() == PollStatus::ready);
    REQUIRE(operation.unwrap().notify_if_ready([&callback_called] { callback_called = true; }));
    REQUIRE(callback_called);
    REQUIRE(DmaTxCompletion::ready());
}

TEST_CASE("host mmio starts STM32G0 USART1 RX DMA transfers through the UART API",
          "[host-mmio][bring-up][stm32g0][dma]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using DmaRxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_RX>;

    auto uart = alloy::hal::uart::open<Uart1Connector>({
        .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
    });
    auto rx_dma = alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART1,
                                        alloy::hal::dma::SignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
    });
    auto buffer = std::to_array<std::byte>({std::byte{0x00}, std::byte{0x00}, std::byte{0x00}});

    DmaRxCompletion::signal();

    REQUIRE(uart.configure().is_ok());
    REQUIRE(uart.read_dma(rx_dma, buffer).is_ok());

    REQUIRE_FALSE(DmaRxCompletion::ready());
    REQUIRE(mmio.peek(kUsart1Cr3Address) == (1u << 6u));
    REQUIRE(mmio.peek(kDma1Cpar1Address) ==
            static_cast<std::uint32_t>(decltype(uart)::rx_data_register_address()));
    REQUIRE(mmio.peek(kDma1Cmar1Address) ==
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer.data())));
    REQUIRE(mmio.peek(kDma1Cndtr1Address) == buffer.size());
    REQUIRE(mmio.peek(kDma1Ccr1Address) == 0x0000'1083u);

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif1>()));
    DMA_Channel1_IRQHandler();

    REQUIRE(DmaRxCompletion::ready());
}

TEST_CASE("host mmio exposes STM32G0 UART RX DMA as a polling async operation",
          "[host-mmio][bring-up][stm32g0][async]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using DmaRxCompletion = alloy::runtime::dma_event::token<
        alloy::hal::dma::PeripheralId::USART1,
        alloy::hal::dma::SignalId::signal_RX>;
    using PollStatus = alloy::async::poll_status;

    auto uart = alloy::hal::uart::open<Uart1Connector>({
        .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
    });
    auto rx_dma = alloy::hal::dma::open<alloy::hal::dma::PeripheralId::USART1,
                                        alloy::hal::dma::SignalId::signal_RX>({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
    });
    auto buffer = std::to_array<std::byte>({std::byte{0x00}, std::byte{0x00}});

    REQUIRE(uart.configure().is_ok());

    const auto operation = alloy::async::uart::read_dma(uart, rx_dma, buffer);
    REQUIRE(operation.is_ok());
    REQUIRE(operation.unwrap().poll() == PollStatus::pending);

    bool callback_called = false;
    REQUIRE_FALSE(operation.unwrap().notify_if_ready([&callback_called] { callback_called = true; }));
    REQUIRE_FALSE(callback_called);

    mmio.preload(kDma1IsrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_dma1_isr_tcif1>()));
    DMA_Channel1_IRQHandler();

    REQUIRE(operation.unwrap().poll() == PollStatus::ready);
    REQUIRE(operation.unwrap().notify_if_ready([&callback_called] { callback_called = true; }));
    REQUIRE(callback_called);
    REQUIRE(DmaRxCompletion::ready());
}

TEST_CASE("host mmio bridges STM32G0 USART1 IRQ into runtime interrupt token",
          "[host-mmio][bring-up][stm32g0][event]") {
    using InterruptId = alloy::device::interrupt_stubs::InterruptId;
    using Usart1Interrupt = alloy::runtime::interrupt_event::token<InterruptId::USART1>;

    Usart1Interrupt::reset();

    USART1_IRQHandler();

    REQUIRE(Usart1Interrupt::ready());
}

}  // namespace alloy::test::mmio
