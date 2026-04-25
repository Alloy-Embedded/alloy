#pragma once

// registry.hpp: flat array of type-erased variable descriptors.
//
// The slave looks up vars by Modbus register address at runtime. The registry
// provides O(N) linear scan (N is typically small, ≤ 64 vars on embedded).
//
// Footprint check: compile-time static_assert that the registry struct fits
// within ALLOY_MODBUS_REGISTRY_MAX_BYTES (default 8192). Override by defining
// that macro before including this header or via CMake compile definitions.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "alloy/modbus/var.hpp"

#ifndef ALLOY_MODBUS_REGISTRY_MAX_BYTES
#define ALLOY_MODBUS_REGISTRY_MAX_BYTES 8192u
#endif

namespace alloy::modbus {

// ============================================================================
// Type-erased variable descriptor
// ============================================================================

struct VarDescriptor {
    std::uint16_t    address;     // first Modbus register
    std::uint8_t     reg_count;   // number of 16-bit registers
    Access           access;
    WordOrder        word_order;
    std::string_view name;

    // True if the given Modbus register address falls within this variable.
    [[nodiscard]] constexpr bool owns(std::uint16_t reg_addr) const noexcept {
        return reg_addr >= address &&
               reg_addr < static_cast<std::uint16_t>(address + reg_count);
    }
};

// ============================================================================
// Registry<N>: fixed-capacity descriptor table
// ============================================================================

template <std::size_t N>
class Registry {
    static_assert(N > 0u, "Registry must hold at least one variable");
    static_assert(
        N * sizeof(VarDescriptor) <= ALLOY_MODBUS_REGISTRY_MAX_BYTES,
        "Modbus registry exceeds ALLOY_MODBUS_REGISTRY_MAX_BYTES; "
        "increase the cap or reduce the number of variables");

   public:
    explicit constexpr Registry(std::array<VarDescriptor, N> vars) noexcept
        : vars_{vars} {}

    [[nodiscard]] constexpr std::size_t size() const noexcept { return N; }

    // Linear scan: find the descriptor that owns the given register address.
    // Returns nullptr if no variable covers that address.
    [[nodiscard]] constexpr const VarDescriptor* find(
        std::uint16_t reg_addr) const noexcept {
        for (const auto& v : vars_) {
            if (v.owns(reg_addr)) return &v;
        }
        return nullptr;
    }

    [[nodiscard]] constexpr auto begin() const noexcept { return vars_.begin(); }
    [[nodiscard]] constexpr auto end()   const noexcept { return vars_.end();   }

    [[nodiscard]] constexpr const VarDescriptor& operator[](std::size_t i) const noexcept {
        return vars_[i];
    }

   private:
    std::array<VarDescriptor, N> vars_;
};

// ============================================================================
// Helper: build a Registry from a list of Var<T> descriptors.
// ============================================================================

// Converts a Var<T> to a type-erased VarDescriptor.
template <VarValueType T>
[[nodiscard]] constexpr VarDescriptor make_descriptor(const Var<T>& v) noexcept {
    return VarDescriptor{
        .address    = v.address,
        .reg_count  = static_cast<std::uint8_t>(Var<T>::kRegCount),
        .access     = v.access,
        .word_order = v.word_order,
        .name       = v.name,
    };
}

// Builds a Registry<sizeof...(Vars)> from any mix of Var<T> arguments.
template <VarValueType... Ts>
[[nodiscard]] constexpr auto make_registry(const Var<Ts>&... vars) noexcept {
    return Registry<sizeof...(Ts)>{
        std::array<VarDescriptor, sizeof...(Ts)>{make_descriptor(vars)...}};
}

}  // namespace alloy::modbus
