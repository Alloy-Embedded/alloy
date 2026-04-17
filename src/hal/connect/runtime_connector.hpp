#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "hal/detail/resolved_route.hpp"

#include "device/runtime.hpp"

namespace alloy::hal::connection {

namespace detail::runtime_connector_detail {

namespace route = alloy::hal::detail::route;

struct EmptyRequirements {
    [[nodiscard]] constexpr auto size() const -> std::size_t { return 0u; }
};

using RegisterRef = device::runtime::RuntimeRegisterRef;
using FieldRef = device::runtime::RuntimeFieldRef;

inline constexpr auto kInvalidRegisterRef = device::runtime::invalid_register_ref;
inline constexpr auto kInvalidFieldRef = device::runtime::invalid_field_ref;

template <device::runtime::RegisterId Id>
[[nodiscard]] consteval auto register_ref() -> RegisterRef {
    using traits = device::runtime::RegisterTraits<Id>;
    if constexpr (!traits::kPresent) {
        return kInvalidRegisterRef;
    } else {
        return RegisterRef{Id, traits::kBaseAddress, traits::kOffsetBytes, true};
    }
}

template <device::runtime::FieldId Id>
[[nodiscard]] consteval auto field_ref() -> FieldRef {
    using traits = device::runtime::RegisterFieldTraits<Id>;
    if constexpr (!traits::kPresent) {
        return kInvalidFieldRef;
    } else {
        return FieldRef{Id, register_ref<traits::kRegisterId>(), traits::kBitOffset,
                        traits::kBitWidth, true};
    }
}

enum class GpioSchema : std::uint8_t {
    unknown,
    st_gpio,
    microchip_pio_v,
};

template <typename SchemaId>
[[nodiscard]] consteval auto to_gpio_schema(SchemaId schema_id) -> GpioSchema {
    if constexpr (requires { SchemaId::schema_alloy_gpio_st_gpio; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_st_gpio) {
            return GpioSchema::st_gpio;
        }
    }
    if constexpr (requires { SchemaId::schema_alloy_gpio_microchip_pio_v; }) {
        if (schema_id == SchemaId::schema_alloy_gpio_microchip_pio_v) {
            return GpioSchema::microchip_pio_v;
        }
    }
    return GpioSchema::unknown;
}

struct RuntimePinLocation {
    device::runtime::PortId port_id = device::runtime::PortId::none;
    int pin_number = -1;
    bool valid = false;
};

template <std::size_t... Index>
[[nodiscard]] consteval auto find_runtime_pin_location_impl(device::runtime::PinId pin_id,
                                                            std::index_sequence<Index...>)
    -> RuntimePinLocation {
    auto resolved = RuntimePinLocation{};

    auto match = [&]<std::size_t I>() consteval {
        constexpr auto candidate = device::runtime::runtime_pin_ids[I];
        using traits = device::runtime::PinTraits<candidate>;
        if (resolved.valid || candidate != pin_id || !traits::kPresent) {
            return;
        }

        resolved = {
            .port_id = traits::kPortId,
            .pin_number = traits::kPinNumber,
            .valid = true,
        };
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] consteval auto find_runtime_pin_location(device::runtime::PinId pin_id)
    -> RuntimePinLocation {
    return find_runtime_pin_location_impl(
        pin_id, std::make_index_sequence<std::size(device::runtime::runtime_pin_ids)>{});
}

[[nodiscard]] consteval auto port_instance(device::runtime::PortId port_id) -> int {
    if (port_id == device::runtime::PortId::none) {
        return -1;
    }
    return static_cast<int>(port_id) - 1;
}

template <std::size_t... Index>
[[nodiscard]] consteval auto find_gpio_peripheral_id_impl(device::runtime::PortId port_id,
                                                          std::index_sequence<Index...>)
    -> device::runtime::PeripheralId {
    const auto target_instance = port_instance(port_id);
    auto resolved = device::runtime::PeripheralId::none;

    auto match = [&]<std::size_t I>() consteval {
        constexpr auto peripheral_id = device::runtime::runtime_peripheral_ids[I];
        using traits = device::runtime::PeripheralInstanceTraits<peripheral_id>;
        if constexpr (traits::kPresent && traits::kPeripheralClassId ==
                                              device::runtime::PeripheralClassId::class_gpio) {
            if (resolved == device::runtime::PeripheralId::none &&
                traits::kInstance == target_instance) {
                resolved = peripheral_id;
            }
        }
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] consteval auto find_gpio_peripheral_id(device::runtime::PortId port_id)
    -> device::runtime::PeripheralId {
    return find_gpio_peripheral_id_impl(
        port_id, std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

template <std::size_t... Index>
[[nodiscard]] consteval auto find_runtime_peripheral_base_address_impl(
    device::runtime::PeripheralId peripheral_id, std::index_sequence<Index...>) -> std::uintptr_t {
    auto resolved = std::uintptr_t{0};

    auto match = [&]<std::size_t I>() consteval {
        constexpr auto candidate = device::runtime::runtime_peripheral_ids[I];
        using traits = device::runtime::PeripheralInstanceTraits<candidate>;
        if (resolved != 0u || candidate != peripheral_id || !traits::kPresent) {
            return;
        }
        resolved = traits::kBaseAddress;
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] consteval auto runtime_peripheral_base_address(
    device::runtime::PeripheralId peripheral_id) -> std::uintptr_t {
    return find_runtime_peripheral_base_address_impl(
        peripheral_id,
        std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

template <std::size_t... Index>
[[nodiscard]] consteval auto find_gpio_schema_impl(device::runtime::PeripheralId peripheral_id,
                                                   std::index_sequence<Index...>) -> GpioSchema {
    auto resolved = GpioSchema::unknown;

    auto match = [&]<std::size_t I>() consteval {
        constexpr auto candidate = device::runtime::runtime_peripheral_ids[I];
        using traits = device::runtime::PeripheralInstanceTraits<candidate>;
        if (resolved != GpioSchema::unknown || candidate != peripheral_id || !traits::kPresent) {
            return;
        }
        resolved = to_gpio_schema(traits::kSchemaId);
    };

    (match.template operator()<Index>(), ...);
    return resolved;
}

[[nodiscard]] consteval auto gpio_schema(device::runtime::PeripheralId peripheral_id)
    -> GpioSchema {
    return find_gpio_schema_impl(
        peripheral_id,
        std::make_index_sequence<std::size(device::runtime::runtime_peripheral_ids)>{});
}

template <std::size_t Begin, std::size_t End>
[[nodiscard]] consteval auto find_runtime_field_ref_impl(device::runtime::FieldId field_id)
    -> FieldRef {
    if constexpr (Begin >= End) {
        return kInvalidFieldRef;
    } else if constexpr ((End - Begin) == 1u) {
        constexpr auto candidate = device::runtime::runtime_field_ids[Begin];
        if constexpr (candidate == device::runtime::FieldId::none) {
            return kInvalidFieldRef;
        } else if (candidate == field_id) {
            return field_ref<candidate>();
        } else {
            return kInvalidFieldRef;
        }
    } else {
        constexpr auto mid = Begin + ((End - Begin) / 2u);
        const auto left = find_runtime_field_ref_impl<Begin, mid>(field_id);
        if (left.valid) {
            return left;
        }
        return find_runtime_field_ref_impl<mid, End>(field_id);
    }
}

[[nodiscard]] consteval auto find_runtime_field_ref(device::runtime::FieldId field_id) -> FieldRef {
    return find_runtime_field_ref_impl<0u, std::size(device::runtime::runtime_field_ids)>(field_id);
}

[[nodiscard]] constexpr auto synth_register_ref(std::uintptr_t base_address,
                                                std::uint32_t offset_bytes) -> RegisterRef {
    if (base_address == 0u) {
        return kInvalidRegisterRef;
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
    -> FieldRef {
    if (base_address == 0u || bit_width == 0u) {
        return kInvalidFieldRef;
    }
    return {
        .reg = synth_register_ref(base_address, register_offset_bytes),
        .bit_offset = bit_offset,
        .bit_width = bit_width,
        .valid = true,
    };
}

[[nodiscard]] constexpr auto st_gpio_mode_field(std::uintptr_t base_address, int line_index)
    -> FieldRef {
    return line_index < 0 ? kInvalidFieldRef
                          : synth_field_ref(base_address, 0x00u,
                                            static_cast<std::uint16_t>(line_index * 2), 2u);
}

[[nodiscard]] constexpr auto st_gpio_af_field(std::uintptr_t base_address, int line_index)
    -> FieldRef {
    if (line_index < 0) {
        return kInvalidFieldRef;
    }
    const auto afr_offset = line_index < 8 ? 0x20u : 0x24u;
    return synth_field_ref(base_address, afr_offset,
                           static_cast<std::uint16_t>((line_index % 8) * 4), 4u);
}

[[nodiscard]] constexpr auto microchip_pio_pdr_field(std::uintptr_t base_address, int line_index)
    -> FieldRef {
    return line_index < 0
               ? kInvalidFieldRef
               : synth_field_ref(base_address, 0x04u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] constexpr auto microchip_pio_abcdsr_field(std::uintptr_t base_address, int line_index)
    -> FieldRef {
    return line_index < 0
               ? kInvalidFieldRef
               : synth_field_ref(base_address, 0x70u, static_cast<std::uint16_t>(line_index), 1u);
}

[[nodiscard]] consteval auto make_stm32_pinmux_operation(device::runtime::PinId pin_id,
                                                         std::uint32_t selector)
    -> route::Operation {
    const auto pin = find_runtime_pin_location(pin_id);
    if (!pin.valid) {
        return {};
    }

    const auto gpio_peripheral_id = find_gpio_peripheral_id(pin.port_id);
    if (gpio_peripheral_id == device::runtime::PeripheralId::none) {
        return {};
    }

    if (gpio_schema(gpio_peripheral_id) != GpioSchema::st_gpio) {
        return {};
    }

    const auto base_address = runtime_peripheral_base_address(gpio_peripheral_id);
    const auto mode_field = st_gpio_mode_field(base_address, pin.pin_number);
    const auto af_field = st_gpio_af_field(base_address, pin.pin_number);
    if (!mode_field.valid || !af_field.valid) {
        return {};
    }

    return {
        .kind = route::OperationKind::configure_stm32_af,
        .primary = mode_field,
        .secondary = af_field,
        .peripheral_id = gpio_peripheral_id,
        .value = selector,
    };
}

[[nodiscard]] consteval auto make_same70_pinmux_operation(device::runtime::PinId pin_id,
                                                          std::uint32_t selector)
    -> route::Operation {
    const auto pin = find_runtime_pin_location(pin_id);
    if (!pin.valid) {
        return {};
    }

    const auto gpio_peripheral_id = find_gpio_peripheral_id(pin.port_id);
    if (gpio_peripheral_id == device::runtime::PeripheralId::none) {
        return {};
    }

    if (gpio_schema(gpio_peripheral_id) != GpioSchema::microchip_pio_v) {
        return {};
    }

    const auto base_address = runtime_peripheral_base_address(gpio_peripheral_id);
    const auto pdr_field = microchip_pio_pdr_field(base_address, pin.pin_number);
    const auto abcdsr_field = microchip_pio_abcdsr_field(base_address, pin.pin_number);
    if (!pdr_field.valid || !abcdsr_field.valid) {
        return {};
    }

    return {
        .kind = route::OperationKind::configure_same70_pio,
        .primary = pdr_field,
        .secondary = abcdsr_field,
        .peripheral_id = gpio_peripheral_id,
        .value = selector,
    };
}

template <typename OperationKind>
[[nodiscard]] consteval auto is_set_bit_operation(OperationKind kind) -> bool {
    if constexpr (requires { OperationKind::operation_kind_set_bit; }) {
        return kind == OperationKind::operation_kind_set_bit;
    } else {
        return false;
    }
}

template <typename OperationKind>
[[nodiscard]] consteval auto is_clear_bit_operation(OperationKind kind) -> bool {
    if constexpr (requires { OperationKind::operation_kind_clear_bit; }) {
        return kind == OperationKind::operation_kind_clear_bit;
    } else {
        return false;
    }
}

[[nodiscard]] consteval auto resolve_runtime_operation(
    const device::runtime::RouteOperation& descriptor) -> route::Operation {
    using resolved_kind = route::OperationKind;

    if (descriptor.kind_id == device::runtime::OperationKindId::operation_kind_write_selector) {
        if (descriptor.pin_id == device::runtime::PinId::none || descriptor.value_int < 0) {
            return {};
        }

        if (const auto stm32 = make_stm32_pinmux_operation(
                descriptor.pin_id, static_cast<std::uint32_t>(descriptor.value_int));
            stm32.kind != resolved_kind::invalid) {
            return stm32;
        }
        if (const auto same70 = make_same70_pinmux_operation(
                descriptor.pin_id, static_cast<std::uint32_t>(descriptor.value_int));
            same70.kind != resolved_kind::invalid) {
            return same70;
        }
        return {};
    }

    if (!is_set_bit_operation(descriptor.kind_id) && !is_clear_bit_operation(descriptor.kind_id)) {
        return {};
    }

    if (descriptor.field_id == device::runtime::FieldId::none) {
        return {};
    }

    const auto field = find_runtime_field_ref(descriptor.field_id);
    if (!field.valid) {
        return {};
    }

    return {
        .kind = is_set_bit_operation(descriptor.kind_id) ? resolved_kind::set_field
                                                         : resolved_kind::clear_field,
        .primary = field,
    };
}

template <typename Binding, device::runtime::PinId PinIdValue,
          device::runtime::SignalId SignalIdValue>
struct runtime_binding {
    using binding_type = Binding;
    using signal_type = typename Binding::signal_type;
    using pin_type = typename Binding::pin_type;
    static constexpr auto pin_id = PinIdValue;
    static constexpr auto signal_id = SignalIdValue;
};

template <std::size_t Capacity>
consteval void append_operation(route::List<route::Operation, Capacity>& list,
                                const route::Operation& operation) {
    route::append_unique(list, operation);
}

template <typename Peripheral, device::runtime::PeripheralId PeripheralIdValue,
          typename... Bindings>
struct runtime_connector {
    using peripheral_type = Peripheral;
    using binding_tuple = std::tuple<typename Bindings::binding_type...>;

    static constexpr auto peripheral_id = PeripheralIdValue;
    static constexpr auto binding_count = sizeof...(Bindings);
    static constexpr auto package_name = std::string_view{};
    static constexpr auto valid =
        ((Bindings::pin_id != device::runtime::PinId::none &&
          Bindings::signal_id != device::runtime::SignalId::none) &&
         ...) &&
        ((device::runtime::RouteTraits<Bindings::pin_id, PeripheralIdValue,
                                       Bindings::signal_id>::kPresent) &&
         ...);

    template <std::size_t Index>
    using binding_type = std::tuple_element_t<Index, binding_tuple>;

    [[nodiscard]] static consteval auto requirements() -> EmptyRequirements { return {}; }

    template <typename Binding, std::size_t Capacity>
    static consteval void append_binding_operations(route::List<route::Operation, Capacity>& list) {
        using route_traits =
            device::runtime::RouteTraits<Binding::pin_id, PeripheralIdValue, Binding::signal_id>;
        static_assert(route_traits::kPresent,
                      "Runtime connector requires typed route traits for every binding.");

        for (const auto& operation : route_traits::kOperations) {
            append_operation(list, resolve_runtime_operation(operation));
        }
    }

    [[nodiscard]] static consteval auto operations() {
        route::List<route::Operation, binding_count * 8> list{};
        (append_binding_operations<Bindings>(list), ...);
        return list;
    }
};

}  // namespace detail::runtime_connector_detail

template <typename Binding, device::runtime::PinId PinIdValue,
          device::runtime::SignalId SignalIdValue>
using runtime_binding =
    detail::runtime_connector_detail::runtime_binding<Binding, PinIdValue, SignalIdValue>;

template <typename Peripheral, device::runtime::PeripheralId PeripheralIdValue,
          typename... Bindings>
using runtime_connector =
    detail::runtime_connector_detail::runtime_connector<Peripheral, PeripheralIdValue, Bindings...>;

}  // namespace alloy::hal::connection
