#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"
#include "host_mmio/framework/runtime_mmio.hpp"

#include "hal/detail/resolved_route.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/uart/detail/backend.hpp"
#include "hal/types.hpp"

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>

namespace alloy::test::mmio {
namespace {

namespace rt = alloy::hal::detail::runtime;
namespace route = alloy::hal::detail::route;
namespace gpio_detail = alloy::hal::gpio::detail;
namespace uart_detail = alloy::hal::uart::detail;

constexpr auto kPmcBase = std::uintptr_t{0x400e0600u};
constexpr auto kPmcPcer0 = std::uintptr_t{0x400e0610u};
constexpr auto kWdtBase = std::uintptr_t{0x400e1850u};
constexpr auto kRswdtBase = std::uintptr_t{0x400e1900u};
constexpr auto kResetControllerBase = std::uintptr_t{0x400e1800u};
constexpr auto kPioBBase = std::uintptr_t{0x400e1000u};
constexpr auto kPioCBase = std::uintptr_t{0x400e1200u};
constexpr auto kUsart0Base = std::uintptr_t{0x40024000u};
constexpr auto kLedLine = std::uint16_t{8u};
constexpr auto kTxLine = std::uint16_t{1u};
constexpr auto kRxLine = std::uint16_t{0u};
constexpr auto kSame70PioSelectorB = std::uint32_t{2u};
constexpr auto kWatchdogGuardValue = std::uint32_t{0x0fffu};

[[nodiscard]] constexpr auto register_ref(std::uintptr_t base_address, std::uint32_t offset_bytes)
    -> rt::RegisterRef {
    return {
        .base_address = base_address,
        .offset_bytes = offset_bytes,
        .valid = true,
    };
}

[[nodiscard]] constexpr auto field_ref(std::uintptr_t base_address, std::uint32_t register_offset_bytes,
                                       std::uint16_t bit_offset,
                                       std::uint16_t bit_width = 1u) -> rt::FieldRef {
    return {
        .reg = register_ref(base_address, register_offset_bytes),
        .bit_offset = bit_offset,
        .bit_width = bit_width,
        .valid = true,
    };
}

template <std::uint16_t Line>
struct same70_pio_pin_handle {
    static constexpr bool valid = true;
    static constexpr auto schema = rt::GpioSchema::microchip_pio_v;
    static constexpr auto peripheral_id = alloy::device::runtime::PeripheralId::none;
    static constexpr auto pio_enable_field = field_ref(kPioCBase, 0x00u, Line);
    static constexpr auto pio_drive_enable_field = field_ref(kPioCBase, 0x50u, Line);
    static constexpr auto pio_drive_disable_field = field_ref(kPioCBase, 0x54u, Line);
    static constexpr auto pio_pull_up_enable_field = field_ref(kPioCBase, 0x64u, Line);
    static constexpr auto pio_pull_up_disable_field = field_ref(kPioCBase, 0x60u, Line);
    static constexpr auto pio_pull_down_enable_field = field_ref(kPioCBase, 0x94u, Line);
    static constexpr auto pio_pull_down_disable_field = field_ref(kPioCBase, 0x90u, Line);
    static constexpr auto pio_set_field = field_ref(kPioCBase, 0x30u, Line);
    static constexpr auto pio_clear_field = field_ref(kPioCBase, 0x34u, Line);
    static constexpr auto pio_output_enable_field = field_ref(kPioCBase, 0x10u, Line);
    static constexpr auto pio_output_disable_field = field_ref(kPioCBase, 0x14u, Line);
    static constexpr auto pio_input_state_field = field_ref(kPioCBase, 0x3cu, Line);
};

struct fake_tx_signal {
    static constexpr auto name = std::string_view{"tx"};
};

struct fake_rx_signal {
    static constexpr auto name = std::string_view{"rx"};
};

template <typename Signal>
struct fake_uart_binding {
    using signal_type = Signal;
};

struct same70_debug_uart_connector {
    [[maybe_unused]] static constexpr bool valid = true;
    static constexpr auto binding_count = std::size_t{2u};
    template <std::size_t Index>
    using binding_type = std::tuple_element_t<
        Index, std::tuple<fake_uart_binding<fake_tx_signal>, fake_uart_binding<fake_rx_signal>>>;
};

class same70_debug_uart_handle {
  public:
    using connector_type = same70_debug_uart_connector;

    static constexpr bool valid = true;
    static constexpr bool is_st_style = false;
    static constexpr bool is_st_legacy_style = false;
    static constexpr bool is_st_modern_style = false;
    static constexpr bool is_microchip_uart_r = false;
    static constexpr bool is_microchip_usart_zw = true;

    static constexpr auto us_cr_reg = register_ref(kUsart0Base, 0x00u);
    static constexpr auto us_mr_reg = register_ref(kUsart0Base, 0x04u);
    static constexpr auto us_brgr_reg = register_ref(kUsart0Base, 0x20u);
    [[maybe_unused]] static constexpr auto us_csr_reg = register_ref(kUsart0Base, 0x14u);
    static constexpr auto us_thr_reg = register_ref(kUsart0Base, 0x1cu);
    static constexpr auto us_rstrx_field = field_ref(kUsart0Base, 0x00u, 2u);
    static constexpr auto us_rsttx_field = field_ref(kUsart0Base, 0x00u, 3u);
    static constexpr auto us_rxdis_field = field_ref(kUsart0Base, 0x00u, 5u);
    static constexpr auto us_txdis_field = field_ref(kUsart0Base, 0x00u, 7u);
    static constexpr auto us_rststa_field = field_ref(kUsart0Base, 0x00u, 8u);
    static constexpr auto us_usart_mode_field = field_ref(kUsart0Base, 0x04u, 0u, 4u);
    static constexpr auto us_usclks_field = field_ref(kUsart0Base, 0x04u, 4u, 2u);
    static constexpr auto us_chrl_field = field_ref(kUsart0Base, 0x04u, 6u, 2u);
    static constexpr auto us_cd_field = field_ref(kUsart0Base, 0x20u, 0u, 16u);
    static constexpr auto us_rxen_field = field_ref(kUsart0Base, 0x00u, 4u);
    static constexpr auto us_txen_field = field_ref(kUsart0Base, 0x00u, 6u);
    static constexpr auto us_txrdy_field = field_ref(kUsart0Base, 0x14u, 1u);
    static constexpr auto us_txchr_field = field_ref(kUsart0Base, 0x1cu, 0u, 9u);

    explicit same70_debug_uart_handle(alloy::hal::UartConfig config) : config_{config} {}

    [[nodiscard]] auto config() const -> const alloy::hal::UartConfig& { return config_; }

    [[nodiscard]] static constexpr auto operations() {
        return route::List<route::Operation, 0u>{};
    }

  private:
    alloy::hal::UartConfig config_{};
};

void disable_same70_watchdog(const rt::RegisterRef& reg, const rt::FieldRef& disable_field,
                             const rt::FieldRef& guard_field) {
    const auto disable_bits = rt::field_bits(disable_field, 1u).unwrap();
    const auto guard_bits = rt::field_bits(guard_field, kWatchdogGuardValue).unwrap();
    rt::write_register(reg, disable_bits | guard_bits).unwrap();
}

void enable_same70_peripheral_clock(std::uint32_t peripheral_id) {
    rt::write_register(register_ref(kPmcBase, 0x10u), std::uint32_t{1u} << peripheral_id).unwrap();
}

void release_runtime_reset_bit(std::uint32_t bit_index) {
    rt::modify_register_bit(register_ref(kResetControllerBase, 0x00u), bit_index, false).unwrap();
}

void configure_same70_peripheral_mux(std::uintptr_t pio_base, std::uint16_t line,
                                     std::uint32_t selector) {
    const auto pdr_field = field_ref(pio_base, 0x04u, line);
    const auto abcdsr_field = field_ref(pio_base, 0x70u, line);
    const auto pdr_bits = rt::field_bits(pdr_field, 1u).unwrap();
    rt::write_register(pdr_field.reg, pdr_bits).unwrap();

    const auto mask = rt::field_mask(abcdsr_field);
    const auto abcdsr0_address = pio_base + 0x70u;
    const auto abcdsr1_address = pio_base + 0x74u;
    auto abcdsr0 = rt::read_mmio32(abcdsr0_address);
    auto abcdsr1 = rt::read_mmio32(abcdsr1_address);
    abcdsr0 = (selector & 0x1u) != 0u ? (abcdsr0 | mask) : (abcdsr0 & ~mask);
    abcdsr1 = (selector & 0x2u) != 0u ? (abcdsr1 | mask) : (abcdsr1 & ~mask);
    rt::write_mmio32(abcdsr0_address, abcdsr0);
    rt::write_mmio32(abcdsr1_address, abcdsr1);
}

}  // namespace

TEST_CASE("host mmio migrates foundational SAME70 bring-up into recorded runtime flows",
          "[host-mmio][bring-up][same70]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    mmio.preload(kResetControllerBase, 0xffff'ffffu);

    disable_same70_watchdog(register_ref(kWdtBase, 0x04u), field_ref(kWdtBase, 0x04u, 15u),
                            field_ref(kWdtBase, 0x04u, 16u, 12u));
    disable_same70_watchdog(register_ref(kRswdtBase, 0x04u), field_ref(kRswdtBase, 0x04u, 15u),
                            field_ref(kRswdtBase, 0x04u, 16u, 12u));
    enable_same70_peripheral_clock(12u);
    enable_same70_peripheral_clock(11u);
    enable_same70_peripheral_clock(13u);
    release_runtime_reset_bit(7u);

    REQUIRE(mmio.peek(kWdtBase + 0x04u) == 0x0fff8000u);
    REQUIRE(mmio.peek(kRswdtBase + 0x04u) == 0x0fff8000u);
    REQUIRE(mmio.peek(kResetControllerBase) == 0xffff'ff7fu);

    const auto expected = std::array{
        access{.kind = access_kind::write, .address = kWdtBase + 0x04u, .value = 0x0fff8000u, .mask = 0u},
        access{.kind = access_kind::write, .address = kRswdtBase + 0x04u, .value = 0x0fff8000u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPmcPcer0, .value = 1u << 12u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPmcPcer0, .value = 1u << 11u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPmcPcer0, .value = 1u << 13u, .mask = 0u},
        access{.kind = access_kind::read, .address = kResetControllerBase, .value = 0xffff'ffffu, .mask = 0u},
        access{.kind = access_kind::write, .address = kResetControllerBase, .value = 0xffff'ff7fu, .mask = 0u},
    };

    REQUIRE(trace.entries().size() == expected.size());
    for (auto index = std::size_t{0}; index < expected.size(); ++index) {
        INFO("trace entry " << index << '\n' << describe_trace(trace));
        REQUIRE(trace.entries()[index] == expected[index]);
    }
}

TEST_CASE("host mmio covers SAME70-style gpio and uart initialization with production backends",
          "[host-mmio][bring-up][same70]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    configure_same70_peripheral_mux(kPioBBase, kTxLine, kSame70PioSelectorB);
    configure_same70_peripheral_mux(kPioBBase, kRxLine, kSame70PioSelectorB);

    const auto gpio_result = gpio_detail::configure_microchip_pio<same70_pio_pin_handle<kLedLine>>({
        .direction = alloy::hal::PinDirection::Output,
        .drive = alloy::hal::PinDrive::PushPull,
        .pull = alloy::hal::PinPull::None,
        .initial_state = alloy::hal::PinState::Low,
    });
    REQUIRE(gpio_result.is_ok());

    same70_debug_uart_handle uart_handle{
        alloy::hal::UartConfig{
            .baudrate = alloy::hal::Baudrate::e115200,
            .data_bits = alloy::hal::DataBits::Eight,
            .stop_bits = alloy::hal::StopBits::One,
            .parity = alloy::hal::Parity::None,
            .flow_control = alloy::hal::FlowControl::None,
            .peripheral_clock_hz = 12'000'000u,
        },
    };
    const auto uart_result = uart_detail::configure_uart(uart_handle);
    REQUIRE(uart_result.is_ok());

    mmio.preload(kUsart0Base + 0x14u, 0x0000'0002u);
    const auto tx_result = uart_detail::write_uart_byte(uart_handle, std::byte{0x41});
    REQUIRE(tx_result.is_ok());

    REQUIRE(mmio.peek(kPioBBase + 0x04u) == 0x0000'0001u);
    REQUIRE(mmio.peek(kPioBBase + 0x70u) == 0x0000'0000u);
    REQUIRE(mmio.peek(kPioBBase + 0x74u) == 0x0000'0003u);
    REQUIRE(mmio.peek(kPioCBase + 0x00u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x54u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x60u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x90u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x34u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x10u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kUsart0Base + 0x00u) == 0x0000'0050u);
    REQUIRE(mmio.peek(kUsart0Base + 0x04u) == 0x0000'00c0u);
    REQUIRE(mmio.peek(kUsart0Base + 0x20u) == 7u);
    REQUIRE(mmio.peek(kUsart0Base + 0x1cu) == 0x41u);

    const auto expected_prefix = std::array{
        access{.kind = access_kind::write, .address = kPioBBase + 0x04u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x74u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x74u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x04u, .value = 1u << kRxLine, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x74u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x74u, .value = (1u << kTxLine) | (1u << kRxLine), .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x00u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x54u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x60u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x90u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x34u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x10u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart0Base + 0x00u, .value = 0x0000'01acu, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart0Base + 0x04u, .value = 0x0000'00c0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart0Base + 0x20u, .value = 7u, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart0Base + 0x00u, .value = 0x0000'0050u, .mask = 0u},
    };

    REQUIRE(trace.entries().size() >= expected_prefix.size() + 2u);
    for (auto index = std::size_t{0}; index < expected_prefix.size(); ++index) {
        INFO("trace entry " << index << '\n' << describe_trace(trace));
        REQUIRE(trace.entries()[index] == expected_prefix[index]);
    }

    const auto ready_index = expected_prefix.size();
    INFO(describe_trace(trace));
    REQUIRE(trace.entries()[ready_index] ==
            access{.kind = access_kind::read, .address = kUsart0Base + 0x14u, .value = 0x0000'0002u, .mask = 0u});
    REQUIRE(trace.entries()[ready_index + 1u] ==
            access{.kind = access_kind::write, .address = kUsart0Base + 0x1cu, .value = 0x41u, .mask = 0u});
}

}  // namespace alloy::test::mmio
