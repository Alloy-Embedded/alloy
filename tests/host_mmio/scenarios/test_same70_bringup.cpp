#include "host_mmio/framework/mmio_space.hpp"
#include "host_mmio/framework/register_expect.hpp"
#include "host_mmio/framework/runtime_mmio.hpp"

#include "boards/same70_xplained/board_dma.hpp"
#include "boards/same70_xplained/board_spi.hpp"
#include "boards/same70_xplained/board_uart.hpp"
#include "device/runtime.hpp"
#include "hal/dma.hpp"
#include "hal/detail/resolved_route.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/pwm.hpp"
#include "hal/rtc.hpp"
#include "hal/spi.hpp"
#include "hal/timer.hpp"
#include "hal/uart/detail/backend.hpp"
#include "hal/types.hpp"
#include "low_power.hpp"

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

using RegisterId = alloy::device::runtime::RegisterId;
using FieldId = alloy::device::runtime::FieldId;

constexpr auto kPmcBase = std::uintptr_t{0x400e0600u};
constexpr auto kPmcPcer0 = std::uintptr_t{0x400e0610u};
constexpr auto kPmcPcer1 = std::uintptr_t{0x400e0700u};
constexpr auto kWdtBase = std::uintptr_t{0x400e1850u};
constexpr auto kRswdtBase = std::uintptr_t{0x400e1900u};
constexpr auto kResetControllerBase = std::uintptr_t{0x400e1800u};
constexpr auto kPioABase = std::uintptr_t{0x400e0e00u};
constexpr auto kPioBBase = std::uintptr_t{0x400e1000u};
constexpr auto kPioCBase = std::uintptr_t{0x400e1200u};
constexpr auto kUsart1Base = std::uintptr_t{0x40028000u};
constexpr auto kRtcBase = std::uintptr_t{0x400E1860u};
constexpr auto kXdmacBase = std::uintptr_t{0x40078000u};
constexpr auto kTc0Base = std::uintptr_t{0x4000C000u};
constexpr auto kPwm0Base = std::uintptr_t{0x40020000u};
constexpr auto kScbScrAddress = std::uintptr_t{0xE000ED10u};
constexpr auto kScbScrSleepdeepMask = std::uint32_t{1u << 2u};
constexpr auto kLedLine = std::uint16_t{8u};
constexpr auto kTxLine = std::uint16_t{4u};
constexpr auto kRxLine = std::uint16_t{21u};
constexpr auto kSame70PioSelectorA = std::uint32_t{0u};
constexpr auto kSame70PioSelectorD = std::uint32_t{3u};
constexpr auto kWatchdogGuardValue = std::uint32_t{0x0fffu};

[[nodiscard]] constexpr auto same70_usart_cd(std::uint32_t peripheral_clock_hz,
                                             std::uint32_t baudrate) -> std::uint32_t {
    return (peripheral_clock_hz + ((16u * baudrate) / 2u)) / (16u * baudrate);
}

[[nodiscard]] constexpr auto same70_spi_scbr(std::uint32_t peripheral_clock_hz,
                                             std::uint32_t target_clock_hz) -> std::uint32_t {
    auto divider = (peripheral_clock_hz + target_clock_hz - 1u) / target_clock_hz;
    if (divider == 0u) {
        divider = 1u;
    }
    if (divider > 255u) {
        divider = 255u;
    }
    return divider;
}

[[nodiscard]] constexpr auto register_ref(std::uintptr_t base_address, std::uint32_t offset_bytes)
    -> rt::RegisterRef {
    return {
        .base_address = base_address,
        .offset_bytes = offset_bytes,
        .valid = true,
    };
}

[[nodiscard]] constexpr auto register_address(const rt::RegisterRef& reg) -> std::uintptr_t {
    return reg.base_address + reg.offset_bytes;
}

template <RegisterId Id>
[[nodiscard]] consteval auto register_address() -> std::uintptr_t {
    constexpr auto reg = rt::register_ref<Id>();
    return register_address(reg);
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

template <std::size_t N>
[[nodiscard]] auto build_register_value(const std::array<std::pair<rt::FieldRef, std::uint32_t>, N>& fields)
    -> std::uint32_t {
    auto value = std::uint32_t{0u};
    for (const auto& [field, field_value] : fields) {
        if (!field.valid) {
            continue;
        }
        value |= rt::field_bits(field, field_value).unwrap();
    }
    return value;
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

    static constexpr auto us_cr_reg = register_ref(kUsart1Base, 0x00u);
    static constexpr auto us_mr_reg = register_ref(kUsart1Base, 0x04u);
    static constexpr auto us_brgr_reg = register_ref(kUsart1Base, 0x20u);
    [[maybe_unused]] static constexpr auto us_csr_reg = register_ref(kUsart1Base, 0x14u);
    static constexpr auto us_thr_reg = register_ref(kUsart1Base, 0x1cu);
    static constexpr auto us_rstrx_field = field_ref(kUsart1Base, 0x00u, 2u);
    static constexpr auto us_rsttx_field = field_ref(kUsart1Base, 0x00u, 3u);
    static constexpr auto us_rxdis_field = field_ref(kUsart1Base, 0x00u, 5u);
    static constexpr auto us_txdis_field = field_ref(kUsart1Base, 0x00u, 7u);
    static constexpr auto us_rststa_field = field_ref(kUsart1Base, 0x00u, 8u);
    static constexpr auto us_usart_mode_field = field_ref(kUsart1Base, 0x04u, 0u, 4u);
    static constexpr auto us_usclks_field = field_ref(kUsart1Base, 0x04u, 4u, 2u);
    static constexpr auto us_chrl_field = field_ref(kUsart1Base, 0x04u, 6u, 2u);
    static constexpr auto us_cd_field = field_ref(kUsart1Base, 0x20u, 0u, 16u);
    static constexpr auto us_rxen_field = field_ref(kUsart1Base, 0x00u, 4u);
    static constexpr auto us_txen_field = field_ref(kUsart1Base, 0x00u, 6u);
    static constexpr auto us_txrdy_field = field_ref(kUsart1Base, 0x14u, 1u);
    static constexpr auto us_txchr_field = field_ref(kUsart1Base, 0x1cu, 0u, 9u);

    explicit same70_debug_uart_handle(alloy::hal::UartConfig config) : config_{config} {}

    [[nodiscard]] auto config() const -> const alloy::hal::UartConfig& { return config_; }

    [[nodiscard]] static constexpr auto operations() {
        return route::List<route::Operation, 0u>{};
    }

  private:
    alloy::hal::UartConfig config_{};
};

struct low_power_test_state {
    mmio_space* mmio = nullptr;
    bool before_called = false;
    bool after_called = false;
    bool wait_called = false;
    alloy::device::low_power::ModeId before_mode = alloy::device::low_power::ModeId::none;
    alloy::device::low_power::ModeId after_mode = alloy::device::low_power::ModeId::none;
    std::uint32_t scr_during_wait = 0u;
};

[[nodiscard]] auto active_low_power_test_state() -> low_power_test_state*& {
    static auto* state = static_cast<low_power_test_state*>(nullptr);
    return state;
}

void low_power_before_entry(alloy::device::low_power::ModeId mode) {
    auto* state = active_low_power_test_state();
    REQUIRE(state != nullptr);
    state->before_called = true;
    state->before_mode = mode;
}

void low_power_after_wakeup(alloy::device::low_power::ModeId mode) {
    auto* state = active_low_power_test_state();
    REQUIRE(state != nullptr);
    state->after_called = true;
    state->after_mode = mode;
}

void low_power_wait_hook() {
    auto* state = active_low_power_test_state();
    REQUIRE(state != nullptr);
    REQUIRE(state->mmio != nullptr);
    state->wait_called = true;
    state->scr_during_wait = state->mmio->peek(kScbScrAddress);
}

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

    configure_same70_peripheral_mux(kPioBBase, kTxLine, kSame70PioSelectorD);
    configure_same70_peripheral_mux(kPioABase, kRxLine, kSame70PioSelectorA);

    const auto gpio_result = gpio_detail::configure_microchip_pio<same70_pio_pin_handle<kLedLine>>({
        .direction = alloy::hal::PinDirection::Output,
        .drive = alloy::hal::PinDrive::PushPull,
        .pull = alloy::hal::PinPull::None,
        .initial_state = alloy::hal::PinState::Low,
    });
    REQUIRE(gpio_result.is_ok());

    same70_debug_uart_handle uart_handle{
        alloy::hal::UartConfig{
            .baudrate = board::kDebugUartBaudrate,
            .data_bits = alloy::hal::DataBits::Eight,
            .parity = alloy::hal::Parity::None,
            .stop_bits = alloy::hal::StopBits::One,
            .flow_control = alloy::hal::FlowControl::None,
            .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
        },
    };
    const auto uart_result = uart_detail::configure_uart(uart_handle);
    REQUIRE(uart_result.is_ok());

    mmio.preload(kUsart1Base + 0x14u, 0x0000'0002u);
    const auto tx_result = uart_detail::write_uart_byte(uart_handle, std::byte{0x41});
    REQUIRE(tx_result.is_ok());

    REQUIRE(mmio.peek(kPioABase + 0x04u) == (1u << kRxLine));
    REQUIRE(mmio.peek(kPioABase + 0x70u) == 0x0000'0000u);
    REQUIRE(mmio.peek(kPioABase + 0x74u) == 0x0000'0000u);
    REQUIRE(mmio.peek(kPioBBase + 0x04u) == (1u << kTxLine));
    REQUIRE(mmio.peek(kPioBBase + 0x70u) == (1u << kTxLine));
    REQUIRE(mmio.peek(kPioBBase + 0x74u) == (1u << kTxLine));
    REQUIRE(mmio.peek(kPioCBase + 0x00u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x54u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x60u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x90u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x34u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kPioCBase + 0x10u) == (1u << kLedLine));
    REQUIRE(mmio.peek(kUsart1Base + 0x00u) == 0x0000'0050u);
    REQUIRE(mmio.peek(kUsart1Base + 0x04u) == 0x0000'08c0u);
    REQUIRE(mmio.peek(kUsart1Base + 0x20u) ==
            same70_usart_cd(board::kDebugUartPeripheralClockHz,
                            static_cast<std::uint32_t>(board::kDebugUartBaudrate)));
    REQUIRE(mmio.peek(kUsart1Base + 0x1cu) == 0x41u);

    const auto expected_prefix = std::array{
        access{.kind = access_kind::write, .address = kPioBBase + 0x04u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioBBase + 0x74u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x70u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioBBase + 0x74u, .value = 1u << kTxLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioABase + 0x04u, .value = 1u << kRxLine, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioABase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::read, .address = kPioABase + 0x74u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioABase + 0x70u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioABase + 0x74u, .value = 0u, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x00u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x54u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x60u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x90u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x34u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kPioCBase + 0x10u, .value = 1u << kLedLine, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart1Base + 0x00u, .value = 0x0000'01acu, .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart1Base + 0x04u, .value = 0x0000'08c0u, .mask = 0u},
        access{.kind = access_kind::write,
               .address = kUsart1Base + 0x20u,
               .value = same70_usart_cd(board::kDebugUartPeripheralClockHz,
                                        static_cast<std::uint32_t>(board::kDebugUartBaudrate)),
               .mask = 0u},
        access{.kind = access_kind::write, .address = kUsart1Base + 0x00u, .value = 0x0000'0050u, .mask = 0u},
    };

    REQUIRE(trace.entries().size() >= expected_prefix.size() + 2u);
    for (auto index = std::size_t{0}; index < expected_prefix.size(); ++index) {
        INFO("trace entry " << index << '\n' << describe_trace(trace));
        REQUIRE(trace.entries()[index] == expected_prefix[index]);
    }

    const auto ready_index = expected_prefix.size();
    INFO(describe_trace(trace));
    REQUIRE(trace.entries()[ready_index] ==
            access{.kind = access_kind::read, .address = kUsart1Base + 0x14u, .value = 0x0000'0002u, .mask = 0u});
    REQUIRE(trace.entries()[ready_index + 1u] ==
            access{.kind = access_kind::write, .address = kUsart1Base + 0x1cu, .value = 0x41u, .mask = 0u});
}

TEST_CASE("host mmio enables SAME70 board debug usart clock before configure",
          "[host-mmio][bring-up][same70][uart][board]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    auto uart = board::make_debug_uart({
        .baudrate = board::kDebugUartBaudrate,
        .data_bits = alloy::hal::DataBits::Eight,
        .parity = alloy::hal::Parity::None,
        .stop_bits = alloy::hal::StopBits::One,
        .flow_control = alloy::hal::FlowControl::None,
        .peripheral_clock_hz = board::kDebugUartPeripheralClockHz,
    });

    mmio.preload(kUsart1Base + 0x14u, 0x0000'0002u);

    REQUIRE(uart.configure().is_ok());
    REQUIRE(uart.write_byte(std::byte{0x41}).is_ok());

    REQUIRE(mmio.peek(kPmcPcer0) == ((1u << 10u) | (1u << 11u) | (1u << 14u)));
    REQUIRE(mmio.peek(kPioABase + 0x74u) == 0u);
    REQUIRE(mmio.peek(kPioBBase + 0x70u) == (1u << kTxLine));
    REQUIRE(mmio.peek(kPioBBase + 0x74u) == (1u << kTxLine));
    REQUIRE(mmio.peek(kUsart1Base + 0x00u) == 0x0000'0050u);
    REQUIRE(mmio.peek(kUsart1Base + 0x04u) == 0x0000'08c0u);
    REQUIRE(mmio.peek(kUsart1Base + 0x20u) ==
            same70_usart_cd(board::kDebugUartPeripheralClockHz,
                            static_cast<std::uint32_t>(board::kDebugUartBaudrate)));
    REQUIRE(mmio.peek(kUsart1Base + 0x1cu) == 0x41u);
}

TEST_CASE("host mmio covers SAME70 RTC update handshake",
          "[host-mmio][bring-up][same70][rtc]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    mmio.preload(kRtcBase + 0x18u, 0x0000'0001u);
    mmio.preload(kRtcBase + 0x08u, 0x0000'1234u);
    mmio.preload(kRtcBase + 0x0Cu, 0x0000'5678u);

    auto rtc = alloy::hal::rtc::open<alloy::device::runtime::PeripheralId::RTC>({
        .enable_write_access = true,
        .enter_init_mode = true,
        .enable_alarm_interrupt = false,
    });

    REQUIRE(rtc.configure().is_ok());
    REQUIRE(rtc.read_time().is_ok());
    REQUIRE(rtc.read_date().is_ok());
    REQUIRE(rtc.leave_init_mode().is_ok());

    REQUIRE(mmio.peek(kRtcBase + 0x00u) == 0x0000'0003u);
    REQUIRE(mmio.peek(kRtcBase + 0x1Cu) == 0x0000'0018u);
}

TEST_CASE("host mmio covers typed SAME70 timer control",
          "[host-mmio][bring-up][same70][timer]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    auto timer = alloy::hal::timer::open<alloy::device::runtime::PeripheralId::TC0>();

    REQUIRE(timer.start().is_ok());
    REQUIRE(timer.set_period(0x1234u).is_ok());
    REQUIRE(timer.stop().is_ok());

    REQUIRE(mmio.peek(kTc0Base + 0x00u) == 0x0000'0006u);
    REQUIRE(mmio.peek(kTc0Base + 0x1Cu) == 0x0000'1234u);
}

TEST_CASE("host mmio covers typed SAME70 pwm control",
          "[host-mmio][bring-up][same70][pwm]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    auto pwm = alloy::hal::pwm::open<alloy::device::runtime::PeripheralId::PWM0, 0u>();

    REQUIRE(pwm.set_period(0x0200u).is_ok());
    REQUIRE(pwm.set_duty_cycle(0x0080u).is_ok());
    REQUIRE(pwm.start().is_ok());
    REQUIRE(pwm.stop().is_ok());

    REQUIRE(mmio.peek(kPwm0Base + 0x20Cu) == 0x0000'0200u);
    REQUIRE(mmio.peek(kPwm0Base + 0x204u) == 0x0000'0080u);
    REQUIRE(mmio.peek(kPwm0Base + 0x04u) == 0x0000'0000u);
    REQUIRE(mmio.peek(kPwm0Base + 0x28u) == 0x0000'0001u);
}

TEST_CASE("host mmio covers typed SAME70 uart dma configuration",
          "[host-mmio][bring-up][same70][dma]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    auto uart = board::make_debug_uart();
    auto tx_dma = board::make_debug_uart_tx_dma({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .channel_index = 0,
    });
    auto rx_dma = board::make_debug_uart_rx_dma({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .channel_index = 1,
    });

    REQUIRE(uart.configure_tx_dma(tx_dma).is_ok());
    REQUIRE(uart.configure_rx_dma(rx_dma).is_ok());

    REQUIRE(mmio.peek(kPmcPcer1) == rt::field_mask(rt::field_ref<FieldId::field_pmc_pcer1_pid58>()));
    REQUIRE(mmio.peek(kXdmacBase + 0x20u) == 0x0000'0002u);
    REQUIRE(mmio.peek(kXdmacBase + 0x78u) == 0x0901'0011u);
    REQUIRE(mmio.peek(kXdmacBase + 0xB8u) == 0x0A04'0001u);
}

TEST_CASE("host mmio covers typed SAME70 spi shared-bus reconfiguration",
          "[host-mmio][bring-up][same70][shared-bus]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using SpiHandle = board::BoardSpi;

    auto bus = alloy::hal::spi::open_shared_bus<board::BoardSpiConnector>(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode0, 1'000'000u, alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8, board::kBoardSpiPeripheralClockHz});
    auto slow_device = bus.device(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode3, 500'000u, alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8, board::kBoardSpiPeripheralClockHz});

    REQUIRE(bus.configure().is_ok());
    const auto bus_csr = mmio.peek(register_address(SpiHandle::csr_reg));

    REQUIRE(slow_device.configure().is_ok());
    const auto slow_csr = mmio.peek(register_address(SpiHandle::csr_reg));

    const auto expected_bus_csr = build_register_value(std::array{
        std::pair{SpiHandle::cpol_field, 0u},
        std::pair{SpiHandle::ncpha_field, 1u},
        std::pair{SpiHandle::bits_field, 0u},
        std::pair{SpiHandle::scbr_field,
                  same70_spi_scbr(board::kBoardSpiPeripheralClockHz, 1'000'000u)},
        std::pair{SpiHandle::dlybs_field, 0u},
        std::pair{SpiHandle::dlybct_field, 0u},
    });
    const auto expected_slow_csr = build_register_value(std::array{
        std::pair{SpiHandle::cpol_field, 1u},
        std::pair{SpiHandle::ncpha_field, 0u},
        std::pair{SpiHandle::bits_field, 0u},
        std::pair{SpiHandle::scbr_field,
                  same70_spi_scbr(board::kBoardSpiPeripheralClockHz, 500'000u)},
        std::pair{SpiHandle::dlybs_field, 0u},
        std::pair{SpiHandle::dlybct_field, 0u},
    });

    REQUIRE(bus_csr == expected_bus_csr);
    REQUIRE(slow_csr == expected_slow_csr);
    REQUIRE(bus_csr != slow_csr);
}

TEST_CASE("host mmio exposes SAME70 low-power coordination hooks and wake-capable sources",
          "[host-mmio][bring-up][same70][low-power]") {
    trace_log trace;
    mmio_space mmio{trace};
    runtime_mmio_scope scope{mmio};

    using ModeId = alloy::device::low_power::ModeId;
    using namespace alloy::low_power;
    constexpr auto wakeup_source = alloy::device::low_power::wakeup_sources.front().wakeup_source_id;
    using WakeupCompletion = wakeup_token<wakeup_source>;

    low_power_test_state state{.mmio = &mmio};
    active_low_power_test_state() = &state;
    mmio.preload(kScbScrAddress, 0x0000'0010u);
    WakeupCompletion::reset();

    const auto previous_hooks = set_hooks({low_power_before_entry, low_power_after_wakeup});
    const auto previous_wait_hook = detail::test_support::set_wait_hook(low_power_wait_hook);

    const auto result = enter(ModeId::deep_sleep);

    static_cast<void>(detail::test_support::set_wait_hook(previous_wait_hook));
    static_cast<void>(set_hooks(previous_hooks));
    active_low_power_test_state() = nullptr;

    REQUIRE(result.is_ok());
    REQUIRE(state.before_called);
    REQUIRE(state.after_called);
    REQUIRE(state.wait_called);
    REQUIRE(state.before_mode == ModeId::deep_sleep);
    REQUIRE(state.after_mode == ModeId::deep_sleep);
    REQUIRE((state.scr_during_wait & kScbScrSleepdeepMask) != 0u);
    REQUIRE(mmio.peek(kScbScrAddress) == 0x0000'0010u);

    REQUIRE(!alloy::device::low_power::wakeup_sources.empty());
    REQUIRE(wakeup_source_valid_in(ModeId::deep_sleep, wakeup_source));
    REQUIRE(completion_valid_in<WakeupCompletion>(ModeId::deep_sleep));

    WakeupCompletion::signal();
    REQUIRE(WakeupCompletion::ready());
}

}  // namespace alloy::test::mmio
