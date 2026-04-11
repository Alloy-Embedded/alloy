#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "hal/gpio/detail/backend.hpp"
#include "hal/types.hpp"

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "device/descriptors.hpp"
#include "device/runtime_lookup.hpp"
#include "device/traits.hpp"

namespace alloy::hal::gpio {

using Config = GpioConfig;
using Direction = PinDirection;
using State = PinState;
using Pull = PinPull;
using Drive = PinDrive;

namespace detail {

template <typename Descriptor, std::size_t Capacity>
struct DescriptorList {
    std::array<const Descriptor*, Capacity> items{};
    std::size_t count = 0;

    [[nodiscard]] constexpr auto size() const -> std::size_t { return count; }
    [[nodiscard]] constexpr auto empty() const -> bool { return count == 0; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const Descriptor* {
        return items[index];
    }
    [[nodiscard]] constexpr auto begin() const { return items.begin(); }
    [[nodiscard]] constexpr auto end() const { return items.begin() + count; }
};

[[nodiscard]] constexpr auto as_string(const char* text) -> std::string_view {
    return text == nullptr ? std::string_view{} : std::string_view{text};
}

[[nodiscard]] constexpr auto strings_equal(const char* lhs, std::string_view rhs) -> bool {
    return as_string(lhs) == rhs;
}

[[nodiscard]] consteval auto selected_device() -> std::string_view {
    return device::SelectedDeviceTraits::name;
}

[[nodiscard]] consteval auto selected_package() -> std::string_view {
    if constexpr (!device::SelectedDescriptors::available) {
        return {};
    }

    for (const auto& descriptor : device::descriptors::tables::package_map) {
        if (strings_equal(descriptor.device, selected_device())) {
            return as_string(descriptor.package_name);
        }
    }

    return {};
}

[[nodiscard]] consteval auto is_general_gpio_role(std::string_view role) -> bool {
    if (role.size() < 3) {
        return false;
    }

    const auto has_numeric_suffix = [&]() consteval {
        bool found_digit = false;
        for (const char ch : role) {
            if (ch >= '0' && ch <= '9') {
                found_digit = true;
                continue;
            }
            if (found_digit) {
                return false;
            }
        }
        return found_digit;
    }();

    if (!has_numeric_suffix) {
        return false;
    }

    return role.starts_with("in") || role.starts_with("IN") || role.starts_with("io") ||
           role.starts_with("IO");
}

[[nodiscard]] consteval auto candidate_is_general_gpio(
    const device::descriptors::family::ConnectionCandidateDescriptor& candidate) -> bool {
    bool has_gpio_capability = false;
    for (const auto& capability : device::descriptors::tables::candidate_capability_refs.subspan(
             candidate.capability_offset, candidate.capability_count)) {
        const auto capability_id = as_string(capability.capability_id);
        if (capability_id.starts_with("capability:gpio:")) {
            has_gpio_capability = true;
            break;
        }
    }

    if (!has_gpio_capability) {
        return false;
    }

    return is_general_gpio_role(as_string(candidate.signal));
}

[[nodiscard]] consteval auto is_gpio_peripheral_name(std::string_view name) -> bool {
    if (!name.starts_with("GPIO")) {
        return false;
    }

    if (name.size() == 5) {
        const char suffix = name[4];
        return suffix >= 'A' && suffix <= 'Z';
    }

    for (std::size_t index = 4; index < name.size(); ++index) {
        const char ch = name[index];
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    return name.size() > 4;
}

[[nodiscard]] consteval auto find_gpio_candidate(std::string_view pin_name)
    -> const device::descriptors::family::ConnectionCandidateDescriptor* {
    const auto package = selected_package();

    for (const auto& candidate : device::descriptors::tables::connection_candidates) {
        if (!strings_equal(candidate.device, selected_device())) {
            continue;
        }
        if (!strings_equal(candidate.pin, pin_name)) {
            continue;
        }
        if (!is_gpio_peripheral_name(as_string(candidate.peripheral))) {
            continue;
        }
        if (!candidate_is_general_gpio(candidate)) {
            continue;
        }
        if (!package.empty()) {
            bool package_match = false;
            for (const auto& requirement_ref :
                 device::descriptors::tables::candidate_requirement_refs.subspan(
                     candidate.requirement_offset, candidate.requirement_count)) {
                if (requirement_ref.candidate_id != candidate.candidate_id) {
                    continue;
                }
                const auto* requirement =
                    device::runtime::find_route_requirement(requirement_ref.requirement_id);
                if (requirement == nullptr) {
                    continue;
                }
                if (!strings_equal(requirement->device, selected_device())) {
                    continue;
                }
                if (!strings_equal(requirement->kind, "package")) {
                    continue;
                }
                if (strings_equal(requirement->target_ref_id, package)) {
                    package_match = true;
                }
            }
            if (!package_match) {
                continue;
            }
        }
        return &candidate;
    }

    return nullptr;
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

[[nodiscard]] consteval auto derived_gpio_peripheral(std::string_view pin_name)
    -> std::array<char, 16> {
    std::array<char, 16> buffer{};
    if (pin_name.size() < 3 || pin_name[0] != 'P') {
        return buffer;
    }

    const char port = pin_name[1];
    if (port < 'A' || port > 'Z') {
        return buffer;
    }

    buffer[0] = 'G';
    buffer[1] = 'P';
    buffer[2] = 'I';
    buffer[3] = 'O';
    buffer[4] = port;
    buffer[5] = '\0';
    return buffer;
}

[[nodiscard]] consteval auto find_package_pad(std::string_view pin_name)
    -> const device::descriptors::family::PackagePadDescriptor* {
    const auto package = selected_package();
    for (const auto& pad : device::descriptors::tables::package_pads) {
        if (!strings_equal(pad.device, selected_device())) {
            continue;
        }
        if (!strings_equal(pad.package_name, package)) {
            continue;
        }
        if (!strings_equal(pad.bonded_pin, pin_name)) {
            continue;
        }
        return &pad;
    }
    return nullptr;
}

[[nodiscard]] consteval auto find_rcc_descriptor(std::string_view peripheral_name)
    -> const device::descriptors::family::RccDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::rcc_map) {
        if (strings_equal(descriptor.peripheral, peripheral_name)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] consteval auto find_requirement_by_identity(std::string_view kind,
                                                          std::string_view target,
                                                          std::string_view value = {})
    -> const device::descriptors::family::RouteRequirementDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::route_requirements) {
        if (!strings_equal(descriptor.device, selected_device())) {
            continue;
        }
        if (!strings_equal(descriptor.kind, kind)) {
            continue;
        }
        if (kind == "package") {
            if (strings_equal(descriptor.target_ref_id, target)) {
                return &descriptor;
            }
            continue;
        }
        if (kind == "bonded-pin") {
            if (strings_equal(descriptor.target_ref_id, target) &&
                strings_equal(descriptor.value_ref_id, value)) {
                return &descriptor;
            }
            continue;
        }
        if (strings_equal(descriptor.diagnostic_target, target) &&
            (value.empty() || strings_equal(descriptor.diagnostic_value, value))) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] consteval auto find_operation_by_identity(std::string_view kind,
                                                        std::string_view target)
    -> const device::descriptors::family::RouteOperationDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::route_operations) {
        if (!strings_equal(descriptor.device, selected_device())) {
            continue;
        }
        if (!strings_equal(descriptor.kind, kind)) {
            continue;
        }
        if (strings_equal(descriptor.diagnostic_target, target)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] consteval auto find_requirement(std::string_view requirement_id)
    -> const device::descriptors::family::RouteRequirementDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::route_requirements) {
        if (strings_equal(descriptor.device, selected_device()) &&
            strings_equal(descriptor.requirement_name, requirement_id)) {
            return &descriptor;
        }
    }
    return nullptr;
}

[[nodiscard]] consteval auto find_operation(std::string_view operation_id)
    -> const device::descriptors::family::RouteOperationDescriptor* {
    for (const auto& descriptor : device::descriptors::tables::route_operations) {
        if (strings_equal(descriptor.device, selected_device()) &&
            strings_equal(descriptor.operation_name, operation_id)) {
            return &descriptor;
        }
    }
    return nullptr;
}

template <typename Descriptor, std::size_t Capacity, typename Accessor>
consteval void append_unique(DescriptorList<Descriptor, Capacity>& list,
                             const Descriptor* descriptor, Accessor accessor) {
    if (descriptor == nullptr) {
        return;
    }

    const auto descriptor_id = as_string(accessor(*descriptor));
    for (std::size_t index = 0; index < list.count; ++index) {
        if (as_string(accessor(*list.items[index])) == descriptor_id) {
            return;
        }
    }

    list.items[list.count++] = descriptor;
}

[[nodiscard]] consteval auto find_peripheral_base(std::string_view peripheral_name)
    -> const device::descriptors::startup::PeripheralBase* {
    for (const auto& descriptor : device::descriptors::tables::peripheral_bases) {
        if (strings_equal(descriptor.name, peripheral_name)) {
            return &descriptor;
        }
    }
    return nullptr;
}

}  // namespace detail

template <typename Pin>
struct pin_handle {
    static constexpr bool available = device::SelectedDescriptors::available;
    static constexpr auto selected_device = detail::selected_device();
    static constexpr auto package_name = detail::selected_package();
    static constexpr auto package_pad = detail::find_package_pad(Pin::name);
    static constexpr auto gpio_candidate = detail::find_gpio_candidate(Pin::name);
    static constexpr auto derived_peripheral_name_storage =
        detail::derived_gpio_peripheral(Pin::name);

    static constexpr auto peripheral_name = []() consteval -> std::string_view {
        if (gpio_candidate != nullptr) {
            return detail::as_string(gpio_candidate->peripheral);
        }

        return detail::as_string(derived_peripheral_name_storage.data());
    }();

    static constexpr auto peripheral_base = detail::find_peripheral_base(peripheral_name);
    static constexpr auto rcc_descriptor = detail::find_rcc_descriptor(peripheral_name);

    static constexpr auto line_index = []() consteval -> int {
        if (gpio_candidate != nullptr) {
            const auto index =
                detail::parse_trailing_number(detail::as_string(gpio_candidate->signal));
            if (index >= 0) {
                return index;
            }
        }

        return detail::parse_trailing_number(Pin::name);
    }();

    static constexpr bool valid = available && package_pad != nullptr && !package_name.empty() &&
                                  !peripheral_name.empty() && peripheral_base != nullptr &&
                                  line_index >= 0;

    constexpr explicit pin_handle(Config config = {}) : config_(config) {}

    [[nodiscard]] constexpr auto config() const -> const Config& { return config_; }

    [[nodiscard]] static consteval auto requirements() {
        detail::DescriptorList<device::descriptors::family::RouteRequirementDescriptor, 8> list{};

        if constexpr (!valid) {
            return list;
        }

        if (gpio_candidate != nullptr) {
            for (const auto& requirement_ref :
                 device::descriptors::tables::candidate_requirement_refs.subspan(
                     gpio_candidate->requirement_offset, gpio_candidate->requirement_count)) {
                if (requirement_ref.candidate_id != gpio_candidate->candidate_id) {
                    continue;
                }
                const auto* requirement =
                    device::runtime::find_route_requirement(requirement_ref.requirement_id);
                if (requirement == nullptr) {
                    continue;
                }
                detail::append_unique(list, requirement, [](const auto& descriptor) {
                    return descriptor.requirement_name;
                });
            }
            return list;
        }

        detail::append_unique(
            list, detail::find_requirement_by_identity("package", package_name, "selected"),
            [](const auto& descriptor) { return descriptor.requirement_name; });
        detail::append_unique(
            list, detail::find_requirement_by_identity("bonded-pin", Pin::name, package_name),
            [](const auto& descriptor) { return descriptor.requirement_name; });
        if (rcc_descriptor != nullptr) {
            detail::append_unique(
                list,
                detail::find_requirement_by_identity(
                    "clock-enable", detail::as_string(rcc_descriptor->enable_signal)),
                [](const auto& descriptor) { return descriptor.requirement_name; });

            const auto reset_signal = detail::as_string(rcc_descriptor->reset_signal);
            if (!reset_signal.empty()) {
                detail::append_unique(
                    list, detail::find_requirement_by_identity("reset-release", reset_signal),
                    [](const auto& descriptor) { return descriptor.requirement_name; });
            }
        }

        return list;
    }

    [[nodiscard]] static consteval auto operations() {
        detail::DescriptorList<device::descriptors::family::RouteOperationDescriptor, 8> list{};

        if constexpr (!valid) {
            return list;
        }

        if (gpio_candidate != nullptr) {
            for (const auto& operation_ref :
                 device::descriptors::tables::candidate_operation_refs.subspan(
                     gpio_candidate->operation_offset, gpio_candidate->operation_count)) {
                if (operation_ref.candidate_id != gpio_candidate->candidate_id) {
                    continue;
                }
                const auto* operation =
                    device::runtime::find_route_operation(operation_ref.operation_id);
                if (operation == nullptr) {
                    continue;
                }
                detail::append_unique(list, operation, [](const auto& descriptor) {
                    return descriptor.operation_name;
                });
            }
            return list;
        }

        if (rcc_descriptor != nullptr) {
            detail::append_unique(list,
                                  detail::find_operation_by_identity(
                                      "set-bit", detail::as_string(rcc_descriptor->enable_signal)),
                                  [](const auto& descriptor) { return descriptor.operation_name; });

            const auto reset_signal = detail::as_string(rcc_descriptor->reset_signal);
            if (!reset_signal.empty()) {
                detail::append_unique(
                    list, detail::find_operation_by_identity("clear-bit", reset_signal),
                    [](const auto& descriptor) { return descriptor.operation_name; });
            }
        }

        return list;
    }

    [[nodiscard]] static constexpr auto pin_name() -> std::string_view { return Pin::name; }

    [[nodiscard]] static constexpr auto base_address() -> std::uintptr_t {
        return peripheral_base == nullptr ? 0u : peripheral_base->address;
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
