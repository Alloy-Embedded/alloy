#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"
#include "host_mmio/framework/runtime_mmio.hpp"
#include "host_mmio/framework/stm32_gpio_uart_expect.hpp"

#include "boards/nucleo_f401re/board_uart.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/types.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

namespace alloy::test::mmio {
namespace {

namespace rt = alloy::hal::detail::runtime;
namespace gpio_detail = alloy::hal::gpio::detail;
namespace stm32_mmio = alloy::test::mmio::stm32;

using PeripheralId = alloy::device::runtime::PeripheralId;
using FieldId = alloy::device::runtime::FieldId;

constexpr auto kRccAhb1enrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_ahb1enr>();
constexpr auto kRccApb1enrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_apb1enr>();
constexpr auto kRccApb1rstrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_rcc_apb1rstr>();
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
constexpr auto kUsart2SrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_sr>();
constexpr auto kUsart2DrAddress =
    stm32_mmio::register_address<alloy::device::runtime::RegisterId::register_usart2_dr>();

struct stm32f4_led_pin_handle {
    static constexpr bool valid = true;
    static constexpr auto schema = rt::GpioSchema::st_gpio;
    static constexpr auto peripheral_id = PeripheralId::GPIOA;
    [[maybe_unused]] static constexpr auto mode_field =
        rt::field_ref<FieldId::field_gpioa_moder_moder5>();
    [[maybe_unused]] static constexpr auto output_type_field =
        rt::field_ref<FieldId::field_gpioa_otyper_ot5>();
    [[maybe_unused]] static constexpr auto pull_field =
        rt::field_ref<FieldId::field_gpioa_pupdr_pupdr5>();
    [[maybe_unused]] static constexpr auto input_field =
        rt::field_ref<FieldId::field_gpioa_idr_idr5>();
    [[maybe_unused]] static constexpr auto output_set_field =
        rt::field_ref<FieldId::field_gpioa_bsrr_bs5>();
    [[maybe_unused]] static constexpr auto output_reset_field =
        rt::field_ref<FieldId::field_gpioa_bsrr_br5>();
};

}  // namespace

TEST_CASE("host mmio covers descriptor-driven STM32F4 gpio and uart bring-up",
          "[host-mmio][bring-up][stm32f4]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    mmio.preload(kRccApb1rstrAddress,
                 rt::field_mask(rt::field_ref<FieldId::field_rcc_apb1rstr_usart2rst>()));
    mmio.preload(kUsart2SrAddress,
                 rt::field_bits(rt::field_ref<FieldId::field_usart2_sr_txe>(), 1u).unwrap());

    const auto gpio_config = alloy::hal::GpioConfig{
        .direction = alloy::hal::PinDirection::Output,
        .drive = alloy::hal::PinDrive::PushPull,
        .pull = alloy::hal::PinPull::None,
        .initial_state = alloy::hal::PinState::High,
    };
    const auto gpio_result = gpio_detail::configure_gpio<stm32f4_led_pin_handle>(gpio_config);
    REQUIRE(gpio_result.is_ok());

    const auto gpio_write_result =
        gpio_detail::write_gpio<stm32f4_led_pin_handle>(gpio_config, alloy::hal::PinState::Low);
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
            .gpio_enable_address = kRccAhb1enrAddress,
            .gpio_enable_value = rt::field_mask(rt::field_ref<FieldId::field_rcc_ahb1enr_gpioaen>()),
            .gpio_reset_address = kRccApb1rstrAddress,
            .gpio_reset_mask = 0u,
            .uart_enable_address = kRccApb1enrAddress,
            .uart_enable_value = rt::field_mask(
                rt::field_ref<FieldId::field_rcc_apb1enr_usart2en>()),
            .uart_reset_address = kRccApb1rstrAddress,
            .uart_reset_mask = rt::field_mask(rt::field_ref<FieldId::field_rcc_apb1rstr_usart2rst>()),
            .gpio_moder_address = kGpioaModerAddress,
            .gpio_moder_value = 0x0000'04A0u,
            .gpio_otyper_address = kGpioaOtyperAddress,
            .gpio_otyper_value = 0u,
            .gpio_pupdr_address = kGpioaPupdrAddress,
            .gpio_pupdr_value = 0u,
            .gpio_afrl_address = kGpioaAfrlAddress,
            .gpio_afrl_value = 0x0000'7700u,
            .gpio_bsrr_address = kGpioaBsrrAddress,
            .gpio_set_value = 1u << 5u,
            .gpio_reset_value = 1u << (5u + 16u),
            .uart_cr1_address = kUsart2Cr1Address,
            .uart_cr1_initial_value = 0x0000'000Cu,
            .uart_cr1_enabled_value = 0x0000'200Cu,
            .uart_brr_address = kUsart2BrrAddress,
            .uart_brr_value = 365u,
            .uart_data_address = kUsart2DrAddress,
            .uart_data_value = 0x41u,
        });
}

}  // namespace alloy::test::mmio
