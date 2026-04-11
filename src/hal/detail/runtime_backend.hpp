#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"
#include "device/runtime_lookup.hpp"

namespace alloy::hal::detail::runtime {

[[nodiscard]] constexpr auto as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return as_string(lhs) == rhs;
}

[[nodiscard]] inline auto mmio32(std::uintptr_t address) -> volatile std::uint32_t& {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

struct RegisterRef {
    std::uintptr_t base_address = 0u;
    std::uint32_t offset_bytes = 0u;
    bool valid = false;
};

struct FieldRef {
    RegisterRef reg{};
    std::uint16_t bit_offset = 0u;
    std::uint16_t bit_width = 0u;
    bool valid = false;
};

enum class GpioSchema : std::uint8_t {
    unknown,
    st_gpio,
    microchip_pio_v,
    nxp_imxrt_gpio_v1,
    nxp_gpio,
};

enum class UartSchema : std::uint8_t {
    unknown,
    st_sci3_v2_1_cube,
    st_sci2_v1_2_cube,
    microchip_uart_r,
    microchip_usart_zw,
    nxp_lpuart_v1,
};

enum class PinmuxSchema : std::uint8_t {
    unknown,
    stm32_af_v1,
    sam_pio_v1,
    imxrt_iomuxc_v1,
};

struct ParsedPinTarget {
    char port = '\0';
    std::uint32_t line = 0u;
    bool valid = false;
};

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

[[nodiscard]] constexpr auto parse_suffix_decimal(std::string_view text, std::string_view marker)
    -> int {
    const auto position = text.find(marker);
    if (position == std::string_view::npos) {
        return -1;
    }
    return parse_decimal(text.substr(position + marker.size()));
}

[[nodiscard]] constexpr auto gpio_port_index(char port) -> int {
    return (port >= 'A' && port <= 'Z') ? (port - 'A') : -1;
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

[[nodiscard]] constexpr auto parse_pin_target(std::string_view target) -> ParsedPinTarget {
    constexpr std::string_view prefix = "pinmux.";
    const auto pin = target.starts_with(prefix) ? target.substr(prefix.size()) : target;
    if (pin.size() < 3u || pin[0] != 'P') {
        return {};
    }

    const auto port = pin[1];
    if (gpio_port_index(port) < 0) {
        return {};
    }

    const auto line = parse_decimal(pin.substr(2u));
    if (line < 0 || line > 31) {
        return {};
    }

    return {
        .port = port,
        .line = static_cast<std::uint32_t>(line),
        .valid = true,
    };
}

[[nodiscard]] constexpr auto parse_pmc_pid(std::string_view register_name) -> int {
    if (!register_name.starts_with("PID")) {
        return -1;
    }
    return parse_decimal(register_name.substr(3u));
}

[[nodiscard]] constexpr auto make_indexed_field_name(std::string_view prefix,
                                                     std::uint32_t index)
    -> std::array<char, 32> {
    std::array<char, 32> buffer{};
    auto write = std::size_t{0};

    for (const char ch : prefix) {
        buffer[write++] = ch;
    }

    if (index >= 10u) {
        buffer[write++] = static_cast<char>('0' + (index / 10u));
    }
    buffer[write++] = static_cast<char>('0' + (index % 10u));
    buffer[write] = '\0';
    return buffer;
}

[[nodiscard]] constexpr auto make_lower_gate_name(std::string_view peripheral_name)
    -> std::array<char, 32> {
    std::array<char, 32> buffer{};
    constexpr std::string_view prefix = "gate:";
    auto write = std::size_t{0};

    for (const char ch : prefix) {
        buffer[write++] = ch;
    }

    for (const char ch : peripheral_name) {
        if (ch >= 'A' && ch <= 'Z') {
            buffer[write++] = static_cast<char>(ch - 'A' + 'a');
        } else {
            buffer[write++] = ch;
        }
    }

    buffer[write] = '\0';
    return buffer;
}

[[nodiscard]] constexpr auto field_mask(const FieldRef& field) -> std::uint32_t {
    if (!field.valid || field.bit_width == 0u) {
        return 0u;
    }
    if (field.bit_width >= 32u) {
        return 0xFFFF'FFFFu;
    }
    return ((1u << field.bit_width) - 1u) << field.bit_offset;
}

inline auto read_register(const RegisterRef& reg)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return core::Ok(static_cast<std::uint32_t>(mmio32(reg.base_address + reg.offset_bytes)));
}

inline auto write_register(const RegisterRef& reg, std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    if (!reg.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    mmio32(reg.base_address + reg.offset_bytes) = value;
    return core::Ok();
}

[[nodiscard]] constexpr auto field_bits(const FieldRef& field, std::uint32_t value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return core::Ok((value << field.bit_offset) & field_mask(field));
}

inline auto modify_field(const FieldRef& field, std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto current = mmio32(field.reg.base_address + field.reg.offset_bytes);
    current &= ~field_mask(field);
    current |= (value << field.bit_offset) & field_mask(field);
    mmio32(field.reg.base_address + field.reg.offset_bytes) = current;
    return core::Ok();
}

[[nodiscard]] inline auto read_field(const FieldRef& field)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    if (!field.valid) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto value = mmio32(field.reg.base_address + field.reg.offset_bytes);
    return core::Ok((value & field_mask(field)) >> field.bit_offset);
}

[[nodiscard]] constexpr auto runtime_profile_schema_id(std::string_view subsystem,
                                                       std::string_view peripheral_name)
    -> std::string_view {
    if (const auto* profile =
            device::runtime::find_runtime_profile(subsystem, "peripheral", peripheral_name);
        profile != nullptr) {
        return as_string(profile->schema_id);
    }
    return {};
}

[[nodiscard]] constexpr auto peripheral_schema_id(std::string_view peripheral_name)
    -> std::string_view {
    if (const auto* instance = device::runtime::find_peripheral_instance(peripheral_name);
        instance != nullptr) {
        return as_string(instance->backend_schema_id);
    }
    return {};
}

[[nodiscard]] constexpr auto gpio_schema_from_id(std::string_view schema_id) -> GpioSchema {
    if (schema_id == "alloy.gpio.st-gpio") {
        return GpioSchema::st_gpio;
    }
    if (schema_id == "alloy.gpio.microchip-pio-v") {
        return GpioSchema::microchip_pio_v;
    }
    if (schema_id == "alloy.gpio.nxp-imxrt-gpio-v1") {
        return GpioSchema::nxp_imxrt_gpio_v1;
    }
    if (schema_id == "alloy.gpio.nxp-gpio") {
        return GpioSchema::nxp_gpio;
    }
    return GpioSchema::unknown;
}

[[nodiscard]] constexpr auto uart_schema_from_id(std::string_view schema_id) -> UartSchema {
    if (schema_id == "alloy.uart.st-sci3-v2-1-cube") {
        return UartSchema::st_sci3_v2_1_cube;
    }
    if (schema_id == "alloy.uart.st-sci2-v1-2-cube") {
        return UartSchema::st_sci2_v1_2_cube;
    }
    if (schema_id == "alloy.uart.microchip-uart-r") {
        return UartSchema::microchip_uart_r;
    }
    if (schema_id == "alloy.uart.microchip-usart-zw") {
        return UartSchema::microchip_usart_zw;
    }
    if (schema_id == "alloy.uart.nxp-lpuart-v1") {
        return UartSchema::nxp_lpuart_v1;
    }
    return UartSchema::unknown;
}

[[nodiscard]] constexpr auto pinmux_schema_from_id(std::string_view schema_id) -> PinmuxSchema {
    if (schema_id == "alloy.pinmux.stm32-af-v1") {
        return PinmuxSchema::stm32_af_v1;
    }
    if (schema_id == "alloy.pinmux.sam-pio-v1") {
        return PinmuxSchema::sam_pio_v1;
    }
    if (schema_id == "alloy.pinmux.imxrt-iomuxc-v1") {
        return PinmuxSchema::imxrt_iomuxc_v1;
    }
    return PinmuxSchema::unknown;
}

template <typename Handle>
[[nodiscard]] consteval auto gpio_schema_for() -> GpioSchema {
    return gpio_schema_from_id(peripheral_schema_id(Handle::peripheral_name));
}

template <typename Handle>
[[nodiscard]] consteval auto uart_schema_for() -> UartSchema {
    return uart_schema_from_id(peripheral_schema_id(Handle::peripheral_name));
}

[[nodiscard]] inline auto read_register(std::string_view peripheral_name,
                                        std::string_view register_name)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    const auto* register_descriptor = device::runtime::find_register(peripheral_name, register_name);
    if (register_descriptor == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto* base = device::runtime::find_peripheral_instance(peripheral_name);
    if (base == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto value = static_cast<std::uint32_t>(
        mmio32(base->base_address + register_descriptor->offset_bytes));
    return core::Ok(static_cast<std::uint32_t>(value));
}

inline auto write_register(std::string_view peripheral_name, std::string_view register_name,
                           std::uint32_t value) -> core::Result<void, core::ErrorCode> {
    const auto* register_descriptor = device::runtime::find_register(peripheral_name, register_name);
    const auto* base = device::runtime::find_peripheral_instance(peripheral_name);
    if (register_descriptor == nullptr || base == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    mmio32(base->base_address + register_descriptor->offset_bytes) = value;
    return core::Ok();
}

[[nodiscard]] inline auto field_bits(std::string_view peripheral_name, std::string_view register_name,
                                     std::string_view field_name, std::uint32_t value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field(peripheral_name, register_name, field_name);
    if (field == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const auto mask = device::runtime::field_mask(*field);
    return core::Ok((value << field->bit_offset) & mask);
}

[[nodiscard]] inline auto field_bits_by_runtime_id(std::string_view peripheral_name,
                                                   std::string_view register_name,
                                                   std::string_view runtime_field_id,
                                                   std::uint32_t value)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field_by_runtime_id(
        peripheral_name, register_name, runtime_field_id);
    if (field == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    const auto mask = device::runtime::field_mask(*field);
    return core::Ok((value << field->bit_offset) & mask);
}

[[nodiscard]] inline auto modify_field(std::string_view peripheral_name, std::string_view register_name,
                                       std::string_view field_name, std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field(peripheral_name, register_name, field_name);
    const auto* reg = device::runtime::find_register(peripheral_name, register_name);
    const auto* base = device::runtime::find_peripheral_instance(peripheral_name);
    if (field == nullptr || reg == nullptr || base == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto& mmio = mmio32(base->base_address + reg->offset_bytes);
    const auto mask = device::runtime::field_mask(*field);
    mmio = (mmio & ~mask) | ((value << field->bit_offset) & mask);
    return core::Ok();
}

[[nodiscard]] inline auto modify_field_by_runtime_id(std::string_view peripheral_name,
                                                     std::string_view register_name,
                                                     std::string_view runtime_field_id,
                                                     std::uint32_t value)
    -> core::Result<void, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field_by_runtime_id(
        peripheral_name, register_name, runtime_field_id);
    const auto* reg = device::runtime::find_register(peripheral_name, register_name);
    const auto* base = device::runtime::find_peripheral_instance(peripheral_name);
    if (field == nullptr || reg == nullptr || base == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    auto& mmio = mmio32(base->base_address + reg->offset_bytes);
    const auto mask = device::runtime::field_mask(*field);
    mmio = (mmio & ~mask) | ((value << field->bit_offset) & mask);
    return core::Ok();
}

[[nodiscard]] inline auto read_field(std::string_view peripheral_name, std::string_view register_name,
                                     std::string_view field_name)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field(peripheral_name, register_name, field_name);
    const auto register_value = read_register(peripheral_name, register_name);
    if (field == nullptr || register_value.is_err()) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    return core::Ok((register_value.unwrap() & device::runtime::field_mask(*field)) >>
                    field->bit_offset);
}

[[nodiscard]] inline auto read_field_by_runtime_id(std::string_view peripheral_name,
                                                   std::string_view register_name,
                                                   std::string_view runtime_field_id)
    -> core::Result<std::uint32_t, core::ErrorCode> {
    const auto* field = device::runtime::find_register_field_by_runtime_id(
        peripheral_name, register_name, runtime_field_id);
    const auto register_value = read_register(peripheral_name, register_name);
    if (field == nullptr || register_value.is_err()) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    return core::Ok((register_value.unwrap() & device::runtime::field_mask(*field)) >>
                    field->bit_offset);
}

[[nodiscard]] inline auto fallback_register_field(std::string_view peripheral_name,
                                                  std::string_view register_name,
                                                  std::string_view diagnostic_target)
    -> core::Result<std::string_view, core::ErrorCode> {
    const auto separator = diagnostic_target.rfind('.');
    if (separator == std::string_view::npos || separator + 1u >= diagnostic_target.size()) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto direct_field = diagnostic_target.substr(separator + 1u);
    if (device::runtime::find_register_field(peripheral_name, register_name, direct_field) !=
        nullptr) {
        return core::Ok(std::string_view{direct_field});
    }

    if ((register_name == "IOPENR" || register_name == "IOPRSTR") && direct_field.starts_with("GPIO") &&
        direct_field.size() >= 6u) {
        static thread_local std::array<char, 16> normalized{};
        normalized = {};
        normalized[0] = 'I';
        normalized[1] = 'O';
        normalized[2] = 'P';
        auto write = std::size_t{3};
        for (std::size_t index = 4; index < direct_field.size(); ++index) {
            normalized[write++] = direct_field[index];
        }
        normalized[write] = '\0';
        if (device::runtime::find_register_field(peripheral_name, register_name, normalized.data()) !=
            nullptr) {
            return core::Ok(std::string_view{normalized.data()});
        }
    }

    return core::Err(core::ErrorCode::NotSupported);
}

inline auto enable_pmc_pid(int pid) -> core::Result<void, core::ErrorCode> {
    if (pid < 0 || pid >= 64) {
        return core::Err(core::ErrorCode::OutOfRange);
    }

    const auto* pmc = device::runtime::find_peripheral_instance("PMC");
    if (pmc == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto offset = pid < 32 ? 0x10u : 0x100u;
    const auto bit_index = pid < 32 ? pid : pid - 32;
    mmio32(pmc->base_address + offset) = (1u << bit_index);
    return core::Ok();
}

inline auto apply_clock_gate(
    const device::descriptors::family::ClockGateDescriptor& descriptor)
    -> core::Result<void, core::ErrorCode> {
    const auto peripheral_name = as_string(descriptor.register_peripheral);
    const auto register_name = as_string(descriptor.register_name);
    const auto runtime_field_id = as_string(descriptor.register_field_id);

    if (peripheral_name == "PMC" && register_name.starts_with("PID")) {
        return enable_pmc_pid(parse_pmc_pid(register_name));
    }

    if (!runtime_field_id.empty()) {
        return modify_field_by_runtime_id(peripheral_name, register_name, runtime_field_id, 1u);
    }

    const auto fallback = fallback_register_field(peripheral_name, register_name,
                                                  as_string(descriptor.enable_signal));
    if (fallback.is_ok()) {
        return modify_field(peripheral_name, register_name, fallback.unwrap(), 1u);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

inline auto release_reset(const device::descriptors::family::ResetDescriptor& descriptor)
    -> core::Result<void, core::ErrorCode> {
    const auto peripheral_name = as_string(descriptor.register_peripheral);
    const auto register_name = as_string(descriptor.register_name);
    const auto runtime_field_id = as_string(descriptor.register_field_id);
    if (!runtime_field_id.empty()) {
        return modify_field_by_runtime_id(peripheral_name, register_name, runtime_field_id, 0u);
    }

    const auto fallback = fallback_register_field(peripheral_name, register_name,
                                                  as_string(descriptor.reset_signal));
    if (fallback.is_ok()) {
        return modify_field(peripheral_name, register_name, fallback.unwrap(), 0u);
    }

    return core::Err(core::ErrorCode::NotSupported);
}

inline auto apply_clock_gate_by_name(std::string_view gate_name)
    -> core::Result<void, core::ErrorCode> {
    const auto* descriptor = device::runtime::find_clock_gate(gate_name);
    if (descriptor == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return apply_clock_gate(*descriptor);
}

inline auto release_reset_by_name(std::string_view reset_name)
    -> core::Result<void, core::ErrorCode> {
    const auto* descriptor = device::runtime::find_reset(reset_name);
    if (descriptor == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }
    return release_reset(*descriptor);
}

inline auto enable_gpio_port_runtime(std::string_view peripheral_name)
    -> core::Result<void, core::ErrorCode> {
    const auto* instance = device::runtime::find_peripheral_instance(peripheral_name);
    if (instance == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto gate_name = as_string(instance->clock_gate_id);
    if (!gate_name.empty()) {
        const auto gate_result = apply_clock_gate_by_name(gate_name);
        if (gate_result.is_err()) {
            return gate_result;
        }
    }

    const auto reset_name = as_string(instance->reset_id);
    if (!reset_name.empty()) {
        const auto reset_result = release_reset_by_name(reset_name);
        if (reset_result.is_err()) {
            return reset_result;
        }
    }

    return core::Ok();
}

inline auto enable_gpio_port_runtime(char port) -> core::Result<void, core::ErrorCode> {
    const auto peripheral_name = gpio_peripheral_name(port);
    if (peripheral_name.empty()) {
        return core::Err(core::ErrorCode::InvalidParameter);
    }
    return enable_gpio_port_runtime(peripheral_name);
}

inline auto apply_stm32_pinmux(const ParsedPinTarget& pin_target, std::uint32_t selector)
    -> core::Result<void, core::ErrorCode> {
    const auto peripheral_name = gpio_peripheral_name(pin_target.port);
    const auto enable_result = enable_gpio_port_runtime(peripheral_name);
    if (enable_result.is_err()) {
        return enable_result;
    }

    const auto moder_field = make_indexed_field_name("MODER", pin_target.line);
    const auto moder_result =
        modify_field(peripheral_name, "MODER", moder_field.data(), 0x2u);
    if (moder_result.is_err()) {
        return moder_result;
    }

    const auto afr_register = pin_target.line < 8u ? "AFRL" : "AFRH";
    const auto afr_field = make_indexed_field_name("AFSEL", pin_target.line);
    return modify_field(peripheral_name, afr_register, afr_field.data(), selector);
}

inline auto apply_same70_pinmux(const ParsedPinTarget& pin_target, std::uint32_t selector)
    -> core::Result<void, core::ErrorCode> {
    if (selector > 3u) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto peripheral_name = gpio_peripheral_name(pin_target.port);
    const auto enable_result = enable_gpio_port_runtime(peripheral_name);
    if (enable_result.is_err()) {
        return enable_result;
    }

    const auto pin_field = make_indexed_field_name("P", pin_target.line);
    const auto pdr_bits = field_bits(peripheral_name, "PDR", pin_field.data(), 1u);
    if (pdr_bits.is_err()) {
        return core::Err(core::ErrorCode{pdr_bits.unwrap_err()});
    }
    const auto pdr_result = write_register(peripheral_name, "PDR", pdr_bits.unwrap());
    if (pdr_result.is_err()) {
        return pdr_result;
    }

    const auto* abcdsr = device::runtime::find_register(peripheral_name, "ABCDSR[%s]");
    const auto* base = device::runtime::find_peripheral_instance(peripheral_name);
    const auto* field = device::runtime::find_register_field(peripheral_name, "ABCDSR[%s]", pin_field.data());
    if (abcdsr == nullptr || base == nullptr || field == nullptr) {
        return core::Err(core::ErrorCode::NotSupported);
    }

    const auto mask = device::runtime::field_mask(*field);
    auto& abcdsr0 = mmio32(base->base_address + abcdsr->offset_bytes);
    auto& abcdsr1 = mmio32(base->base_address + abcdsr->offset_bytes + sizeof(std::uint32_t));
    abcdsr0 = (selector & 0x1u) != 0u ? (abcdsr0 | mask) : (abcdsr0 & ~mask);
    abcdsr1 = (selector & 0x2u) != 0u ? (abcdsr1 | mask) : (abcdsr1 & ~mask);
    return core::Ok();
}

inline auto apply_route_operation(
    const device::descriptors::family::RouteOperationDescriptor& descriptor)
    -> core::Result<void, core::ErrorCode> {
    const auto kind = as_string(descriptor.kind);
    const auto schema_id = as_string(descriptor.schema_id);

    if (kind == "write-selector") {
        const auto pin_target = parse_pin_target(as_string(descriptor.target_ref_id));
        if (!pin_target.valid || descriptor.value_int < 0) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }

        switch (pinmux_schema_from_id(schema_id)) {
            case PinmuxSchema::stm32_af_v1:
                return apply_stm32_pinmux(pin_target,
                                          static_cast<std::uint32_t>(descriptor.value_int));
            case PinmuxSchema::sam_pio_v1:
                return apply_same70_pinmux(pin_target,
                                           static_cast<std::uint32_t>(descriptor.value_int));
            default:
                return core::Err(core::ErrorCode::NotSupported);
        }
    }

    const auto peripheral_name = as_string(descriptor.register_peripheral);
    const auto register_name = as_string(descriptor.register_name);
    const auto runtime_field_id = as_string(descriptor.register_field_id);

    if (kind == "write-register") {
        if (peripheral_name.empty() || register_name.empty()) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return write_register(peripheral_name, register_name,
                              static_cast<std::uint32_t>(descriptor.value_int));
    }

    if (!runtime_field_id.empty()) {
        if (kind == "set-bit") {
            return modify_field_by_runtime_id(peripheral_name, register_name, runtime_field_id,
                                              1u);
        }
        if (kind == "clear-bit") {
            return modify_field_by_runtime_id(peripheral_name, register_name, runtime_field_id,
                                              0u);
        }
        if (kind == "write-field") {
            return modify_field_by_runtime_id(
                peripheral_name, register_name, runtime_field_id,
                static_cast<std::uint32_t>(descriptor.value_int));
        }
    }

    if (schema_id == "alloy.clock.microchip-pmc-p" && peripheral_name == "PMC" &&
        register_name.starts_with("PID") && kind == "set-bit") {
        return enable_pmc_pid(parse_pmc_pid(register_name));
    }

    if ((kind == "set-bit" || kind == "clear-bit") && !peripheral_name.empty() &&
        !register_name.empty()) {
        const auto fallback =
            fallback_register_field(peripheral_name, register_name,
                                    as_string(descriptor.diagnostic_target));
        if (fallback.is_ok()) {
            return modify_field(peripheral_name, register_name, fallback.unwrap(),
                                kind == "set-bit" ? 1u : 0u);
        }
    }

    return core::Err(core::ErrorCode::NotSupported);
}

template <typename OperationList>
inline auto apply_route_operations(const OperationList& operations)
    -> core::Result<void, core::ErrorCode> {
    for (std::size_t index = 0; index < operations.size(); ++index) {
        const auto* descriptor = operations[index];
        if (descriptor == nullptr) {
            return core::Err(core::ErrorCode::InvalidParameter);
        }
        const auto result = apply_route_operation(*descriptor);
        if (result.is_err()) {
            return result;
        }
    }
    return core::Ok();
}

}  // namespace alloy::hal::detail::runtime
