#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/descriptors.hpp"
#include "device/traits.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/types.hpp"

namespace alloy::hal::gpio {

using Config = GpioConfig;
using Direction = PinDirection;
using State = PinState;
using Pull = PinPull;
using Drive = PinDrive;

namespace detail {

template <std::size_t Capacity>
struct StringList {
    std::array<std::string_view, Capacity> items{};
    std::size_t count = 0;

    [[nodiscard]] constexpr auto size() const -> std::size_t { return count; }
    [[nodiscard]] constexpr auto empty() const -> bool { return count == 0; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> std::string_view {
        return items[index];
    }
};

struct PinInfo {
    char port = '\0';
    int number = -1;
    bool valid = false;
};

struct PeripheralInfo {
    const char* name = nullptr;
    const char* schema_id = nullptr;
    const char* clock_gate_id = nullptr;
    const char* reset_id = nullptr;
    std::uintptr_t base_address = 0u;
    bool valid = false;
};

[[nodiscard]] constexpr auto as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return as_string(lhs) == rhs;
}

[[nodiscard]] consteval auto parse_trailing_number(std::string_view text) -> int {
    int value = 0;
    int multiplier = 1;
    bool found_digit = false;

    for (auto index = text.size(); index > 0; --index) {
        const char ch = text[index - 1];
        if (ch < '0' || ch > '9') {
            break;
        }
        found_digit = true;
        value += (ch - '0') * multiplier;
        multiplier *= 10;
    }

    return found_digit ? value : -1;
}

[[nodiscard]] consteval auto selected_device() -> std::string_view {
    return device::SelectedDeviceTraits::name;
}

[[nodiscard]] consteval auto find_pin(std::string_view pin_name) -> PinInfo {
    for (const auto& descriptor : device::descriptors::tables::pins) {
        if (!strings_equal(descriptor.pin_name, pin_name)) {
            continue;
        }

        const auto port = as_string(descriptor.port);
        return {
            .port = port.empty() ? '\0' : port.front(),
            .number = descriptor.number,
            .valid = true,
        };
    }

    return {};
}

[[nodiscard]] consteval auto find_gpio_peripheral_name(std::string_view pin_name) -> const char* {
    for (const auto& descriptor : device::descriptors::tables::pin_signals) {
        if (!strings_equal(descriptor.pin_name, pin_name)) {
            continue;
        }
        if (as_string(descriptor.function) != "gpio") {
            continue;
        }
        return descriptor.peripheral;
    }

    return nullptr;
}

[[nodiscard]] consteval auto find_gpio_line_index(std::string_view pin_name) -> int {
    for (const auto& descriptor : device::descriptors::tables::pin_signals) {
        if (!strings_equal(descriptor.pin_name, pin_name)) {
            continue;
        }
        if (as_string(descriptor.function) != "gpio") {
            continue;
        }

        if (const auto value = parse_trailing_number(as_string(descriptor.signal)); value >= 0) {
            return value;
        }
    }

    const auto pin = find_pin(pin_name);
    return pin.valid ? pin.number : -1;
}

[[nodiscard]] consteval auto find_peripheral(std::string_view peripheral_name) -> PeripheralInfo {
    for (const auto& descriptor : device::descriptors::tables::peripheral_instances) {
        if (!strings_equal(descriptor.name, peripheral_name)) {
            continue;
        }

        return {
            .name = descriptor.name,
            .schema_id = descriptor.backend_schema_id,
            .clock_gate_id = descriptor.clock_gate_id,
            .reset_id = descriptor.reset_id,
            .base_address = descriptor.base_address,
            .valid = true,
        };
    }

    return {};
}

[[nodiscard]] consteval auto find_register_ref(std::string_view peripheral_name,
                                               std::uintptr_t base_address,
                                               std::string_view register_name)
    -> hal::detail::runtime::RegisterRef {
    for (const auto& descriptor : device::descriptors::tables::registers) {
        if (!strings_equal(descriptor.peripheral_name, peripheral_name)) {
            continue;
        }
        if (!strings_equal(descriptor.register_name, register_name)) {
            continue;
        }

        return {
            .base_address = base_address,
            .offset_bytes = static_cast<std::uint32_t>(descriptor.offset_bytes),
            .valid = true,
        };
    }

    return {};
}

[[nodiscard]] consteval auto find_field_ref(std::string_view peripheral_name,
                                            std::uintptr_t base_address,
                                            std::string_view register_name,
                                            std::string_view field_name)
    -> hal::detail::runtime::FieldRef {
    const auto reg = find_register_ref(peripheral_name, base_address, register_name);
    if (!reg.valid) {
        return {};
    }

    for (const auto& descriptor : device::descriptors::tables::register_fields) {
        if (!strings_equal(descriptor.peripheral_name, peripheral_name)) {
            continue;
        }
        if (!strings_equal(descriptor.register_name, register_name)) {
            continue;
        }
        if (!strings_equal(descriptor.field_name, field_name)) {
            continue;
        }

        return {
            .reg = reg,
            .bit_offset = descriptor.bit_offset,
            .bit_width = descriptor.bit_width,
            .valid = true,
        };
    }

    return {};
}

[[nodiscard]] consteval auto find_indexed_field_ref(std::string_view peripheral_name,
                                                    std::uintptr_t base_address,
                                                    std::string_view register_name,
                                                    std::string_view prefix,
                                                    std::uint32_t index)
    -> hal::detail::runtime::FieldRef {
    const auto field_name = hal::detail::runtime::make_indexed_field_name(prefix, index);
    return find_field_ref(peripheral_name, base_address, register_name, field_name.data());
}

[[nodiscard]] consteval auto signal_field_name(std::string_view signal) -> std::array<char, 32> {
    std::array<char, 32> buffer{};

    const auto signal_start = signal.rfind('.');
    const auto token = signal_start == std::string_view::npos ? signal : signal.substr(signal_start + 1);

    if ((token.starts_with("GPIO") || token.starts_with("gpio")) && token.size() >= 6u &&
        (token.substr(5u) == "EN" || token.substr(5u) == "RST")) {
        buffer[0] = 'I';
        buffer[1] = 'O';
        buffer[2] = 'P';
        buffer[3] = static_cast<char>(token[4] >= 'a' && token[4] <= 'z' ? token[4] - 'a' + 'A'
                                                                          : token[4]);
        for (std::size_t index = 5; index < token.size(); ++index) {
            buffer[index - 1] = token[index];
        }
        return buffer;
    }

    for (std::size_t index = 0; index < token.size() && index < buffer.size() - 1; ++index) {
        const auto ch = token[index];
        buffer[index] =
            static_cast<char>(ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch);
    }

    return buffer;
}

[[nodiscard]] consteval auto find_clock_field_ref(std::string_view gate_name)
    -> hal::detail::runtime::FieldRef {
    for (const auto& descriptor : device::descriptors::tables::clock_gates) {
        if (!strings_equal(descriptor.device, selected_device())) {
            continue;
        }
        if (!strings_equal(descriptor.gate_name, gate_name)) {
            continue;
        }

        const auto rcc = find_peripheral(as_string(descriptor.register_peripheral));
        if (!rcc.valid) {
            return {};
        }

        auto field_name = signal_field_name(as_string(descriptor.enable_signal));
        return find_field_ref(as_string(descriptor.register_peripheral), rcc.base_address,
                              as_string(descriptor.register_name), field_name.data());
    }

    return {};
}

[[nodiscard]] consteval auto find_reset_field_ref(std::string_view reset_name)
    -> hal::detail::runtime::FieldRef {
    for (const auto& descriptor : device::descriptors::tables::resets) {
        if (!strings_equal(descriptor.device, selected_device())) {
            continue;
        }
        if (!strings_equal(descriptor.reset_name, reset_name)) {
            continue;
        }

        const auto rcc = find_peripheral(as_string(descriptor.register_peripheral));
        if (!rcc.valid) {
            return {};
        }

        auto field_name = signal_field_name(as_string(descriptor.reset_signal));
        return find_field_ref(as_string(descriptor.register_peripheral), rcc.base_address,
                              as_string(descriptor.register_name), field_name.data());
    }

    return {};
}

}  // namespace detail

template <typename Pin>
struct pin_handle {
    static constexpr bool available = device::SelectedDescriptors::available;
    static constexpr auto pin_info = detail::find_pin(Pin::name);
    static constexpr auto peripheral_name_cstr = detail::find_gpio_peripheral_name(Pin::name);
    static constexpr auto peripheral = detail::find_peripheral(detail::as_string(peripheral_name_cstr));
    static constexpr auto peripheral_name = detail::as_string(peripheral_name_cstr);
    static constexpr auto package_name =
        detail::as_string(device::descriptors::tables::device_descriptor.package_name);
    static constexpr auto line_index = detail::find_gpio_line_index(Pin::name);
    static constexpr auto schema =
        hal::detail::runtime::gpio_schema_from_id(detail::as_string(peripheral.schema_id));

    static constexpr auto clock_gate_field =
        detail::find_clock_field_ref(detail::as_string(peripheral.clock_gate_id));
    static constexpr auto reset_field =
        detail::find_reset_field_ref(detail::as_string(peripheral.reset_id));

    static constexpr auto mode_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "MODER", "MODER",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto output_type_field = detail::find_indexed_field_ref(
        peripheral_name, peripheral.base_address, "OTYPER", "OT", static_cast<std::uint32_t>(line_index));
    static constexpr auto pull_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PUPDR", "PUPDR",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto input_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "IDR", "IDR",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto output_set_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "BSRR", "BS",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto output_reset_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "BSRR", "BR",
                                       static_cast<std::uint32_t>(line_index));

    static constexpr auto pio_enable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PER", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_output_enable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "OER", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_output_disable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "ODR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_drive_enable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "MDER", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_drive_disable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "MDDR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_pull_up_enable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PUER", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_pull_up_disable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PUDR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_pull_down_enable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PPDER", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_pull_down_disable_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PPDDR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_set_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "SODR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_clear_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "CODR", "P",
                                       static_cast<std::uint32_t>(line_index));
    static constexpr auto pio_input_state_field =
        detail::find_indexed_field_ref(peripheral_name, peripheral.base_address, "PDSR", "P",
                                       static_cast<std::uint32_t>(line_index));

    static constexpr bool valid = [] {
        if constexpr (!available) {
            return false;
        }
        if (!pin_info.valid || !peripheral.valid || line_index < 0) {
            return false;
        }

        switch (schema) {
            case hal::detail::runtime::GpioSchema::st_gpio:
                return mode_field.valid && output_type_field.valid && pull_field.valid &&
                       input_field.valid && output_set_field.valid && output_reset_field.valid;
            case hal::detail::runtime::GpioSchema::microchip_pio_v:
                return pio_enable_field.valid && pio_output_enable_field.valid &&
                       pio_output_disable_field.valid && pio_set_field.valid &&
                       pio_clear_field.valid && pio_input_state_field.valid;
            default:
                return false;
        }
    }();

    [[nodiscard]] static consteval auto requirements() {
        detail::StringList<3> list{};
        if constexpr (!valid) {
            return list;
        }

        list.items[list.count++] = "package:selected";
        if (!detail::as_string(peripheral.clock_gate_id).empty()) {
            list.items[list.count++] = detail::as_string(peripheral.clock_gate_id);
        }
        if (!detail::as_string(peripheral.reset_id).empty()) {
            list.items[list.count++] = detail::as_string(peripheral.reset_id);
        }
        return list;
    }

    [[nodiscard]] static consteval auto operations() {
        detail::StringList<2> list{};
        if constexpr (!valid) {
            return list;
        }

        switch (schema) {
            case hal::detail::runtime::GpioSchema::st_gpio:
                list.items[list.count++] = "gpio:st-config";
                break;
            case hal::detail::runtime::GpioSchema::microchip_pio_v:
                list.items[list.count++] = "gpio:microchip-config";
                break;
            default:
                break;
        }
        return list;
    }

    constexpr explicit pin_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static constexpr auto pin_name() -> std::string_view { return Pin::name; }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral.base_address;
    }

    [[nodiscard]] auto configure() const -> core::Result<void, core::ErrorCode> {
        return detail::configure_gpio<pin_handle<Pin>>(config_);
    }

    [[nodiscard]] auto write(State state) const -> core::Result<void, core::ErrorCode> {
        return detail::write_gpio<pin_handle<Pin>>(config_, state);
    }

    [[nodiscard]] auto set_high() const -> core::Result<void, core::ErrorCode> {
        return write(State::High);
    }

    [[nodiscard]] auto set_low() const -> core::Result<void, core::ErrorCode> {
        return write(State::Low);
    }

    [[nodiscard]] auto toggle() const -> core::Result<void, core::ErrorCode> {
        return detail::toggle_gpio<pin_handle<Pin>>(config_);
    }

    [[nodiscard]] auto read() const -> core::Result<State, core::ErrorCode> {
        return detail::read_gpio<pin_handle<Pin>>();
    }

   private:
    Config config_{};
};

template <typename Pin>
[[nodiscard]] constexpr auto open(Config config = {}) -> pin_handle<Pin> {
    static_assert(pin_handle<Pin>::valid,
                  "Requested GPIO pin is not available for the selected device/package.");
    return pin_handle<Pin>{config};
}

}  // namespace alloy::hal::gpio
