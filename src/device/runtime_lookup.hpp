#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "device/descriptors.hpp"
#include "device/traits.hpp"

namespace alloy::device::runtime {

[[nodiscard]] constexpr auto as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return as_string(lhs) == rhs;
}

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

[[nodiscard]] constexpr auto selected_device() -> std::string_view {
    return SelectedDeviceTraits::name;
}

template <typename Descriptor, std::size_t Extent>
[[nodiscard]] constexpr auto find_by_name(std::span<const Descriptor, Extent> descriptors,
                                          const char* Descriptor::*member,
                                          std::string_view value) -> const Descriptor* {
    for (const auto& descriptor : descriptors) {
        if (strings_equal(descriptor.*member, value)) {
            return &descriptor;
        }
    }
    return nullptr;
}

template <typename Descriptor, std::size_t Extent>
[[nodiscard]] constexpr auto find_device_scoped_by_name(
    std::span<const Descriptor, Extent> descriptors,
    const char* Descriptor::*device_member,
    const char* Descriptor::*value_member,
    std::string_view value)
    -> const Descriptor* {
    for (const auto& descriptor : descriptors) {
        if (!strings_equal(descriptor.*device_member, selected_device())) {
            continue;
        }
        if (strings_equal(descriptor.*value_member, value)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto find_runtime_profile(std::string_view subsystem,
                                                  std::string_view source_kind,
                                                  std::string_view source_id)
    -> const descriptors::family::RuntimeProfileDescriptor* {
    for (const auto& descriptor : descriptors::tables::runtime_profiles) {
        if (!strings_equal(descriptor.subsystem, subsystem)) {
            continue;
        }
        if (!strings_equal(descriptor.source_kind, source_kind)) {
            continue;
        }
        if (strings_equal(descriptor.source_id, source_id)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto find_pin(std::string_view pin_name)
    -> const descriptors::device_contract::PinDescriptor* {
    return find_by_name(descriptors::tables::pins, &descriptors::device_contract::PinDescriptor::pin_name,
                        pin_name);
}

[[nodiscard]] constexpr auto find_peripheral_instance(std::string_view peripheral_name)
    -> const descriptors::device_contract::PeripheralInstanceDescriptor* {
    return find_by_name(descriptors::tables::peripheral_instances,
                        &descriptors::device_contract::PeripheralInstanceDescriptor::name,
                        peripheral_name);
}

[[nodiscard]] constexpr auto find_register(std::string_view peripheral_name,
                                           std::string_view register_name)
    -> const descriptors::device_contract::RegisterDescriptor* {
    for (const auto& descriptor : descriptors::tables::registers) {
        if (!strings_equal(descriptor.peripheral_name, peripheral_name)) {
            continue;
        }
        if (strings_equal(descriptor.register_name, register_name)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto find_register_field(std::string_view peripheral_name,
                                                 std::string_view register_name,
                                                 std::string_view field_name)
    -> const descriptors::device_contract::RegisterFieldDescriptor* {
    for (const auto& descriptor : descriptors::tables::register_fields) {
        if (!strings_equal(descriptor.peripheral_name, peripheral_name)) {
            continue;
        }
        if (!strings_equal(descriptor.register_name, register_name)) {
            continue;
        }
        if (strings_equal(descriptor.field_name, field_name)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto runtime_field_suffix(std::string_view runtime_field_id)
    -> std::string_view {
    const auto separator = runtime_field_id.rfind(':');
    if (separator == std::string_view::npos || separator + 1u >= runtime_field_id.size()) {
        return {};
    }
    return runtime_field_id.substr(separator + 1u);
}

[[nodiscard]] constexpr auto find_register_field_by_runtime_id(
    std::string_view peripheral_name,
    std::string_view register_name,
    std::string_view runtime_field_id) -> const descriptors::device_contract::RegisterFieldDescriptor* {
    const auto field_suffix = runtime_field_suffix(runtime_field_id);
    if (field_suffix.empty()) {
        return nullptr;
    }

    for (const auto& descriptor : descriptors::tables::register_fields) {
        if (!strings_equal(descriptor.peripheral_name, peripheral_name)) {
            continue;
        }
        if (!strings_equal(descriptor.register_name, register_name)) {
            continue;
        }
        if (ascii_iequals(as_string(descriptor.field_name), field_suffix)) {
            return &descriptor;
        }
    }

    return nullptr;
}

[[nodiscard]] constexpr auto find_clock_binding(std::string_view peripheral_name)
    -> const descriptors::family::PeripheralClockBindingDescriptor* {
    return find_device_scoped_by_name(
        descriptors::tables::peripheral_clock_bindings,
        &descriptors::family::PeripheralClockBindingDescriptor::device,
        &descriptors::family::PeripheralClockBindingDescriptor::peripheral,
        peripheral_name);
}

[[nodiscard]] constexpr auto find_clock_gate(std::string_view gate_name)
    -> const descriptors::family::ClockGateDescriptor* {
    return find_device_scoped_by_name(descriptors::tables::clock_gates,
                                      &descriptors::family::ClockGateDescriptor::device,
                                      &descriptors::family::ClockGateDescriptor::gate_name,
                                      gate_name);
}

[[nodiscard]] constexpr auto find_reset(std::string_view reset_name)
    -> const descriptors::family::ResetDescriptor* {
    return find_device_scoped_by_name(descriptors::tables::resets,
                                      &descriptors::family::ResetDescriptor::device,
                                      &descriptors::family::ResetDescriptor::reset_name,
                                      reset_name);
}

[[nodiscard]] constexpr auto find_clock_selector(std::string_view selector_name)
    -> const descriptors::family::ClockSelectorDescriptor* {
    return find_device_scoped_by_name(descriptors::tables::clock_selectors,
                                      &descriptors::family::ClockSelectorDescriptor::device,
                                      &descriptors::family::ClockSelectorDescriptor::selector_name,
                                      selector_name);
}

[[nodiscard]] constexpr auto find_interrupt_bindings(std::string_view peripheral_name)
    -> std::span<const descriptors::device_contract::InterruptBindingDescriptor> {
    const auto* instance = find_peripheral_instance(peripheral_name);
    if (instance == nullptr || instance->interrupt_binding_count == 0u) {
        return {};
    }

    return descriptors::tables::interrupt_bindings.subspan(instance->interrupt_binding_offset,
                                                           instance->interrupt_binding_count);
}

[[nodiscard]] constexpr auto find_dma_bindings(std::string_view peripheral_name)
    -> std::span<const descriptors::device_contract::DmaBindingDescriptor> {
    const auto* instance = find_peripheral_instance(peripheral_name);
    if (instance == nullptr || instance->dma_binding_count == 0u) {
        return {};
    }

    return descriptors::tables::dma_bindings.subspan(instance->dma_binding_offset,
                                                     instance->dma_binding_count);
}

[[nodiscard]] constexpr auto find_capability_overlays(std::string_view peripheral_name)
    -> std::span<const descriptors::device_contract::CapabilityOverlayDescriptor> {
    const auto* instance = find_peripheral_instance(peripheral_name);
    if (instance == nullptr || instance->capability_overlay_count == 0u) {
        return {};
    }

    return descriptors::tables::capability_overlays.subspan(instance->capability_overlay_offset,
                                                            instance->capability_overlay_count);
}

[[nodiscard]] constexpr auto field_mask(std::uint16_t bit_offset,
                                        std::uint16_t bit_width) -> std::uint32_t {
    if (bit_width == 0u || bit_width >= 32u) {
        return bit_width == 32u ? 0xFFFF'FFFFu : 0u;
    }
    return ((1u << bit_width) - 1u) << bit_offset;
}

[[nodiscard]] constexpr auto field_mask(const descriptors::device_contract::RegisterFieldDescriptor& field)
    -> std::uint32_t {
    return field_mask(field.bit_offset, field.bit_width);
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

[[nodiscard]] constexpr auto parse_suffix_decimal(std::string_view text,
                                                  std::string_view marker) -> int {
    const auto position = text.find(marker);
    if (position == std::string_view::npos) {
        return -1;
    }
    return parse_decimal(text.substr(position + marker.size()));
}

[[nodiscard]] constexpr auto find_route_requirement(
    descriptors::family::RouteRequirementId requirement_id)
    -> const descriptors::family::RouteRequirementDescriptor* {
    for (const auto& descriptor : descriptors::tables::route_requirements) {
        if (descriptor.requirement_id == requirement_id) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] constexpr auto find_route_operation(
    descriptors::family::RouteOperationId operation_id)
    -> const descriptors::family::RouteOperationDescriptor* {
    for (const auto& descriptor : descriptors::tables::route_operations) {
        if (descriptor.operation_id == operation_id) {
            return &descriptor;
        }
    }
    return nullptr;
}

}  // namespace alloy::device::runtime
