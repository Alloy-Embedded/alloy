#pragma once

// drivers/display/uc8151/uc8151.hpp
//
// Driver for Ultrachip UC8151 e-paper display controller over SPI.
// Targets 2.13" (212x104) and 2.9" (296x128) black+white panels.
// Written against the UC8151 / SSD1675-compatible register map.
// Seed driver: init + full-frame update + deep-sleep. See drivers/README.md.
//
// SPI mode 0. Bus surface: transfer(span<const uint8_t>, span<uint8_t>) -> Result<void,ErrorCode>.
// Requires:
//   DcPolicy   — controls the D/C (Data/Command) line.
//   CsPolicy   — controls the chip-select line.
//   BusyPolicy — reads the BUSY pin (high = busy, low = idle).
//
// Supplied policies:
//   NoOpDcPolicy, NoOpCsPolicy, NoOpBusyPolicy   — no-op stubs (hw-managed or test).
//   GpioDcPolicy<Pin>, GpioCsPolicy<Pin>          — GPIO via set_high()/set_low().
//   GpioBusyPolicy<Pin>                           — GPIO input via is_high().

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::display::uc8151 {

// ── Command constants ──────────────────────────────────────────────────────────

inline constexpr std::uint8_t kDriverOutputCtrl     = 0x01u;
inline constexpr std::uint8_t kBoosterSoftStartCtrl = 0x0Cu;
inline constexpr std::uint8_t kGateScanStartPos     = 0x0Fu;
inline constexpr std::uint8_t kDeepSleepMode        = 0x10u;
inline constexpr std::uint8_t kDataEntryMode         = 0x11u;
inline constexpr std::uint8_t kSwReset              = 0x12u;
inline constexpr std::uint8_t kTempSensorCtrl       = 0x18u;
inline constexpr std::uint8_t kMasterActivation     = 0x20u;
inline constexpr std::uint8_t kDisplayUpdateCtrl1   = 0x21u;
inline constexpr std::uint8_t kDisplayUpdateCtrl2   = 0x22u;
inline constexpr std::uint8_t kWriteRam             = 0x24u;  // black/white plane
inline constexpr std::uint8_t kWriteRamRed          = 0x26u;  // red plane (B/W panels: ignored)
inline constexpr std::uint8_t kSetRamXAddrRange     = 0x44u;
inline constexpr std::uint8_t kSetRamYAddrRange     = 0x45u;
inline constexpr std::uint8_t kSetRamXAddrCounter   = 0x4Eu;
inline constexpr std::uint8_t kSetRamYAddrCounter   = 0x4Fu;
inline constexpr std::uint8_t kNop                  = 0xFFu;

// ── DC policies ────────────────────────────────────────────────────────────────

struct NoOpDcPolicy {
    void set_command() const noexcept {}
    void set_data()    const noexcept {}
};

template <typename GpioPin>
struct GpioDcPolicy {
    explicit GpioDcPolicy(GpioPin& pin) : pin_{&pin} {}
    void set_command() const noexcept { (void)pin_->set_low(); }
    void set_data()    const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── CS policies ───────────────────────────────────────────────────────────────

struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_{&pin} {
        (void)pin_->set_high();  // deassert on construction
    }
    void assert_cs()   const noexcept { (void)pin_->set_low(); }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── BUSY policies ─────────────────────────────────────────────────────────────

struct NoOpBusyPolicy {
    [[nodiscard]] bool is_busy() const noexcept { return false; }
};

template <typename GpioPin>
class GpioBusyPolicy {
public:
    explicit GpioBusyPolicy(GpioPin& pin) : pin_{&pin} {}
    /// Returns true while the controller is busy (BUSY pin high).
    [[nodiscard]] bool is_busy() const noexcept {
        return pin_->is_high();
    }
private:
    GpioPin* pin_;
};

// ── Config ─────────────────────────────────────────────────────────────────────

struct Config { /* seed driver: no tuneable fields needed */ };

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 10 ms (spin; calibrated for Cortex-M clock rates).
inline void busy_wait_10ms() {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Poll BUSY pin until it deasserts (low = idle). Returns Timeout after
/// max_iters iterations.
template <typename BusyPolicy>
[[nodiscard]] auto wait_busy(BusyPolicy& busy,
                              std::uint32_t max_iters = 2'000'000u)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    for (std::uint32_t i = 0u; i < max_iters; ++i) {
        if (!busy.is_busy()) {
            return alloy::core::Ok();
        }
    }
    return alloy::core::Err(alloy::core::ErrorCode::Timeout);
}

/// Send one command byte (DC low) then optional data bytes (DC high).
/// CS is asserted/deasserted around each SPI transfer.
/// Data is sent in 64-byte chunks to keep the stack scratch buffer small.
template <typename Bus, typename DcPolicy, typename CsPolicy>
[[nodiscard]] auto send_cmd(Bus& bus, DcPolicy& dc, CsPolicy& cs,
                             std::uint8_t cmd,
                             std::span<const std::uint8_t> data = {})
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    // Command phase: DC low.
    {
        const std::array<std::uint8_t, 1> tx{cmd};
        std::array<std::uint8_t, 1> rx{};
        dc.set_command();
        cs.assert_cs();
        auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                              std::span<std::uint8_t>{rx});
        cs.deassert_cs();
        if (r.is_err()) {
            return r;
        }
    }

    if (data.empty()) {
        return alloy::core::Ok();
    }

    // Data phase: DC high, chunked to avoid stack pressure.
    constexpr std::size_t kChunk = 64u;
    std::array<std::uint8_t, kChunk> rx_scratch{};
    std::size_t offset = 0u;

    dc.set_data();
    while (offset < data.size()) {
        const std::size_t len = (data.size() - offset < kChunk)
                                    ? (data.size() - offset)
                                    : kChunk;
        cs.assert_cs();
        auto r = bus.transfer(
            std::span<const std::uint8_t>{data.data() + offset, len},
            std::span<std::uint8_t>{rx_scratch.data(), len});
        cs.deassert_cs();
        if (r.is_err()) {
            return r;
        }
        offset += len;
    }
    return alloy::core::Ok();
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

/// UC8151 e-paper display driver.
///
/// Template parameters:
///   BusHandle  — SPI bus handle; must provide transfer(span<const uint8_t>, span<uint8_t>).
///   DcPolicy   — controls D/C GPIO; defaults to NoOpDcPolicy.
///   CsPolicy   — controls CS GPIO; defaults to NoOpCsPolicy.
///   BusyPolicy — reads BUSY GPIO; defaults to NoOpBusyPolicy.
///   Width      — panel width in pixels (must be a multiple of 8); default 212.
///   Height     — panel height in pixels; default 104.
template <typename BusHandle,
          typename DcPolicy   = NoOpDcPolicy,
          typename CsPolicy   = NoOpCsPolicy,
          typename BusyPolicy = NoOpBusyPolicy,
          std::uint16_t Width  = 212u,
          std::uint16_t Height = 104u>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    static_assert(Width  > 0u,      "uc8151: Width must be > 0");
    static_assert(Height > 0u,      "uc8151: Height must be > 0");
    static_assert(Width % 8u == 0u, "uc8151: Width must be a multiple of 8");

    explicit Device(BusHandle& bus,
                    DcPolicy   dc   = {},
                    CsPolicy   cs   = {},
                    BusyPolicy busy = {},
                    Config     cfg  = {})
        : bus_{&bus}, dc_{dc}, cs_{cs}, busy_{busy}
    {
        (void)cfg;
    }

    /// Initialise the controller. Must be called before update().
    ///
    /// Sequence:
    ///  1.  SW_RESET → wait_busy.
    ///  2.  DRIVER_OUTPUT_CTRL: gate count = Height-1.
    ///  3.  DATA_ENTRY_MODE: 0x03 (X-inc, Y-inc).
    ///  4.  SET_RAM_X_ADDR_RANGE: [0x00, (Width/8)-1].
    ///  5.  SET_RAM_Y_ADDR_RANGE: [0x00, 0x00, (Height-1)&0xFF, (Height-1)>>8].
    ///  6.  SET_RAM_X_ADDR_COUNTER: [0x00].
    ///  7.  SET_RAM_Y_ADDR_COUNTER: [0x00, 0x00].
    ///  8.  TEMP_SENSOR_CTRL: [0x80] (internal sensor).
    ///  9.  DISPLAY_UPDATE_CTRL2: [0xB1] (enable clock+analog, load LUT from OTP).
    ///  10. MASTER_ACTIVATION → wait_busy.
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. Software reset.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSwReset); r.is_err()) {
            return r;
        }
        detail::busy_wait_10ms();  // controller takes time to assert BUSY
        if (auto r = detail::wait_busy(busy_); r.is_err()) {
            return r;
        }

        // 2. Driver output control: gate count = Height-1.
        {
            constexpr std::uint16_t kGate = Height - 1u;
            const std::array<std::uint8_t, 3> d{
                static_cast<std::uint8_t>(kGate & 0xFFu),
                static_cast<std::uint8_t>((kGate >> 8u) & 0x01u),
                0x00u,
            };
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kDriverOutputCtrl, d); r.is_err()) {
                return r;
            }
        }

        // 3. Data entry mode: X-inc, Y-inc.
        {
            const std::array<std::uint8_t, 1> d{0x03u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kDataEntryMode, d); r.is_err()) {
                return r;
            }
        }

        // 4. RAM X address range: [0x00 .. (Width/8)-1].
        {
            constexpr auto kXEnd = static_cast<std::uint8_t>((Width / 8u) - 1u);
            const std::array<std::uint8_t, 2> d{0x00u, kXEnd};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamXAddrRange, d); r.is_err()) {
                return r;
            }
        }

        // 5. RAM Y address range: [0x00, 0x00, LSB, MSB] for Height-1.
        {
            constexpr std::uint16_t kYEnd = Height - 1u;
            const std::array<std::uint8_t, 4> d{
                0x00u,
                0x00u,
                static_cast<std::uint8_t>(kYEnd & 0xFFu),
                static_cast<std::uint8_t>((kYEnd >> 8u) & 0x01u),
            };
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamYAddrRange, d); r.is_err()) {
                return r;
            }
        }

        // 6. Set RAM X address counter to 0.
        {
            const std::array<std::uint8_t, 1> d{0x00u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamXAddrCounter, d); r.is_err()) {
                return r;
            }
        }

        // 7. Set RAM Y address counter to 0.
        {
            const std::array<std::uint8_t, 2> d{0x00u, 0x00u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamYAddrCounter, d); r.is_err()) {
                return r;
            }
        }

        // 8. Temperature sensor: use internal.
        {
            const std::array<std::uint8_t, 1> d{0x80u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kTempSensorCtrl, d); r.is_err()) {
                return r;
            }
        }

        // 9. Display update control 2: enable clock + analog, load LUT from OTP.
        {
            const std::array<std::uint8_t, 1> d{0xB1u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kDisplayUpdateCtrl2, d); r.is_err()) {
                return r;
            }
        }

        // 10. Master activation → wait for controller to complete OTP load.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kMasterActivation); r.is_err()) {
            return r;
        }
        detail::busy_wait_10ms();
        return detail::wait_busy(busy_);
    }

    /// Full update: write a packed 1-bit-per-pixel framebuffer and trigger
    /// a complete panel refresh.
    ///
    /// framebuffer layout:
    ///   Expected size = (Width × Height + 7) / 8 bytes.
    ///   Each byte: MSB = leftmost pixel. 0 = white, 1 = black.
    ///
    /// Returns InvalidParameter if the buffer size does not match.
    [[nodiscard]] auto update(std::span<const std::uint8_t> framebuffer) -> ResultVoid {
        constexpr std::size_t kExpected =
            (static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height) + 7u) / 8u;

        if (framebuffer.size() != kExpected) {
            return alloy::core::Err(alloy::core::ErrorCode::InvalidParameter);
        }

        // 1. Reset RAM X address counter.
        {
            const std::array<std::uint8_t, 1> xd{0x00u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamXAddrCounter, xd); r.is_err()) {
                return r;
            }
        }

        // 2. Reset RAM Y address counter.
        {
            const std::array<std::uint8_t, 2> yd{0x00u, 0x00u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSetRamYAddrCounter, yd); r.is_err()) {
                return r;
            }
        }

        // 3. WRITE_RAM command followed by entire framebuffer.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kWriteRam, framebuffer); r.is_err()) {
            return r;
        }

        // 4. Display update control 2: load LUT + display.
        {
            const std::array<std::uint8_t, 1> d{0xC7u};
            if (auto r = detail::send_cmd(*bus_, dc_, cs_, kDisplayUpdateCtrl2, d); r.is_err()) {
                return r;
            }
        }

        // 5. Master activation → wait for panel refresh to complete.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kMasterActivation); r.is_err()) {
            return r;
        }
        detail::busy_wait_10ms();
        return detail::wait_busy(busy_);
    }

    /// Enter deep-sleep mode to minimise idle current after update().
    /// Re-run init() before the next update().
    [[nodiscard]] auto sleep() -> ResultVoid {
        const std::array<std::uint8_t, 1> d{0x01u};
        return detail::send_cmd(*bus_, dc_, cs_, kDeepSleepMode, d);
    }

private:
    BusHandle*  bus_;
    DcPolicy    dc_;
    CsPolicy    cs_;
    BusyPolicy  busy_;
};

}  // namespace alloy::drivers::display::uc8151

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented SPI
// bus surface (transfer) and the default no-op GPIO policies.
namespace {

struct _MockSpiForUc8151Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

static_assert(
    sizeof(alloy::drivers::display::uc8151::Device<_MockSpiForUc8151Gate>) > 0,
    "uc8151 Device must compile against the documented SPI + GPIO policy surface");

}  // namespace
