#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "device/runtime.hpp"

namespace alloy::hal::detail::route {

enum class OperationKind : std::uint8_t {
    invalid,
    set_field,
    clear_field,
    apply_clock_gate_by_index,
    release_reset_by_index,
    configure_stm32_af,
    configure_same70_pio,
};

struct Operation {
    OperationKind kind = OperationKind::invalid;
    device::runtime::RuntimeFieldRef primary{};
    device::runtime::RuntimeFieldRef secondary{};
    device::runtime::PeripheralId peripheral_id = device::runtime::PeripheralId::none;
    std::uint16_t lookup_index = 0u;
    std::uint32_t value = 0u;
};

template <typename Value, std::size_t Capacity>
struct List {
    std::array<Value, Capacity> items{};
    std::size_t count = 0u;

    [[nodiscard]] constexpr auto size() const -> std::size_t { return count; }
    [[nodiscard]] constexpr auto empty() const -> bool { return count == 0u; }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const -> const Value& {
        return items[index];
    }
};

[[nodiscard]] constexpr auto equal_field(device::runtime::RuntimeFieldRef lhs,
                                         device::runtime::RuntimeFieldRef rhs) -> bool {
    return lhs.valid == rhs.valid && lhs.reg.base_address == rhs.reg.base_address &&
           lhs.reg.offset_bytes == rhs.reg.offset_bytes && lhs.bit_offset == rhs.bit_offset &&
           lhs.bit_width == rhs.bit_width;
}

[[nodiscard]] constexpr auto equal_operation(const Operation& lhs, const Operation& rhs) -> bool {
    return lhs.kind == rhs.kind && equal_field(lhs.primary, rhs.primary) &&
           equal_field(lhs.secondary, rhs.secondary) && lhs.peripheral_id == rhs.peripheral_id &&
           lhs.lookup_index == rhs.lookup_index && lhs.value == rhs.value;
}

template <std::size_t Capacity>
consteval void append_unique(List<Operation, Capacity>& list, const Operation& operation) {
    if (operation.kind == OperationKind::invalid) {
        return;
    }

    for (std::size_t index = 0; index < list.count; ++index) {
        if (equal_operation(list.items[index], operation)) {
            return;
        }
    }

    list.items[list.count++] = operation;
}

}  // namespace alloy::hal::detail::route
