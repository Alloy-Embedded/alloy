#pragma once

// alloy::hal::mdio — MDIO bus abstraction (IEEE 802.3 clause 22).
//
// Defines the MdioBus<T> concept that all MDIO bus drivers must satisfy.
// PHY drivers (e.g. KSZ8081) are templated over this concept; they never
// depend on a concrete bus type.
//
// Concept contract:
//   read(phy, reg)         — reads a 16-bit PHY register; returns Result<uint16_t>
//   write(phy, reg, value) — writes a 16-bit PHY register; returns Result<void>
//
// Both operations must be noexcept; error propagation uses core::Result.
// ErrorCode::Timeout is the expected error when the management bus hangs.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal::mdio {

template <typename T>
concept MdioBus = requires(T& bus,
                            const T& cbus,
                            std::uint8_t phy,
                            std::uint8_t reg,
                            std::uint16_t value) {
    // Clause-22 register read: phy in [0,31], reg in [0,31].
    { cbus.read(phy, reg) } noexcept
        -> std::same_as<core::Result<std::uint16_t, core::ErrorCode>>;

    // Clause-22 register write.
    { bus.write(phy, reg, value) } noexcept
        -> std::same_as<core::Result<void, core::ErrorCode>>;
};

}  // namespace alloy::hal::mdio
