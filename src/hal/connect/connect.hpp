#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>

#include "hal/connect/fixed_string.hpp"

#include "device/descriptors.hpp"
#include "device/runtime_lookup.hpp"
#include "device/traits.hpp"

namespace alloy::hal::connection {

template <FixedString Name>
struct peripheral {
    static constexpr auto name = std::string_view{Name};
};

template <FixedString Name>
struct pin {
    static constexpr auto name = std::string_view{Name};
};

template <FixedString Name>
struct signal {
    static constexpr auto name = std::string_view{Name};
};

template <typename Signal, typename Pin>
struct binding {
    using signal_type = Signal;
    using pin_type = Pin;
};

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

[[nodiscard]] consteval auto candidate_matches_selected_package(
    const device::descriptors::family::ConnectionCandidateDescriptor& candidate_descriptor)
    -> bool {
    const auto package = selected_package();
    if (package.empty()) {
        return false;
    }

    for (const auto& requirement_ref :
         device::descriptors::tables::candidate_requirement_refs.subspan(
             candidate_descriptor.requirement_offset, candidate_descriptor.requirement_count)) {
        if (requirement_ref.candidate_id != candidate_descriptor.candidate_id) {
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
            return true;
        }
    }

    return false;
}

[[nodiscard]] consteval auto find_requirement(std::string_view requirement_id)
    -> const device::descriptors::family::RouteRequirementDescriptor* {
    for (const auto& requirement : device::descriptors::tables::route_requirements) {
        if (strings_equal(requirement.device, selected_device()) &&
            strings_equal(requirement.requirement_name, requirement_id)) {
            return &requirement;
        }
    }

    return nullptr;
}

[[nodiscard]] consteval auto find_operation(std::string_view operation_id)
    -> const device::descriptors::family::RouteOperationDescriptor* {
    for (const auto& operation : device::descriptors::tables::route_operations) {
        if (strings_equal(operation.device, selected_device()) &&
            strings_equal(operation.operation_name, operation_id)) {
            return &operation;
        }
    }

    return nullptr;
}

[[nodiscard]] consteval auto find_group(std::string_view group_id)
    -> const device::descriptors::family::ConnectionGroupDescriptor* {
    for (const auto& group : device::descriptors::tables::connection_groups) {
        if (strings_equal(group.device, selected_device()) &&
            strings_equal(group.group_name, group_id)) {
            return &group;
        }
    }

    return nullptr;
}

[[nodiscard]] consteval auto find_candidate(std::string_view peripheral_name,
                                            std::string_view pin_name, std::string_view signal_name)
    -> const device::descriptors::family::ConnectionCandidateDescriptor* {
    for (const auto& candidate : device::descriptors::tables::connection_candidates) {
        if (!strings_equal(candidate.device, selected_device())) {
            continue;
        }
        if (!strings_equal(candidate.peripheral, peripheral_name)) {
            continue;
        }
        if (!strings_equal(candidate.pin, pin_name)) {
            continue;
        }
        if (!candidate_matches_selected_package(candidate)) {
            continue;
        }
        bool signal_match = false;
        for (const auto& capability :
             device::descriptors::tables::candidate_capability_refs.subspan(
                 candidate.capability_offset, candidate.capability_count)) {
            if (capability.candidate_id != candidate.candidate_id) {
                continue;
            }
            if (!capability.capability_id) {
                continue;
            }
            const auto capability_id = as_string(capability.capability_id);
            if (!capability_id.starts_with("capability:")) {
                continue;
            }
            const auto suffix = capability_id.substr(capability_id.rfind(':') + 1u);
            if (suffix == signal_name) {
                signal_match = true;
                break;
            }
        }
        if (!signal_match) {
            continue;
        }
        return &candidate;
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

template <typename... Bindings>
[[nodiscard]] consteval auto make_signal_array() {
    return std::array<std::string_view, sizeof...(Bindings)>{Bindings::signal_type::name...};
}

template <typename... Bindings>
[[nodiscard]] consteval auto resolve_candidates(std::string_view peripheral_name) {
    return std::array<const device::descriptors::family::ConnectionCandidateDescriptor*,
                      sizeof...(Bindings)>{
        find_candidate(peripheral_name, Bindings::pin_type::name, Bindings::signal_type::name)...};
}

template <std::size_t N>
[[nodiscard]] consteval auto all_candidates_found(
    const std::array<const device::descriptors::family::ConnectionCandidateDescriptor*, N>&
        candidates) -> bool {
    for (const auto* candidate : candidates) {
        if (candidate == nullptr) {
            return false;
        }
    }
    return true;
}

template <std::size_t N>
[[nodiscard]] consteval auto all_candidates_share_group(
    const std::array<const device::descriptors::family::ConnectionCandidateDescriptor*, N>&
        candidates,
    int expected_group_index) -> bool {
    for (const auto* candidate : candidates) {
        if (candidate == nullptr) {
            return false;
        }
        if (candidate->route_group_index != expected_group_index) {
            return false;
        }
    }
    return true;
}

template <std::size_t N>
[[nodiscard]] consteval auto group_matches_exact_signal_set(
    const device::descriptors::family::ConnectionGroupDescriptor& group,
    const std::array<std::string_view, N>& signals) -> bool {
    if (group.signal_count != N) {
        return false;
    }

    for (const auto& signal_ref : device::descriptors::tables::connection_group_signals.subspan(
             group.signal_offset, group.signal_count)) {
        bool present = false;
        for (const auto expected_signal : signals) {
            if (as_string(signal_ref.signal_name) == expected_signal) {
                present = true;
                break;
            }
        }
        if (!present) {
            return false;
        }
    }

    return true;
}

template <std::size_t N>
[[nodiscard]] consteval auto find_exact_group_index(std::string_view peripheral_name,
                                                    const std::array<std::string_view, N>& signals)
    -> int {
    const auto package = selected_package();
    auto index = 0;
    for (const auto& group : device::descriptors::tables::connection_groups) {
        if (!strings_equal(group.device, selected_device())) {
            ++index;
            continue;
        }
        if (!strings_equal(group.peripheral, peripheral_name)) {
            ++index;
            continue;
        }
        if (!strings_equal(group.package_name, package)) {
            ++index;
            continue;
        }
        if (!group_matches_exact_signal_set(group, signals)) {
            ++index;
            continue;
        }
        return index;
    }
    return -1;
}

}  // namespace detail

template <typename Peripheral, typename... Bindings>
struct connector {
    static_assert(sizeof...(Bindings) > 0, "connector requires at least one binding.");

    using peripheral_type = Peripheral;
    using binding_tuple = std::tuple<Bindings...>;

    static constexpr auto binding_count = sizeof...(Bindings);
    static constexpr auto available = device::SelectedDescriptors::available;
    static constexpr auto selected_device = detail::selected_device();
    static constexpr auto package_name = detail::selected_package();
    static constexpr auto signal_names = detail::make_signal_array<Bindings...>();
    static constexpr auto candidates = detail::resolve_candidates<Bindings...>(Peripheral::name);

    [[nodiscard]] static consteval auto compute_group_descriptor_index() -> int {
        if constexpr (binding_count == 1) {
            const auto* candidate = candidates[0];
            return candidate == nullptr ? -1 : candidate->route_group_index;
        }

        return detail::find_exact_group_index(Peripheral::name, signal_names);
    }
    static constexpr auto group_descriptor_index = compute_group_descriptor_index();

    [[nodiscard]] static consteval auto compute_group_descriptor()
        -> const device::descriptors::family::ConnectionGroupDescriptor* {
        if (group_descriptor_index < 0) {
            return static_cast<const device::descriptors::family::ConnectionGroupDescriptor*>(
                nullptr);
        }
        return &device::descriptors::tables::connection_groups[group_descriptor_index];
    }
    static constexpr auto group_descriptor = compute_group_descriptor();

    [[nodiscard]] static consteval auto compute_valid() -> bool {
        if constexpr (!available) {
            return false;
        }
        if (!detail::all_candidates_found(candidates)) {
            return false;
        }
        if constexpr (binding_count == 1) {
            return true;
        }
        if (group_descriptor == nullptr) {
            return false;
        }
        return detail::all_candidates_share_group(candidates, group_descriptor_index);
    }
    static constexpr auto valid = compute_valid();

    [[nodiscard]] static consteval auto has_group() -> bool { return group_descriptor != nullptr; }

    template <std::size_t Index>
    using binding_type = std::tuple_element_t<Index, binding_tuple>;

    [[nodiscard]] static consteval auto requirements() {
        detail::DescriptorList<device::descriptors::family::RouteRequirementDescriptor,
                               binding_count * 8>
            list{};

        for (const auto* candidate : candidates) {
            if (candidate == nullptr) {
                continue;
            }

            for (const auto& requirement :
                 device::descriptors::tables::candidate_requirement_refs.subspan(
                     candidate->requirement_offset, candidate->requirement_count)) {
                if (requirement.candidate_id != candidate->candidate_id) {
                    continue;
                }
                const auto* requirement_descriptor =
                    device::runtime::find_route_requirement(requirement.requirement_id);
                if (requirement_descriptor == nullptr) {
                    continue;
                }
                detail::append_unique(list, requirement_descriptor, [](const auto& descriptor) {
                    return descriptor.requirement_name;
                });
            }
        }

        return list;
    }

    [[nodiscard]] static consteval auto operations() {
        detail::DescriptorList<device::descriptors::family::RouteOperationDescriptor,
                               binding_count * 8>
            list{};

        for (const auto* candidate : candidates) {
            if (candidate == nullptr) {
                continue;
            }

            for (const auto& operation :
                 device::descriptors::tables::candidate_operation_refs.subspan(
                     candidate->operation_offset, candidate->operation_count)) {
                if (operation.candidate_id != candidate->candidate_id) {
                    continue;
                }
                const auto* operation_descriptor =
                    device::runtime::find_route_operation(operation.operation_id);
                if (operation_descriptor == nullptr) {
                    continue;
                }
                detail::append_unique(list, operation_descriptor, [](const auto& descriptor) {
                    return descriptor.operation_name;
                });
            }
        }

        return list;
    }
};

template <typename Peripheral, typename... Bindings>
[[nodiscard]] consteval auto resolve() -> connector<Peripheral, Bindings...> {
    using connector_type = connector<Peripheral, Bindings...>;
    static_assert(connector_type::valid,
                  "Requested connector has no valid descriptor-backed route for the selected "
                  "device/package.");
    return {};
}

}  // namespace alloy::hal::connection
