#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include "hal/connect/tags.hpp"
#include "hal/gpio/detail/backend.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/runtime.hpp"
#include "device/traits.hpp"

namespace alloy::hal::gpio {

using Config = GpioConfig;
using Direction = PinDirection;
using State = PinState;
using Pull = PinPull;
using Drive = PinDrive;

template <device::PinId Id>
using pin = device::pin<Id>;

namespace detail {

namespace rt = alloy::hal::detail::runtime;

template <typename ClassId>
[[nodiscard]] consteval auto peripheral_class_is_gpio(ClassId id) -> bool {
    if constexpr (requires { ClassId::class_gpio; }) {
        return id == ClassId::class_gpio;
    } else {
        return false;
    }
}

using RuntimeRegisterRef = rt::RegisterRef;
using RuntimeFieldRef = rt::FieldRef;
using RuntimePinId = device::PinId;
using RuntimePeripheralId = device::PeripheralId;

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

[[nodiscard]] constexpr auto ascii_lower(char ch) -> char {
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

[[nodiscard]] constexpr auto ascii_iequals(std::string_view lhs, std::string_view rhs) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (ascii_lower(lhs[index]) != ascii_lower(rhs[index])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] constexpr auto trim_case_insensitive_prefix(std::string_view text,
                                                          std::string_view prefix)
    -> std::string_view {
    if (text.size() < prefix.size()) {
        return text;
    }

    if (!ascii_iequals(text.substr(0u, prefix.size()), prefix)) {
        return text;
    }

    text.remove_prefix(prefix.size());
    if (!text.empty() && text.front() == '_') {
        text.remove_prefix(1u);
    }
    return text;
}

template <auto Value>
[[nodiscard]] consteval auto enum_name() -> std::string_view {
#if defined(__clang__) || defined(__GNUC__)
    constexpr std::string_view function = __PRETTY_FUNCTION__;
    constexpr std::string_view needle = "Value = ";
#elif defined(_MSC_VER)
    constexpr std::string_view function = __FUNCSIG__;
    constexpr std::string_view needle = "enum_name<";
#else
    #error Unsupported compiler for compile-time enum name extraction.
#endif

    const auto start = function.find(needle);
    static_assert(start != std::string_view::npos,
                  "Unable to parse compiler function signature for enum name extraction.");

    auto value_start = start + needle.size();
#if defined(_MSC_VER)
    const auto value_end = function.find(">(void)", value_start);
#else
    auto value_end = function.find(';', value_start);
    const auto bracket_end = function.find(']', value_start);
    if (value_end == std::string_view::npos ||
        (bracket_end != std::string_view::npos && bracket_end < value_end)) {
        value_end = bracket_end;
    }
#endif

    auto value = function.substr(value_start, value_end - value_start);
    if (const auto scope = value.rfind("::"); scope != std::string_view::npos) {
        value.remove_prefix(scope + 2u);
    }
    return value;
}

[[nodiscard]] constexpr auto selected_device_name() -> std::string_view {
    return device::SelectedDeviceTraits::name;
}

template <auto Value>
[[nodiscard]] consteval auto selected_scoped_enum_name() -> std::string_view {
    auto name = enum_name<Value>();
    name = trim_case_insensitive_prefix(name, selected_device_name());
    return name;
}

template <std::size_t... Index>
[[nodiscard]] consteval auto find_gpio_peripheral_id_impl(device::PortId port_id,
                                                          std::index_sequence<Index...>)
    -> RuntimePeripheralId {
    auto resolved = RuntimePeripheralId::none;
    const auto target_instance = port_id == device::PortId::none ? -1 : (static_cast<int>(port_id) - 1);

    auto match = [&]<std::size_t I>() consteval {
        constexpr auto peripheral_id = device::runtime::runtime_peripheral_ids[I];
        using traits = device::PeripheralInstanceTraits<peripheral_id>;
        if constexpr (traits::kPresent &&
                      peripheral_class_is_gpio(traits::kPeripheralClassId)) {
            if (resolved == RuntimePeripheralId::none && traits::kInstance == target_instance) {
                resolved = peripheral_id;
            }
        }
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] consteval auto find_gpio_peripheral_id(device::PortId port_id) -> RuntimePeripheralId {
    return find_gpio_peripheral_id_impl(
        port_id,
        std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

[[nodiscard]] constexpr auto prefer_field(RuntimeFieldRef primary, RuntimeFieldRef fallback)
    -> RuntimeFieldRef {
    return primary.valid ? primary : fallback;
}

[[nodiscard]] constexpr auto synth_register_ref(std::uintptr_t base_address,
                                                std::uint32_t offset_bytes) -> RuntimeRegisterRef {
    if (base_address == 0u) {
        return rt::kInvalidRegisterRef;
    }
    return {
        .base_address = base_address,
        .offset_bytes = offset_bytes,
        .valid = true,
    };
}

[[nodiscard]] constexpr auto synth_field_ref(std::uintptr_t base_address,
                                             std::uint32_t register_offset_bytes,
                                             std::uint16_t bit_offset, std::uint16_t bit_width)
    -> RuntimeFieldRef {
    if (base_address == 0u || bit_width == 0u) {
        return rt::kInvalidFieldRef;
    }
    return {
        .reg = synth_register_ref(base_address, register_offset_bytes),
        .bit_offset = bit_offset,
        .bit_width = bit_width,
        .valid = true,
    };
}

[[nodiscard]] constexpr auto st_gpio_mode_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0 ? rt::kInvalidFieldRef
                          : synth_field_ref(base_address, 0x00u,
                                            static_cast<std::uint16_t>(line_index * 2), 2u);
}

[[nodiscard]] constexpr auto st_gpio_output_type_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x04u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto st_gpio_pull_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0 ? rt::kInvalidFieldRef
                          : synth_field_ref(base_address, 0x0Cu,
                                            static_cast<std::uint16_t>(line_index * 2), 2u);
}

[[nodiscard]] constexpr auto st_gpio_input_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x10u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto st_gpio_output_value_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x14u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto st_gpio_output_set_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x18u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto st_gpio_output_reset_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0 ? rt::kInvalidFieldRef
                          : synth_field_ref(base_address, 0x18u,
                                            static_cast<std::uint16_t>(16 + line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_enable_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x00u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_output_enable_field(std::uintptr_t base_address,
                                                               int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x10u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_output_disable_field(std::uintptr_t base_address,
                                                                int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x14u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_set_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x30u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_clear_field(std::uintptr_t base_address, int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x34u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_input_state_field(std::uintptr_t base_address,
                                                             int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x3Cu, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_drive_enable_field(std::uintptr_t base_address,
                                                              int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x50u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_drive_disable_field(std::uintptr_t base_address,
                                                               int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x54u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_pull_up_disable_field(std::uintptr_t base_address,
                                                                 int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x60u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_pull_up_enable_field(std::uintptr_t base_address,
                                                                int line_index) -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x64u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_pull_down_disable_field(std::uintptr_t base_address,
                                                                   int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x90u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_pull_down_enable_field(std::uintptr_t base_address,
                                                                  int line_index)
    -> RuntimeFieldRef {
    return line_index < 0
               ? rt::kInvalidFieldRef
               : synth_field_ref(base_address, 0x94u, static_cast<std::uint16_t>(line_index), 1u);
}

}  // namespace detail

template <typename Pin>
struct pin_handle {
    static constexpr bool available = device::SelectedRuntimeDescriptors::available;
    static_assert(requires { Pin::id; }, "GPIO pin_handle requires a typed pin with a PinId.");
    static constexpr auto pin_id = Pin::id;
    using pin_traits = device::PinTraits<pin_id>;
    static constexpr auto peripheral_id = [] {
        if constexpr (!available || pin_id == detail::RuntimePinId::none || !pin_traits::kPresent) {
            return detail::RuntimePeripheralId::none;
        }
        return detail::find_gpio_peripheral_id(pin_traits::kPortId);
    }();
    using peripheral_traits = device::PeripheralInstanceTraits<peripheral_id>;

    static constexpr auto peripheral_name =
        peripheral_id == detail::RuntimePeripheralId::none
            ? std::string_view{}
            : detail::selected_scoped_enum_name<peripheral_id>();
    static constexpr auto package_name = std::string_view{};
    static constexpr auto schema =
        peripheral_id == detail::RuntimePeripheralId::none
            ? hal::detail::runtime::GpioSchema::unknown
            : hal::detail::runtime::to_gpio_schema(peripheral_traits::kSchemaId);
    using semantic_traits = device::GpioSemanticTraits<pin_id>;
    static constexpr auto line_index =
        pin_traits::kPresent ? static_cast<int>(pin_traits::kPinNumber)
                             : (semantic_traits::kPresent ? static_cast<int>(semantic_traits::kLineIndex)
                                                          : -1);

    static constexpr auto direction_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::nxp_imxrt_gpio_v1) {
            return semantic_traits::kDirectionField;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto mode_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0
                       ? detail::st_gpio_mode_field(peripheral_traits::kBaseAddress, line_index)
                       : detail::rt::kInvalidFieldRef;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto output_type_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0 ? detail::st_gpio_output_type_field(
                                         peripheral_traits::kBaseAddress, line_index)
                                   : detail::rt::kInvalidFieldRef;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pull_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0
                       ? detail::st_gpio_pull_field(peripheral_traits::kBaseAddress, line_index)
                       : detail::rt::kInvalidFieldRef;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto input_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0
                       ? detail::st_gpio_input_field(peripheral_traits::kBaseAddress, line_index)
                       : detail::rt::kInvalidFieldRef;
        }
        if constexpr (schema == hal::detail::runtime::GpioSchema::nxp_imxrt_gpio_v1) {
            return semantic_traits::kInputField;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto output_value_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0 ? detail::st_gpio_output_value_field(
                                         peripheral_traits::kBaseAddress, line_index)
                                   : detail::rt::kInvalidFieldRef;
        }
        if constexpr (schema == hal::detail::runtime::GpioSchema::nxp_imxrt_gpio_v1) {
            return semantic_traits::kOutputValueField;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto output_set_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0 ? detail::st_gpio_output_set_field(
                                         peripheral_traits::kBaseAddress, line_index)
                                   : detail::rt::kInvalidFieldRef;
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto output_reset_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::st_gpio) {
            return line_index >= 0 ? detail::st_gpio_output_reset_field(
                                         peripheral_traits::kBaseAddress, line_index)
                                   : detail::rt::kInvalidFieldRef;
        }
        return detail::rt::kInvalidFieldRef;
    }();

    static constexpr auto pio_enable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(
                semantic_traits::kPioEnableField,
                detail::microchip_pio_enable_field(peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_output_enable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioOutputEnableField,
                                        detail::microchip_pio_output_enable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_output_disable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioOutputDisableField,
                                        detail::microchip_pio_output_disable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_drive_enable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioDriveEnableField,
                                        detail::microchip_pio_drive_enable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_drive_disable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioDriveDisableField,
                                        detail::microchip_pio_drive_disable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_pull_up_enable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioPullUpEnableField,
                                        detail::microchip_pio_pull_up_enable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_pull_up_disable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioPullUpDisableField,
                                        detail::microchip_pio_pull_up_disable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_pull_down_enable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioPullDownEnableField,
                                        detail::microchip_pio_pull_down_enable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_pull_down_disable_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioPullDownDisableField,
                                        detail::microchip_pio_pull_down_disable_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_set_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(
                semantic_traits::kPioSetField,
                detail::microchip_pio_set_field(peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_clear_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(
                semantic_traits::kPioClearField,
                detail::microchip_pio_clear_field(peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();
    static constexpr auto pio_input_state_field = [] {
        if constexpr (schema == hal::detail::runtime::GpioSchema::microchip_pio_v) {
            return detail::prefer_field(semantic_traits::kPioInputStateField,
                                        detail::microchip_pio_input_state_field(
                                            peripheral_traits::kBaseAddress, line_index));
        }
        return detail::rt::kInvalidFieldRef;
    }();

    static constexpr bool valid = [] {
        if constexpr (!available) {
            return false;
        }
        if ((schema != hal::detail::runtime::GpioSchema::st_gpio &&
             pin_id == detail::RuntimePinId::none) ||
            peripheral_id == detail::RuntimePeripheralId::none || !peripheral_traits::kPresent ||
            line_index < 0) {
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
            case hal::detail::runtime::GpioSchema::nxp_imxrt_gpio_v1:
                return direction_field.valid && input_field.valid && output_value_field.valid;
            default:
                return false;
        }
    }();

    [[nodiscard]] static consteval auto requirements() {
        detail::StringList<2> list{};
        if constexpr (!valid) {
            return list;
        }

        list.items[list.count++] = "package:selected";
        list.items[list.count++] = "runtime:descriptor-clock-binding";
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
        return peripheral_traits::kBaseAddress;
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

template <device::PinId PinIdValue>
[[nodiscard]] constexpr auto open(Config config = {}) -> pin_handle<pin<PinIdValue>> {
    return open<pin<PinIdValue>>(config);
}

// One-shot configure for pins that are not bound to a peripheral signal
// (reset lines, LEDs, strap inputs). Direction and initial state are required;
// drive and pull default to PushPull/None.
template <device::PinId PinIdValue>
[[nodiscard]] auto configure(Direction dir, State initial_state = State::Low,
                             Pull pull = Pull::None, Drive drive = Drive::PushPull)
    -> core::Result<void, core::ErrorCode> {
    static_assert(pin_handle<pin<PinIdValue>>::valid,
                  "Requested GPIO pin is not available for the selected device/package.");
    auto handle = open<PinIdValue>(Config{
        .direction   = dir,
        .drive       = drive,
        .pull        = pull,
        .initial_state = initial_state,
    });
    return handle.configure();
}

}  // namespace alloy::hal::gpio
