#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"
#include "device/runtime_lookup.hpp"
#include "device/traits.hpp"

namespace alloy::hal::uart::detail {

struct ParsedPinmuxTarget {
    char port = '\0';
    std::uint32_t line = 0;
    bool valid = false;
};

struct ParsedRegisterTarget {
    std::string_view owner{};
    std::string_view register_name{};
    std::string_view field_name{};
    bool valid = false;
};

[[nodiscard]] constexpr auto descriptor_as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto descriptor_strings_equal(const char* lhs, std::string_view rhs)
    -> bool {
    return descriptor_as_string(lhs) == rhs;
}

[[nodiscard]] inline auto mmio32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

[[nodiscard]] constexpr auto find_mmio_base(std::string_view peripheral_name) -> std::uintptr_t {
    if constexpr (!device::SelectedDescriptors::available) {
        return 0u;
    }

    for (const auto& descriptor : device::descriptors::tables::peripheral_bases) {
        if (descriptor_strings_equal(descriptor.name, peripheral_name)) {
            return descriptor.address;
        }
    }

    return 0u;
}

[[nodiscard]] constexpr auto find_rcc_descriptor(std::string_view peripheral_name)
    -> const device::descriptors::family::RccDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::rcc_map) {
        if (descriptor_strings_equal(descriptor.peripheral, peripheral_name)) {
            return &descriptor;
        }
    }

    return nullptr;
}

[[nodiscard]] constexpr auto gpio_port_index(char port) -> int {
    if (port < 'A' || port > 'Z') {
        return -1;
    }
    return port - 'A';
}

[[nodiscard]] constexpr auto gpio_peripheral_name(char port) -> std::string_view {
    switch (port) {
        case 'A':
            return "GPIOA";
        case 'B':
            return "GPIOB";
        case 'C':
            return "GPIOC";
        case 'D':
            return "GPIOD";
        case 'E':
            return "GPIOE";
        case 'F':
            return "GPIOF";
        case 'G':
            return "GPIOG";
        case 'H':
            return "GPIOH";
        case 'I':
            return "GPIOI";
        default:
            return {};
    }
}

[[nodiscard]] constexpr auto parse_decimal(std::string_view text) -> int {
    if (text.empty()) {
        return -1;
    }

    int value = 0;
    for (const char ch : text) {
        if (ch < '0' || ch > '9') {
            return -1;
        }
        value = (value * 10) + (ch - '0');
    }
    return value;
}

[[nodiscard]] constexpr auto parse_pid(std::string_view signal) -> int {
    const auto marker = signal.find("PID");
    if (marker == std::string_view::npos) {
        return -1;
    }
    return parse_decimal(signal.substr(marker + 3));
}

[[nodiscard]] constexpr auto parse_pinmux_target(std::string_view target) -> ParsedPinmuxTarget {
    constexpr std::string_view prefix = "pinmux.";
    const auto pin = target.starts_with(prefix) ? target.substr(prefix.size()) : target;
    if (pin.size() < 3u) {
        return {};
    }

    if (pin[0] != 'P') {
        return {};
    }

    const char port = pin[1];
    if (gpio_port_index(port) < 0) {
        return {};
    }

    const auto line = parse_decimal(pin.substr(2));
    if (line < 0 || line > 31) {
        return {};
    }

    return ParsedPinmuxTarget{
        .port = port,
        .line = static_cast<std::uint32_t>(line),
        .valid = true,
    };
}

[[nodiscard]] constexpr auto parse_register_target(std::string_view target)
    -> ParsedRegisterTarget {
    const auto separator = target.find('.');
    if (separator == std::string_view::npos || separator == 0u || separator + 1u >= target.size()) {
        return {};
    }

    const auto owner_and_register = target.substr(0, separator);
    const auto field_name = target.substr(separator + 1u);

    const auto owner_separator = owner_and_register.find('_');
    if (owner_separator == std::string_view::npos) {
        return ParsedRegisterTarget{
            .owner = owner_and_register,
            .register_name = owner_and_register,
            .field_name = field_name,
            .valid = true,
        };
    }

    return ParsedRegisterTarget{
        .owner = owner_and_register.substr(0, owner_separator),
        .register_name = owner_and_register.substr(owner_separator + 1u),
        .field_name = field_name,
        .valid = true,
    };
}

template <typename PortHandle>
[[nodiscard]] consteval auto is_stm32g0_family() -> bool {
    (void)sizeof(PortHandle);
    return device::SelectedDeviceTraits::vendor == std::string_view{"st"} &&
           device::SelectedDeviceTraits::family == std::string_view{"stm32g0"};
}

template <typename PortHandle>
[[nodiscard]] consteval auto is_stm32f4_family() -> bool {
    (void)sizeof(PortHandle);
    return device::SelectedDeviceTraits::vendor == std::string_view{"st"} &&
           device::SelectedDeviceTraits::family == std::string_view{"stm32f4"};
}

template <typename PortHandle>
[[nodiscard]] consteval auto is_same70_family() -> bool {
    (void)sizeof(PortHandle);
    return device::SelectedDeviceTraits::vendor == std::string_view{"microchip"} &&
           device::SelectedDeviceTraits::family == std::string_view{"same70"};
}

template <typename PortHandle, std::size_t... Index>
[[nodiscard]] constexpr auto has_signal_impl(std::string_view signal_name,
                                             std::index_sequence<Index...>) -> bool {
    using connector_type = typename PortHandle::connector_type;
    return (((connector_type::template binding_type<Index>::signal_type::name) == signal_name) ||
            ...);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto has_signal(std::string_view signal_name) -> bool {
    using connector_type = typename PortHandle::connector_type;
    return has_signal_impl<PortHandle>(signal_name,
                                       std::make_index_sequence<connector_type::binding_count>{});
}

[[nodiscard]] constexpr auto stm32g0_rcc_offset(std::string_view register_name) -> std::uint32_t {
    if (register_name == "IOPENR") {
        return 0x34u;
    }
    if (register_name == "IOPRSTR") {
        return 0x24u;
    }
    if (register_name == "AHBENR") {
        return 0x38u;
    }
    if (register_name == "AHBRSTR") {
        return 0x28u;
    }
    if (register_name == "APBENR1") {
        return 0x3Cu;
    }
    if (register_name == "APBRSTR1") {
        return 0x2Cu;
    }
    if (register_name == "APBENR2") {
        return 0x40u;
    }
    if (register_name == "APBRSTR2") {
        return 0x30u;
    }
    return 0u;
}

[[nodiscard]] constexpr auto stm32g0_rcc_bit(std::string_view field_name) -> int {
    if (field_name.starts_with("GPIO") && field_name.size() >= 7u) {
        return gpio_port_index(field_name[4]);
    }
    if (field_name.starts_with("USART")) {
        const auto index = parse_decimal(field_name.substr(5, field_name.size() - 7u));
        if (index == 1) {
            return 14;
        }
        if (index >= 2 && index <= 6) {
            return index + 15;
        }
        return -1;
    }
    if (field_name.starts_with("LPUART")) {
        const auto index = parse_decimal(field_name.substr(7, field_name.size() - 9u));
        if (index >= 1 && index <= 2) {
            return index + 22;
        }
        return -1;
    }
    return -1;
}

[[nodiscard]] constexpr auto stm32f4_rcc_offset(std::string_view register_name) -> std::uint32_t {
    if (register_name == "AHB1ENR") {
        return 0x30u;
    }
    if (register_name == "AHB1RSTR") {
        return 0x10u;
    }
    if (register_name == "APB1ENR") {
        return 0x40u;
    }
    if (register_name == "APB1RSTR") {
        return 0x20u;
    }
    if (register_name == "APB2ENR") {
        return 0x44u;
    }
    if (register_name == "APB2RSTR") {
        return 0x24u;
    }
    return 0u;
}

[[nodiscard]] constexpr auto stm32f4_rcc_bit(std::string_view field_name) -> int {
    if (field_name.starts_with("GPIO") && field_name.size() >= 7u) {
        return gpio_port_index(field_name[4]);
    }
    if (field_name.starts_with("USART")) {
        const auto index = parse_decimal(field_name.substr(5, field_name.size() - 7u));
        if (index == 1) {
            return 4;
        }
        if (index >= 2 && index <= 3) {
            return index + 15;
        }
        if (index == 6) {
            return 5;
        }
        return -1;
    }
    if (field_name.starts_with("UART")) {
        const auto index = parse_decimal(field_name.substr(4, field_name.size() - 6u));
        if (index >= 4 && index <= 5) {
            return index + 15;
        }
        return -1;
    }
    return -1;
}

inline auto enable_same70_pid(int pid) -> core::Result<void, core::ErrorCode> {
    constexpr auto pmc_base = find_mmio_base("PMC");
    if (pmc_base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (pid < 0) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (pid < 32) {
        mmio32(pmc_base + 0x10u) = (1u << pid);  // PCER0
        return core::Ok();
    }

    if (pid < 64) {
        mmio32(pmc_base + 0x100u) = (1u << (pid - 32));  // PCER1
        return core::Ok();
    }

    return core::Err(core::ErrorCode::OutOfRange);
}

template <typename PortHandle>
auto enable_gpio_clock_for_port(char port) -> core::Result<void, core::ErrorCode> {
    const auto port_index = gpio_port_index(port);
    if (port_index < 0) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if constexpr (is_stm32g0_family<PortHandle>()) {
        constexpr auto rcc_base = find_mmio_base("RCC");
        static_assert(rcc_base != 0u, "STM32G0 runtime requires RCC base address.");

        const auto bit_mask = (1u << port_index);
        mmio32(rcc_base + 0x34u) |= bit_mask;   // IOPENR
        mmio32(rcc_base + 0x24u) &= ~bit_mask;  // IOPRSTR
        return core::Ok();
    }

    if constexpr (is_stm32f4_family<PortHandle>()) {
        constexpr auto rcc_base = find_mmio_base("RCC");
        static_assert(rcc_base != 0u, "STM32F4 runtime requires RCC base address.");

        const auto bit_mask = (1u << port_index);
        mmio32(rcc_base + 0x30u) |= bit_mask;   // AHB1ENR
        mmio32(rcc_base + 0x10u) &= ~bit_mask;  // AHB1RSTR
        return core::Ok();
    }

    if constexpr (is_same70_family<PortHandle>()) {
        const auto descriptor = find_rcc_descriptor(gpio_peripheral_name(port));
        if (descriptor == nullptr) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        const auto pid = parse_pid(descriptor_as_string(descriptor->enable_signal));
        return enable_same70_pid(pid);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto apply_stm32_pinmux(const ParsedPinmuxTarget& pin_target, std::uint32_t selector)
    -> core::Result<void, core::ErrorCode> {
    const auto clock_result = enable_gpio_clock_for_port<PortHandle>(pin_target.port);
    if (clock_result.is_err()) {
        return clock_result;
    }

    constexpr auto moder_offset = 0x00u;
    constexpr auto afrl_offset = 0x20u;
    constexpr auto afrh_offset = 0x24u;

    const auto gpio_base = find_mmio_base(gpio_peripheral_name(pin_target.port));
    if (gpio_base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto shift = pin_target.line * 2u;
    auto& moder = mmio32(gpio_base + moder_offset);
    moder = (moder & ~(0x3u << shift)) | (0x2u << shift);

    if (pin_target.line < 8u) {
        const auto af_shift = pin_target.line * 4u;
        auto& afrl = mmio32(gpio_base + afrl_offset);
        afrl = (afrl & ~(0xFu << af_shift)) | ((selector & 0xFu) << af_shift);
    } else {
        const auto af_shift = (pin_target.line - 8u) * 4u;
        auto& afrh = mmio32(gpio_base + afrh_offset);
        afrh = (afrh & ~(0xFu << af_shift)) | ((selector & 0xFu) << af_shift);
    }

    return core::Ok();
}

template <typename PortHandle>
auto apply_same70_pinmux(const ParsedPinmuxTarget& pin_target, std::uint32_t selector)
    -> core::Result<void, core::ErrorCode> {
    if (selector > 3u) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    const auto clock_result = enable_gpio_clock_for_port<PortHandle>(pin_target.port);
    if (clock_result.is_err()) {
        return clock_result;
    }

    const auto gpio_base = find_mmio_base(gpio_peripheral_name(pin_target.port));
    if (gpio_base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto mask = (1u << pin_target.line);
    mmio32(gpio_base + 0x04u) = mask;  // PDR

    auto& abcdsr0 = mmio32(gpio_base + 0x70u);
    auto& abcdsr1 = mmio32(gpio_base + 0x74u);
    abcdsr0 = (selector & 0x1u) != 0u ? (abcdsr0 | mask) : (abcdsr0 & ~mask);
    abcdsr1 = (selector & 0x2u) != 0u ? (abcdsr1 | mask) : (abcdsr1 & ~mask);
    return core::Ok();
}

template <typename PortHandle>
auto apply_stm32_rcc_operation(const ParsedRegisterTarget& target, bool set_bit)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto rcc_base = find_mmio_base("RCC");
    static_assert(rcc_base != 0u, "STM32 runtime requires RCC base address.");

    std::uint32_t register_offset = 0u;
    int bit_index = -1;

    if constexpr (is_stm32g0_family<PortHandle>()) {
        register_offset = stm32g0_rcc_offset(target.register_name);
        bit_index = stm32g0_rcc_bit(target.field_name);
    } else if constexpr (is_stm32f4_family<PortHandle>()) {
        register_offset = stm32f4_rcc_offset(target.register_name);
        bit_index = stm32f4_rcc_bit(target.field_name);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }

    if (register_offset == 0u || bit_index < 0) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto& reg = mmio32(rcc_base + register_offset);
    const auto bit_mask = (1u << bit_index);
    if (set_bit) {
        reg |= bit_mask;
    } else {
        reg &= ~bit_mask;
    }
    return core::Ok();
}

template <typename PortHandle>
auto apply_route_operation(
    const device::descriptors::family::RouteOperationDescriptor& operation_descriptor)
    -> core::Result<void, core::ErrorCode> {
    const auto kind = descriptor_as_string(operation_descriptor.kind);
    const auto schema_id = descriptor_as_string(operation_descriptor.schema_id);

    if (kind == "write-selector") {
        const auto pin_target =
            parse_pinmux_target(descriptor_as_string(operation_descriptor.target_ref_id));
        if (!pin_target.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        if (operation_descriptor.value_int < 0) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        if (schema_id == "alloy.pinmux.stm32-af-v1") {
            return apply_stm32_pinmux<PortHandle>(
                pin_target, static_cast<std::uint32_t>(operation_descriptor.value_int));
        }

        if (schema_id == "alloy.pinmux.sam-pio-v1") {
            return apply_same70_pinmux<PortHandle>(
                pin_target, static_cast<std::uint32_t>(operation_descriptor.value_int));
        }

        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto register_peripheral = descriptor_as_string(operation_descriptor.register_peripheral);
    const auto register_name = descriptor_as_string(operation_descriptor.register_name);
    if (register_peripheral.empty() || register_name.empty()) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto base = find_mmio_base(register_peripheral);
    const auto* register_descriptor =
        device::runtime::find_register(register_peripheral, register_name);
    if (base == 0u || register_descriptor == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto register_offset = register_descriptor->offset_bytes;
    auto& reg = mmio32(base + register_offset);

    if (const auto field_runtime_id = descriptor_as_string(operation_descriptor.register_field_id);
        !field_runtime_id.empty()) {
        const auto* field_descriptor = device::runtime::find_register_field_by_runtime_id(
            register_peripheral, register_name, field_runtime_id);
        if (field_descriptor == nullptr) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        auto value = static_cast<std::uint32_t>(operation_descriptor.value_int);
        if (kind == "set-bit") {
            value = 1u;
        } else if (kind == "clear-bit") {
            value = 0u;
        } else if (kind != "write-field") {
            return core::Err(core::ErrorCode::NotSupported);
        }

        const auto mask = device::runtime::field_mask(*field_descriptor);
        reg = (reg & ~mask) | ((value << field_descriptor->bit_offset) & mask);
        return core::Ok();
    }

    if (kind == "write-register") {
        reg = static_cast<std::uint32_t>(operation_descriptor.value_int);
        return core::Ok();
    }

    if constexpr (is_same70_family<PortHandle>()) {
        const auto parsed_target =
            parse_register_target(descriptor_as_string(operation_descriptor.diagnostic_target));
        if (parsed_target.valid && parsed_target.owner == "PMC" && kind == "set-bit") {
            return enable_same70_pid(
                parse_pid(descriptor_as_string(operation_descriptor.diagnostic_target)));
        }
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
auto apply_route_operations() -> core::Result<void, core::ErrorCode> {
    constexpr auto operations = PortHandle::operations();
    for (std::size_t index = 0; index < operations.size(); ++index) {
        const auto* operation_descriptor = operations[index];
        if (operation_descriptor == nullptr) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        const auto result = apply_route_operation<PortHandle>(*operation_descriptor);
        if (result.is_err()) {
            return result;
        }
    }
    return core::Ok();
}

[[nodiscard]] constexpr auto baud_value(Baudrate baudrate) -> std::uint32_t {
    return static_cast<std::uint32_t>(baudrate);
}

[[nodiscard]] constexpr auto same70_mode_register(const UartConfig& config)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (config.flow_control != FlowControl::None) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    std::uint32_t mode = 0u;
    mode |= (0u << 0u);  // USART_MODE::NORMAL
    mode |= (0u << 4u);  // USCLKS::MCK

    switch (config.data_bits) {
        case DataBits::Five:
            mode |= (0u << 6u);
            break;
        case DataBits::Six:
            mode |= (0x1u << 6u);
            break;
        case DataBits::Seven:
            mode |= (0x2u << 6u);
            break;
        case DataBits::Eight:
            mode |= (0x3u << 6u);
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    switch (config.parity) {
        case Parity::Even:
            mode |= (0u << 9u);
            break;
        case Parity::Odd:
            mode |= (0x1u << 9u);
            break;
        case Parity::None:
            mode |= (0x4u << 9u);
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    switch (config.stop_bits) {
        case StopBits::One:
            mode |= (0u << 12u);
            break;
        case StopBits::OneAndHalf:
            mode |= (0x1u << 12u);
            break;
        case StopBits::Two:
            mode |= (0x2u << 12u);
            break;
    }

    return core::Ok(static_cast<std::uint32_t>(mode));
}

template <typename PortHandle>
auto configure_stm32_uart(const typename PortHandle::connector_type&, const UartConfig& config,
                          std::uintptr_t base) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || baud_value(config.baudrate) == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    if (config.flow_control != FlowControl::None) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto has_tx_signal = has_signal<PortHandle>("tx");
    const auto has_rx_signal = has_signal<PortHandle>("rx");
    if (!has_tx_signal && !has_rx_signal) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    std::uint32_t cr1_offset = 0u;
    std::uint32_t cr2_offset = 0u;
    std::uint32_t brr_offset = 0u;
    std::uint32_t ue_mask = 0u;
    std::uint32_t m1_mask = 0u;
    std::uint32_t m0_mask = 0u;

    if constexpr (is_stm32g0_family<PortHandle>()) {
        cr1_offset = 0x00u;
        cr2_offset = 0x04u;
        brr_offset = 0x0Cu;
        ue_mask = (1u << 0u);
        m1_mask = (1u << 28u);
        m0_mask = (1u << 12u);
    } else if constexpr (is_stm32f4_family<PortHandle>()) {
        cr1_offset = 0x0Cu;
        cr2_offset = 0x10u;
        brr_offset = 0x08u;
        ue_mask = (1u << 13u);
        m1_mask = 0u;
        m0_mask = (1u << 12u);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto& cr1 = mmio32(base + cr1_offset);
    auto& cr2 = mmio32(base + cr2_offset);
    auto& brr = mmio32(base + brr_offset);

    cr1 &= ~(ue_mask | (1u << 3u) | (1u << 2u) | (1u << 10u) | (1u << 9u) | m0_mask | m1_mask);
    cr2 &= ~(0x3u << 12u);

    switch (config.data_bits) {
        case DataBits::Seven:
            if constexpr (is_stm32g0_family<PortHandle>()) {
                cr1 |= m1_mask;
            } else {
                return core::Err(core::ErrorCode::NotSupported);
            }
            break;
        case DataBits::Eight:
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    switch (config.parity) {
        case Parity::None:
            break;
        case Parity::Even:
            cr1 |= (1u << 10u);
            break;
        case Parity::Odd:
            cr1 |= (1u << 10u) | (1u << 9u);
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    switch (config.stop_bits) {
        case StopBits::One:
            break;
        case StopBits::Two:
            cr2 |= (0x2u << 12u);
            break;
        default:
            return core::Err(core::ErrorCode::NotSupported);
    }

    brr = (config.peripheral_clock_hz + (baud_value(config.baudrate) / 2u)) /
          baud_value(config.baudrate);

    if (has_rx_signal) {
        cr1 |= (1u << 2u);
    }
    if (has_tx_signal) {
        cr1 |= (1u << 3u);
    }
    cr1 |= ue_mask;

    return core::Ok();
}

template <typename PortHandle>
auto configure_same70_uart(const typename PortHandle::connector_type&, const UartConfig& config,
                           std::uintptr_t base) -> core::Result<void, core::ErrorCode> {
    if (config.peripheral_clock_hz == 0u || baud_value(config.baudrate) == 0u) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto mode_result = same70_mode_register(config);
    if (mode_result.is_err()) {
        return core::Err(core::ErrorCode{mode_result.unwrap_err()});
    }

    const auto has_tx_signal = has_signal<PortHandle>("tx");
    const auto has_rx_signal = has_signal<PortHandle>("rx");
    if (!has_tx_signal && !has_rx_signal) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    auto& cr = mmio32(base + 0x00u);
    auto& mr = mmio32(base + 0x04u);
    auto& brgr = mmio32(base + 0x20u);

    cr = (1u << 2u) | (1u << 3u) | (1u << 5u) | (1u << 7u);
    mr = mode_result.unwrap();

    const auto divisor = 16u * baud_value(config.baudrate);
    std::uint32_t cd = (config.peripheral_clock_hz + (divisor / 2u)) / divisor;
    if (cd == 0u) {
        cd = 1u;
    }
    brgr = cd;

    std::uint32_t enable_mask = 0u;
    if (has_rx_signal) {
        enable_mask |= (1u << 4u);
    }
    if (has_tx_signal) {
        enable_mask |= (1u << 6u);
    }
    cr = enable_mask;
    return core::Ok();
}

template <typename PortHandle>
auto configure_uart(const PortHandle& handle) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto base = PortHandle::base_address();
    if (base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto operations_result = apply_route_operations<PortHandle>();
    if (operations_result.is_err()) {
        return operations_result;
    }

    if constexpr (is_stm32g0_family<PortHandle>() || is_stm32f4_family<PortHandle>()) {
        return configure_stm32_uart<PortHandle>(typename PortHandle::connector_type{},
                                                handle.config(), base);
    }

    if constexpr (is_same70_family<PortHandle>()) {
        return configure_same70_uart<PortHandle>(typename PortHandle::connector_type{},
                                                 handle.config(), base);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto tx_ready_mask() -> std::uint32_t {
    if constexpr (is_same70_family<PortHandle>()) {
        return (1u << 1u);
    }
    return (1u << 7u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto tx_complete_mask() -> std::uint32_t {
    if constexpr (is_same70_family<PortHandle>()) {
        return (1u << 9u);
    }
    return (1u << 6u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto rx_ready_mask() -> std::uint32_t {
    return (1u << 5u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto same70_rx_ready_mask() -> std::uint32_t {
    return (1u << 0u);
}

template <typename PortHandle>
[[nodiscard]] constexpr auto status_register_offset() -> std::uint32_t {
    if constexpr (is_stm32g0_family<PortHandle>()) {
        return 0x1Cu;
    }
    if constexpr (is_stm32f4_family<PortHandle>()) {
        return 0x00u;
    }
    if constexpr (is_same70_family<PortHandle>()) {
        return 0x14u;
    }
    return 0u;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto transmit_register_offset() -> std::uint32_t {
    if constexpr (is_stm32g0_family<PortHandle>()) {
        return 0x28u;
    }
    if constexpr (is_stm32f4_family<PortHandle>()) {
        return 0x04u;
    }
    if constexpr (is_same70_family<PortHandle>()) {
        return 0x1Cu;
    }
    return 0u;
}

template <typename PortHandle>
[[nodiscard]] constexpr auto receive_register_offset() -> std::uint32_t {
    if constexpr (is_stm32g0_family<PortHandle>()) {
        return 0x24u;
    }
    if constexpr (is_stm32f4_family<PortHandle>()) {
        return 0x04u;
    }
    if constexpr (is_same70_family<PortHandle>()) {
        return 0x18u;
    }
    return 0u;
}

template <typename PortHandle>
[[nodiscard]] auto wait_for_mask(std::uintptr_t base, std::uint32_t mask)
    -> core::Result<void, core::ErrorCode> {
    constexpr auto kPollLimit = 1'000'000u;
    const auto status_offset = status_register_offset<PortHandle>();
    if (status_offset == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    for (std::uint32_t remaining = kPollLimit; remaining > 0u; --remaining) {
        if ((mmio32(base + status_offset) & mask) != 0u) {
            return core::Ok();
        }
    }

    return core::Err(core::ErrorCode::Timeout);
}

template <typename PortHandle>
auto write_uart_byte(const PortHandle&, std::byte value) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto base = PortHandle::base_address();
    if (base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto ready = wait_for_mask<PortHandle>(base, tx_ready_mask<PortHandle>());
    if (ready.is_err()) {
        return ready;
    }

    const auto tx_offset = transmit_register_offset<PortHandle>();
    if (tx_offset == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    mmio32(base + tx_offset) = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value));
    return core::Ok();
}

template <typename PortHandle>
auto write_uart(const PortHandle& handle, std::span<const std::byte> buffer)
    -> core::Result<std::size_t, core::ErrorCode> {
    std::size_t written = 0;
    for (const auto value : buffer) {
        const auto write_result = write_uart_byte(handle, value);
        if (write_result.is_err()) {
            if (written == 0u) {
                return core::Err(core::ErrorCode{write_result.err()});
            }
            return core::Ok(static_cast<std::size_t>(written));
        }
        ++written;
    }
    return core::Ok(static_cast<std::size_t>(written));
}

template <typename PortHandle>
auto read_uart(const PortHandle&, std::span<std::byte> buffer)
    -> core::Result<std::size_t, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    if (buffer.empty()) {
        return core::Ok(std::size_t{0});
    }

    const auto base = PortHandle::base_address();
    if (base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto rx_mask = is_same70_family<PortHandle>() ? same70_rx_ready_mask<PortHandle>()
                                                        : rx_ready_mask<PortHandle>();
    const auto rx_offset = receive_register_offset<PortHandle>();
    if (rx_offset == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    std::size_t read = 0;
    for (; read < buffer.size(); ++read) {
        const auto ready = wait_for_mask<PortHandle>(base, rx_mask);
        if (ready.is_err()) {
            if (read == 0u) {
                return core::Err(core::ErrorCode::BufferEmpty);
            }
            return core::Ok(static_cast<std::size_t>(read));
        }

        buffer[read] = std::byte{static_cast<std::uint8_t>(mmio32(base + rx_offset) & 0xFFu)};
    }

    return core::Ok(static_cast<std::size_t>(read));
}

template <typename PortHandle>
auto flush_uart(const PortHandle&) -> core::Result<void, core::ErrorCode> {
    if constexpr (!PortHandle::valid) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }

    const auto base = PortHandle::base_address();
    if (base == 0u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    return wait_for_mask<PortHandle>(base, tx_complete_mask<PortHandle>());
}

}  // namespace alloy::hal::uart::detail
