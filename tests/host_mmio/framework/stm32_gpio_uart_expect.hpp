#pragma once

#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"

#include "device/runtime.hpp"
#include "hal/detail/runtime_lite_ops.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

namespace alloy::test::mmio::stm32 {

namespace rt = alloy::hal::detail::runtime_lite;

using RegisterId = alloy::device::runtime::RegisterId;

[[nodiscard]] constexpr auto register_address(const rt::RegisterRef& reg) -> std::uintptr_t {
    return reg.base_address + reg.offset_bytes;
}

template <RegisterId Id>
[[nodiscard]] consteval auto register_address() -> std::uintptr_t {
    constexpr auto reg = rt::register_ref<Id>();
    return register_address(reg);
}

[[nodiscard]] inline auto first_write_index(const trace_log& trace, std::uintptr_t address,
                                            std::uint32_t value) -> std::size_t {
    for (auto index = std::size_t{0}; index < trace.entries().size(); ++index) {
        const auto& entry = trace.entries()[index];
        if (entry.kind == access_kind::write && entry.address == address && entry.value == value) {
            return index;
        }
    }
    return trace.entries().size();
}

struct GpioUartBringupExpectations {
    std::uintptr_t gpio_enable_address{};
    std::uint32_t gpio_enable_value{};
    std::uintptr_t gpio_reset_address{};
    std::uint32_t gpio_reset_mask{};

    std::uintptr_t uart_enable_address{};
    std::uint32_t uart_enable_value{};
    std::uintptr_t uart_reset_address{};
    std::uint32_t uart_reset_mask{};

    std::uintptr_t gpio_moder_address{};
    std::uint32_t gpio_moder_value{};
    std::uintptr_t gpio_otyper_address{};
    std::uint32_t gpio_otyper_value{};
    std::uintptr_t gpio_pupdr_address{};
    std::uint32_t gpio_pupdr_value{};
    std::uintptr_t gpio_afrl_address{};
    std::uint32_t gpio_afrl_value{};
    std::uintptr_t gpio_bsrr_address{};
    std::uint32_t gpio_set_value{};
    std::uint32_t gpio_reset_value{};

    std::uintptr_t uart_cr1_address{};
    std::uint32_t uart_cr1_initial_value{};
    std::uint32_t uart_cr1_enabled_value{};
    std::uintptr_t uart_brr_address{};
    std::uint32_t uart_brr_value{};
    std::uintptr_t uart_data_address{};
    std::uint32_t uart_data_value{};
};

inline void require_gpio_uart_bringup(const mmio_space& mmio, const trace_log& trace,
                                      const GpioUartBringupExpectations& expectations) {
    REQUIRE(mmio.peek(expectations.gpio_enable_address) == expectations.gpio_enable_value);
    if (expectations.gpio_reset_mask != 0u) {
        REQUIRE((mmio.peek(expectations.gpio_reset_address) & expectations.gpio_reset_mask) == 0u);
    }
    REQUIRE(mmio.peek(expectations.uart_enable_address) == expectations.uart_enable_value);
    if (expectations.uart_reset_mask != 0u) {
        REQUIRE((mmio.peek(expectations.uart_reset_address) & expectations.uart_reset_mask) == 0u);
    }
    REQUIRE(mmio.peek(expectations.gpio_moder_address) == expectations.gpio_moder_value);
    REQUIRE(mmio.peek(expectations.gpio_otyper_address) == expectations.gpio_otyper_value);
    REQUIRE(mmio.peek(expectations.gpio_pupdr_address) == expectations.gpio_pupdr_value);
    REQUIRE(mmio.peek(expectations.gpio_afrl_address) == expectations.gpio_afrl_value);
    REQUIRE(mmio.peek(expectations.gpio_bsrr_address) == expectations.gpio_reset_value);
    REQUIRE(mmio.peek(expectations.uart_cr1_address) == expectations.uart_cr1_enabled_value);
    REQUIRE(mmio.peek(expectations.uart_brr_address) == expectations.uart_brr_value);
    REQUIRE(mmio.peek(expectations.uart_data_address) == expectations.uart_data_value);

    const auto gpio_set_index =
        first_write_index(trace, expectations.gpio_bsrr_address, expectations.gpio_set_value);
    const auto gpio_reset_index =
        first_write_index(trace, expectations.gpio_bsrr_address, expectations.gpio_reset_value);
    const auto uart_enable_gate_index =
        first_write_index(trace, expectations.uart_enable_address, expectations.uart_enable_value);
    const auto uart_mode_route_index =
        first_write_index(trace, expectations.gpio_moder_address, expectations.gpio_moder_value);
    const auto uart_af_route_index =
        first_write_index(trace, expectations.gpio_afrl_address, expectations.gpio_afrl_value);
    const auto uart_cr1_initial_index = first_write_index(trace, expectations.uart_cr1_address,
                                                          expectations.uart_cr1_initial_value);
    const auto uart_cr1_enable_index = first_write_index(trace, expectations.uart_cr1_address,
                                                         expectations.uart_cr1_enabled_value);
    const auto uart_data_index =
        first_write_index(trace, expectations.uart_data_address, expectations.uart_data_value);

    INFO(describe_trace(trace));
    REQUIRE(gpio_set_index < trace.entries().size());
    REQUIRE(gpio_reset_index < trace.entries().size());
    REQUIRE(gpio_set_index < gpio_reset_index);
    REQUIRE(uart_enable_gate_index < trace.entries().size());
    REQUIRE(uart_mode_route_index < trace.entries().size());
    REQUIRE(uart_af_route_index < trace.entries().size());
    REQUIRE(uart_cr1_initial_index < trace.entries().size());
    REQUIRE(uart_cr1_enable_index < trace.entries().size());
    REQUIRE(uart_enable_gate_index < uart_cr1_initial_index);
    REQUIRE(uart_mode_route_index < uart_cr1_initial_index);
    REQUIRE(uart_af_route_index < uart_cr1_initial_index);
    REQUIRE(uart_cr1_initial_index < uart_cr1_enable_index);
    REQUIRE(uart_data_index < trace.entries().size());
    REQUIRE(uart_cr1_enable_index < uart_data_index);
}

}  // namespace alloy::test::mmio::stm32
