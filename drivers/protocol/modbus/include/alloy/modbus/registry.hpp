#pragma once

// registry.hpp: flat array of type-erased variable descriptors.
//
// Two construction modes:
//   - Metadata-only (constexpr): make_registry(Var<T>...) — no data pointer,
//     useful for discovery and footprint checks.
//   - Bound (runtime): Registry<N>{{ bind(var, value), ... }} — stores a data
//     pointer and encode/decode function pointers so the slave can read/write.
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

    // Bound-mode fields: null in metadata-only mode.
    void* data{nullptr};
    void (*encode_fn)(const void* data, std::uint16_t* out, WordOrder) noexcept{nullptr};
    void (*decode_fn)(const std::uint16_t* in, void* data, WordOrder) noexcept{nullptr};

    // True if the given Modbus register address falls within this variable.
    [[nodiscard]] constexpr bool owns(std::uint16_t reg_addr) const noexcept {
        return reg_addr >= address &&
               reg_addr < static_cast<std::uint16_t>(address + reg_count);
    }

    // Copy reg_count register values to out[]. No-op if encode_fn is null.
    void encode_to(std::uint16_t* out) const noexcept {
        if (encode_fn) encode_fn(data, out, word_order);
    }

    // Write reg_count register values from in[] into the bound value.
    // No-op if decode_fn is null.
    void decode_from(const std::uint16_t* in) const noexcept {
        if (decode_fn) decode_fn(in, data, word_order);
    }
};

// ============================================================================
// VarTraits<T>: static encode/decode adapters used as fn pointers in bind()
// ============================================================================

template <VarValueType T>
struct VarTraits {
    static void encode(const void* data, std::uint16_t* out, WordOrder order) noexcept {
        const auto words = encode_words<T>(*static_cast<const T*>(data), order);
        for (std::size_t i = 0u; i < words.size(); ++i) out[i] = words[i];
    }

    static void decode(const std::uint16_t* in, void* data, WordOrder order) noexcept {
        std::span<const std::uint16_t, var_reg_count<T>()> words{in, var_reg_count<T>()};
        *static_cast<T*>(data) = decode_words<T>(words, order);
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
// Factories
// ============================================================================

// Metadata-only descriptor (no data pointer). Result is constexpr-safe.
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

// Bound descriptor: attaches a data pointer and typed encode/decode fns.
// Not constexpr (data is a runtime address).
template <VarValueType T>
[[nodiscard]] VarDescriptor bind(const Var<T>& v, T& value) noexcept {
    return VarDescriptor{
        .address    = v.address,
        .reg_count  = static_cast<std::uint8_t>(Var<T>::kRegCount),
        .access     = v.access,
        .word_order = v.word_order,
        .name       = v.name,
        .data       = &value,
        .encode_fn  = VarTraits<T>::encode,
        .decode_fn  = VarTraits<T>::decode,
    };
}

// Builds a metadata-only Registry<sizeof...(Ts)> from Var<T>... descriptors.
template <VarValueType... Ts>
[[nodiscard]] constexpr auto make_registry(const Var<Ts>&... vars) noexcept {
    return Registry<sizeof...(Ts)>{
        std::array<VarDescriptor, sizeof...(Ts)>{make_descriptor(vars)...}};
}

}  // namespace alloy::modbus
