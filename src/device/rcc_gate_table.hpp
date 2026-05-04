/// @file device/rcc_gate_table.hpp
/// Compile-time RCC clock-gate descriptor and lookup API.
///
/// The `RccGate` struct pairs an RCC enable-register address with its
/// single-bit enable mask.  `find_rcc_gate()` resolves a dotted-path string
/// (e.g. `"rcc.apbenr2.usart1en"`) to the corresponding descriptor at
/// compile time.
///
/// # Two-part design
///
/// 1. **This header** — defines `RccGate` and the `find_rcc_gate()` signature.
/// 2. **Generated per-device table** (alloy_device_rcc_table CMake target) —
///    provides the `kRccGates[]` array and the `find_rcc_gate()` definition.
///    Included when `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` is defined (set by the
///    CMake target alongside the include path).
///
/// # Usage in Tier 2 drivers
///
/// ```cpp
/// #include "device/rcc_gate_table.hpp"
///
/// // In port<P>:
/// static void clock_on() noexcept
///     requires (requires { P::kRccEnable; })
/// {
/// #if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
///     constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
///     *reinterpret_cast<volatile std::uint32_t*>(gate.addr) |= gate.mask;
/// #endif
/// }
/// ```
///
/// @note Without `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` the body is empty — the
/// method signature exists and compile tests pass, but no register is written.
/// Set `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` in production firmware builds.
#pragma once

#include <cstdint>
#include <string_view>

namespace alloy::device::detail {

/// Compile-time RCC enable-gate descriptor.
struct RccGate {
    std::uint32_t addr;  ///< Address of the RCC enable register (APBxENR / AHBxENR).
    std::uint32_t mask;  ///< Single-bit enable mask.
};

// ─── Generated gate table ────────────────────────────────────────────────────
// When ALLOY_DEVICE_RCC_TABLE_AVAILABLE is defined by the device build system,
// the generated header is pulled in.  It must define:
//
//   inline constexpr RccGate kRccGates[] = { /* one entry per kRccEnable path */ };
//
//   [[nodiscard]] consteval RccGate find_rcc_gate(const char* dotted_path) {
//       for (const auto& g : kRccGates) {
//           if (std::string_view{g.path} == dotted_path) return g;
//       }
//       // unreachable in valid consteval context — fire error if path unknown
//   }
//
// The generated header is included via the build-system–supplied
// ALLOY_DEVICE_RCC_TABLE_INCLUDE define:
//
//   target_compile_definitions(my_app PRIVATE
//       ALLOY_DEVICE_RCC_TABLE_AVAILABLE=1
//       ALLOY_DEVICE_RCC_TABLE_INCLUDE="path/to/generated_rcc_gates.hpp")
//
// ─────────────────────────────────────────────────────────────────────────────
#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#  include ALLOY_DEVICE_RCC_TABLE_INCLUDE
#else
/// Fallback declaration — body absent without the generated table.
/// Tier 2 drivers guard their call sites with
/// `#if defined(ALLOY_DEVICE_RCC_TABLE_AVAILABLE)`.
[[nodiscard]] consteval RccGate find_rcc_gate(const char* dotted_path) noexcept;
#endif

}  // namespace alloy::device::detail
