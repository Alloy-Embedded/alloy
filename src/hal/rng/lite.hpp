/// @file hal/rng/lite.hpp
/// Hardware Random Number Generator (RNG) — alloy.device.v2.1 lite driver.
///
/// Thin wrapper for the STM32 RNG peripheral.  Provides blocking and
/// non-blocking reads of 32-bit random words.
///
/// Address-template variant — no codegen format required:
/// @code
///   #include "hal/rng/lite.hpp"
///
///   // STM32G4 RNG base = 0x50060800 (AHB2)
///   using Rng = alloy::hal::rng::lite::engine<0x50060800u>;
///
///   dev::peripheral_on<dev::rng>();   // enable RNG clock
///   Rng::configure();
///   const std::uint32_t rand_word = Rng::read();  // blocking
/// @endcode
///
/// Common RNG base addresses:
///   STM32G4         : 0x50060800  (AHB2)
///   STM32L4 / WB    : 0x50060800
///   STM32H7         : 0x48021800  (AHB2)
///   STM32U5         : 0x520C0800
///
/// Note: STM32G0 does not have a hardware RNG peripheral.
///
/// Register layout (same on G4 / L4 / H7 / WB / U5):
///   0x00 CR  — RNGEN(2), IE(3), CED(5)
///   0x04 SR  — DRDY(0), CECS(1), SECS(2), CEIS(5), SEIS(6)
///   0x08 DR  — 32-bit random number (read-only; clears DRDY)
#pragma once

#include <cstdint>

namespace alloy::hal::rng::lite {

namespace detail {

inline constexpr std::uintptr_t kOfsCr = 0x00u;
inline constexpr std::uintptr_t kOfsSr = 0x04u;
inline constexpr std::uintptr_t kOfsDr = 0x08u;

// CR bits
inline constexpr std::uint32_t kCrRngen = 1u << 2;  ///< RNG enable
inline constexpr std::uint32_t kCrIe    = 1u << 3;  ///< Interrupt enable
inline constexpr std::uint32_t kCrCed   = 1u << 5;  ///< Clock error detection disable

// SR bits
inline constexpr std::uint32_t kSrDrdy  = 1u << 0;  ///< Data ready
inline constexpr std::uint32_t kSrCecs  = 1u << 1;  ///< Clock error current status
inline constexpr std::uint32_t kSrSecs  = 1u << 2;  ///< Seed error current status
inline constexpr std::uint32_t kSrCeis  = 1u << 5;  ///< Clock error interrupt status
inline constexpr std::uint32_t kSrSeis  = 1u << 6;  ///< Seed error interrupt status

}  // namespace detail

/// RNG hardware engine.
///
/// `RngBase` is the RNG peripheral base address (see file header for values).
template <std::uintptr_t RngBase>
class engine {
   // -------------------------------------------------------------------------
   // Vendor scope: STM32 RNG (true random number generator) — G0/G4/L4/WB/H7
   //
   // CR/SR/DR register layout is STM32-specific.
   // Do NOT use for SAME70 TRNG, nRF52 RNG, or RP2040 ROSC randomness.
   // Define ALLOY_ASSERT_VENDOR_STM32 to catch mis-use at compile time.
   // -------------------------------------------------------------------------
#if defined(ALLOY_ASSERT_VENDOR_STM32)
    static_assert(ALLOY_DEVICE_VENDOR_STM32,
        "hal/rng/lite.hpp: RNG layout is STM32-only. "
        "Use a vendor-specific RNG driver for non-STM32 targets.");
#endif

   private:
    [[nodiscard]] static auto reg(std::uintptr_t ofs) noexcept
        -> volatile std::uint32_t& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<volatile std::uint32_t*>(RngBase + ofs);
    }

   public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Enable the RNG peripheral (CR.RNGEN = 1).
    ///
    /// The peripheral clock must be enabled before calling this
    /// (e.g. `dev::peripheral_on<dev::rng>()`).
    /// After enabling, the first random word is typically ready within a few
    /// hundred clock cycles — `read()` polls DRDY automatically.
    static void configure() noexcept {
        reg(detail::kOfsCr) |= detail::kCrRngen;
    }

    /// Disable the RNG (CR.RNGEN = 0).
    static void disable() noexcept {
        reg(detail::kOfsCr) &= ~detail::kCrRngen;
    }

    // -----------------------------------------------------------------------
    // Blocking read
    // -----------------------------------------------------------------------

    /// Read one 32-bit random word (blocks until DRDY = 1).
    ///
    /// If an error is detected (CECS or SECS), the value of DR is
    /// implementation-defined.  Check `has_error()` after each call in
    /// security-sensitive applications.
    [[nodiscard]] static auto read() noexcept -> std::uint32_t {
        while (!ready()) { /* spin */ }
        return reg(detail::kOfsDr);   // reading DR clears DRDY
    }

    // -----------------------------------------------------------------------
    // Status
    // -----------------------------------------------------------------------

    /// True when a fresh 32-bit random word is available in DR (SR.DRDY = 1).
    [[nodiscard]] static auto ready() noexcept -> bool {
        return (reg(detail::kOfsSr) & detail::kSrDrdy) != 0u;
    }

    /// True if a clock or seed error has been flagged (CEIS or SEIS in SR).
    ///
    /// A clock error means the RNG clock was too slow relative to the AHB
    /// clock; a seed error means the noise source failed to pass the health
    /// test.  Both render any previously generated values suspect.
    [[nodiscard]] static auto has_error() noexcept -> bool {
        constexpr auto kErrMask = detail::kSrCeis | detail::kSrSeis;
        return (reg(detail::kOfsSr) & kErrMask) != 0u;
    }

    /// Clear clock and seed error flags (CEIS, SEIS in SR).
    ///
    /// After clearing, the RNG recovers automatically on the next
    /// successful conditioning cycle.  Re-enable the RNG if RNGEN was
    /// cleared by the hardware error recovery.
    static void clear_errors() noexcept {
        reg(detail::kOfsSr) &= ~(detail::kSrCeis | detail::kSrSeis);
    }

    // -----------------------------------------------------------------------
    // Interrupt control
    // -----------------------------------------------------------------------

    /// Enable the RNG interrupt (CR.IE = 1).
    ///
    /// Fires on both DRDY (data ready) and error conditions (CEIS/SEIS).
    /// Wire up via `Nvic::enable_irq(rng_irq_n)` as well.
    static void enable_irq() noexcept {
        reg(detail::kOfsCr) |= detail::kCrIe;
    }

    /// Disable the RNG interrupt (CR.IE = 0).
    static void disable_irq() noexcept {
        reg(detail::kOfsCr) &= ~detail::kCrIe;
    }

    engine() = delete;
};

}  // namespace alloy::hal::rng::lite
