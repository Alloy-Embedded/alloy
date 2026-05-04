/// @file hal/flash/lite.hpp
/// Flash access controller (ACR) — alloy.device.v2.1 lite driver.
///
/// Controls flash wait states, prefetch buffer, and instruction/data caches.
/// Setting the correct latency BEFORE raising the system clock prevents
/// bus faults on zero-wait devices.  Lowering the latency AFTER reducing
/// the clock is also safe.
///
/// Address-template variant — no codegen format required:
/// @code
///   #include "hal/flash/lite.hpp"
///
///   // STM32G0 / G4 FLASH base = 0x40022000
///   using Flash = alloy::hal::flash::lite::controller<0x40022000u>;
///
///   Flash::set_latency(2u);      // 2 WS for 64 MHz on G4 VOS1
///   Flash::enable_prefetch();
///   Flash::enable_icache();
/// @endcode
///
/// Latency guidelines (STM32G4, VOS1, 3.3 V):
///   0 WS — HCLK ≤ 20 MHz
///   1 WS — HCLK ≤ 48 MHz
///   2 WS — HCLK ≤ 72 MHz   ← typical 64 MHz PLL
///   3 WS — HCLK ≤ 96 MHz
///   4 WS — HCLK ≤ 120 MHz
///   ...
///   8 WS — HCLK ≤ 170 MHz
///
/// Register layout (0x00 = ACR — same offset on G0 / G4 / F0 / F3 / F4 / L4):
///   ACR: LATENCY[3:0](3:0), PRFTEN(8), ICEN(9), DCEN(10), ICRST(11), DCRST(12)
///
/// H7 note: uses a different base (0x52002000) and a more complex ACR layout;
/// the latency / prefetch fields are at the same offsets and this driver is
/// compatible with H7 bank-1 usage.
#pragma once

#include <cstdint>

namespace alloy::hal::flash::lite {

namespace detail {

inline constexpr std::uint32_t kAcrLatMask = 0xFu;       ///< LATENCY[3:0]
inline constexpr std::uint32_t kAcrPrften  = 1u << 8;    ///< Prefetch buffer enable
inline constexpr std::uint32_t kAcrIcen    = 1u << 9;    ///< Instruction-cache enable
inline constexpr std::uint32_t kAcrDcen    = 1u << 10;   ///< Data-cache enable
inline constexpr std::uint32_t kAcrIcrst   = 1u << 11;   ///< Instruction-cache reset
inline constexpr std::uint32_t kAcrDcrst   = 1u << 12;   ///< Data-cache reset

}  // namespace detail

/// Flash access controller.
///
/// `FlashBase` is the FLASH peripheral base address:
///   - STM32G0 / G4 / F0 / F1 / F3 / L4 / WB : 0x40022000
///   - STM32F4                                  : 0x40023C00
///   - STM32H7 (bank 1)                         : 0x52002000
template <std::uintptr_t FlashBase>
class controller {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 Flash interface (F0/F1/F3/G0/G4/L4/WB/H7)
   //
   // ACR latency field encoding and prefetch/I-cache/D-cache bits are
   // STM32-specific.  Do NOT use for SAME70 EFC, nRF52 NVMC, or RP2040 XIP.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/flash/lite.hpp: Flash interface layout is STM32-only. "
        "Use a vendor-specific flash driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto acr() noexcept -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(FlashBase);
    }

   public:
    // -----------------------------------------------------------------------
    // Wait states
    // -----------------------------------------------------------------------

    /// Set the flash read latency (wait-state count).
    ///
    /// Call BEFORE raising the system clock to avoid a lockup.
    /// Reads back ACR after writing to confirm the hardware has latched the
    /// new value before control returns (some buses have a pipeline delay).
    ///
    /// @param wait_states  Number of wait states (0–15).  0 = zero-wait
    ///                     (typically safe up to ~24 MHz on most families).
    static void set_latency(std::uint32_t wait_states) noexcept {
        acr() = (acr() & ~detail::kAcrLatMask) | (wait_states & detail::kAcrLatMask);
        // Readback ensures the write is flushed before any clock configuration.
        while ((acr() & detail::kAcrLatMask) != (wait_states & detail::kAcrLatMask)) {
            /* wait */
        }
    }

    /// Return the current flash latency (wait-state count from ACR).
    [[nodiscard]] static auto get_latency() noexcept -> std::uint32_t {
        return acr() & detail::kAcrLatMask;
    }

    // -----------------------------------------------------------------------
    // Prefetch buffer
    // -----------------------------------------------------------------------

    /// Enable the prefetch buffer (ACR.PRFTEN = 1).
    ///
    /// Improves sequential fetch throughput when HCLK > ~24 MHz.
    /// Should be enabled before raising the clock.
    static void enable_prefetch() noexcept {
        acr() |= detail::kAcrPrften;
    }

    /// Disable the prefetch buffer (ACR.PRFTEN = 0).
    static void disable_prefetch() noexcept {
        acr() &= ~detail::kAcrPrften;
    }

    // -----------------------------------------------------------------------
    // Instruction cache
    // -----------------------------------------------------------------------

    /// Enable the instruction cache (ACR.ICEN = 1).
    ///
    /// Available on G4 / H7 / L4 / WB.  Writing on families that lack a
    /// hardware I-cache is a no-op (reserved bit write).
    static void enable_icache() noexcept {
        acr() |= detail::kAcrIcen;
    }

    /// Disable and reset the instruction cache.
    ///
    /// The reset (ICRST) must be asserted and then deasserted to flush stale
    /// cache lines — e.g. after an in-application flash write.
    static void disable_icache() noexcept {
        acr() &= ~detail::kAcrIcen;
        acr() |=  detail::kAcrIcrst;
        acr() &= ~detail::kAcrIcrst;
    }

    // -----------------------------------------------------------------------
    // Data cache
    // -----------------------------------------------------------------------

    /// Enable the data cache (ACR.DCEN = 1).
    ///
    /// Available on G4 / H7.  Harmless reserved-bit write on other families.
    static void enable_dcache() noexcept {
        acr() |= detail::kAcrDcen;
    }

    /// Disable and reset the data cache.
    static void disable_dcache() noexcept {
        acr() &= ~detail::kAcrDcen;
        acr() |=  detail::kAcrDcrst;
        acr() &= ~detail::kAcrDcrst;
    }

    // -----------------------------------------------------------------------
    // Convenience: configure for a given clock frequency
    // -----------------------------------------------------------------------

    /// Apply latency + prefetch + I-cache in one call.
    ///
    /// Convenience wrapper for the typical clock-change sequence:
    ///   1. Set latency (and wait for readback).
    ///   2. Enable prefetch.
    ///   3. Enable instruction cache.
    ///
    /// Call this BEFORE switching to the higher clock.
    static void configure(std::uint32_t wait_states) noexcept {
        set_latency(wait_states);
        enable_prefetch();
        enable_icache();
    }

    controller() = delete;
};

}  // namespace alloy::hal::flash::lite
