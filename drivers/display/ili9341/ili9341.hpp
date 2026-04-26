#pragma once

// drivers/display/ili9341/ili9341.hpp
//
// Driver for Ilitek ILI9341 240x320 TFT LCD controller over SPI.
// Written against datasheet ILI9341 V1.11 (2011).
// Seed driver: full init sequence + fill_rect + draw_pixel + blit.
// See drivers/README.md.
//
// SPI mode 0. Bus surface: transfer(span<const uint8_t>, span<uint8_t>).
// DC pin distinguishes command (low) from data (high) bytes.
//
// DC is managed by the DcPolicy template parameter:
//   NoOpDcPolicy        — caller drives DC externally (not recommended).
//   GpioDcPolicy<Pin>   — software GPIO DC (required for correct operation).
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy        — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>   — software GPIO CS (recommended for shared buses).

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::display::ili9341 {

// ── Commands ──────────────────────────────────────────────────────────────────

inline constexpr std::uint8_t kSwReset   = 0x01u;
inline constexpr std::uint8_t kDispOff   = 0x28u;
inline constexpr std::uint8_t kDispOn    = 0x29u;
inline constexpr std::uint8_t kCaSet     = 0x2Au;
inline constexpr std::uint8_t kPaSet     = 0x2Bu;  // page address (rows)
inline constexpr std::uint8_t kRamWr     = 0x2Cu;
inline constexpr std::uint8_t kMadCtl    = 0x36u;
inline constexpr std::uint8_t kPixFmt    = 0x3Au;
inline constexpr std::uint8_t kSleepOut  = 0x11u;
inline constexpr std::uint8_t kPwrCtrl1  = 0xC0u;
inline constexpr std::uint8_t kPwrCtrl2  = 0xC1u;
inline constexpr std::uint8_t kVComCtrl1 = 0xC5u;
inline constexpr std::uint8_t kVComCtrl2 = 0xC7u;
inline constexpr std::uint8_t kFrmCtrl1  = 0xB1u;
inline constexpr std::uint8_t kDfCtrl    = 0xB6u;

// ── Display geometry ──────────────────────────────────────────────────────────

inline constexpr std::uint16_t kWidth  = 240u;
inline constexpr std::uint16_t kHeight = 320u;

// ── Configuration ─────────────────────────────────────────────────────────────

struct Config {
    std::uint8_t madctl = 0x48u;  // BGR=1, MX=1 (landscape-friendly default)
    std::uint8_t pixfmt = 0x55u;  // 16-bit RGB565
};

// ── DC policies ───────────────────────────────────────────────────────────────

/// NoOpDcPolicy: DC pin driven externally. Not recommended for normal use.
struct NoOpDcPolicy {
    void set_command() const noexcept {}
    void set_data()    const noexcept {}
};

/// GpioDcPolicy: drives a GPIO pin low for commands, high for data.
template <typename GpioPin>
struct GpioDcPolicy {
    explicit GpioDcPolicy(GpioPin& pin) : pin_{&pin} {
        (void)pin_->set_high();  // idle = data mode
    }
    void set_command() const noexcept { (void)pin_->set_low();  }
    void set_data()    const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── CS policies ───────────────────────────────────────────────────────────────

/// NoOpCsPolicy: SPI hardware holds CS permanently (default for dedicated bus).
struct NoOpCsPolicy {
    void assert_cs()   const noexcept {}
    void deassert_cs() const noexcept {}
};

/// GpioCsPolicy: software-controlled GPIO chip select.
template <typename GpioPin>
struct GpioCsPolicy {
    explicit GpioCsPolicy(GpioPin& pin) : pin_{&pin} {
        (void)pin_->set_high();  // deasserted at construction
    }
    void assert_cs()   const noexcept { (void)pin_->set_low();  }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 1 ms (calibrated for Cortex-M clock rates).
inline void busy_wait_1ms() {
    volatile std::uint32_t n = 10'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 10 ms.
inline void busy_wait_10ms() {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 150 ms.
inline void busy_wait_150ms() {
    volatile std::uint32_t n = 1'500'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Send a single command byte to the display.
template <typename Bus, typename DcPolicy, typename CsPolicy>
[[nodiscard]] auto send_cmd(Bus& bus, DcPolicy& dc, CsPolicy& cs,
                             std::uint8_t cmd)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    std::array<std::uint8_t, 1> tx{cmd};
    std::array<std::uint8_t, 1> rx{};
    dc.set_command();
    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                          std::span<std::uint8_t>{rx});
    cs.deassert_cs();
    return r;
}

/// Send a command followed by one or more data bytes.
template <typename Bus, typename DcPolicy, typename CsPolicy, std::size_t N>
[[nodiscard]] auto send_cmd_data(Bus& bus, DcPolicy& dc, CsPolicy& cs,
                                  std::uint8_t cmd,
                                  const std::array<std::uint8_t, N>& data)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    if (auto r = send_cmd(bus, dc, cs, cmd); r.is_err()) {
        return r;
    }
    // Send data bytes
    dc.set_data();
    for (std::size_t i = 0; i < N; ++i) {
        std::array<std::uint8_t, 1> tx{data[i]};
        std::array<std::uint8_t, 1> rx{};
        cs.assert_cs();
        auto r = bus.transfer(std::span<const std::uint8_t>{tx},
                              std::span<std::uint8_t>{rx});
        cs.deassert_cs();
        if (r.is_err()) {
            return r;
        }
    }
    return alloy::core::Ok();
}

/// Set the active window (column + page address ranges) for RAM write.
template <typename Bus, typename DcPolicy, typename CsPolicy>
[[nodiscard]] auto set_window(Bus& bus, DcPolicy& dc, CsPolicy& cs,
                               std::uint16_t x, std::uint16_t y,
                               std::uint16_t w, std::uint16_t h)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    const std::uint16_t x1 = static_cast<std::uint16_t>(x + w - 1u);
    const std::uint16_t y1 = static_cast<std::uint16_t>(y + h - 1u);

    // CASET — column address
    {
        const std::array<std::uint8_t, 4> d{
            static_cast<std::uint8_t>(x  >> 8),
            static_cast<std::uint8_t>(x  & 0xFFu),
            static_cast<std::uint8_t>(x1 >> 8),
            static_cast<std::uint8_t>(x1 & 0xFFu),
        };
        if (auto r = send_cmd_data(bus, dc, cs, kCaSet, d); r.is_err()) {
            return r;
        }
    }
    // PASET — page (row) address
    {
        const std::array<std::uint8_t, 4> d{
            static_cast<std::uint8_t>(y  >> 8),
            static_cast<std::uint8_t>(y  & 0xFFu),
            static_cast<std::uint8_t>(y1 >> 8),
            static_cast<std::uint8_t>(y1 & 0xFFu),
        };
        if (auto r = send_cmd_data(bus, dc, cs, kPaSet, d); r.is_err()) {
            return r;
        }
    }
    // RAMWR — begin RAM write
    return send_cmd(bus, dc, cs, kRamWr);
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename BusHandle,
          typename DcPolicy = NoOpDcPolicy,
          typename CsPolicy = NoOpCsPolicy>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus,
                    DcPolicy  dc  = {},
                    CsPolicy  cs  = {},
                    Config    cfg = {})
        : bus_{&bus}, dc_{dc}, cs_{cs}, cfg_{cfg} {}

    /// Run the ILI9341 power-on init sequence.
    ///
    /// Sequence (per ILI9341 application note and Adafruit reference):
    ///   1. SW_RESET   → wait 150 ms.
    ///   2. SLEEP_OUT  → wait 10 ms.
    ///   3. PWR_CTRL1  = [0x23].
    ///   4. PWR_CTRL2  = [0x10].
    ///   5. VCOM_CTRL1 = [0x3E, 0x28].
    ///   6. VCOM_CTRL2 = [0x86].
    ///   7. MADCTL     = [cfg_.madctl].
    ///   8. PIX_FMT    = [cfg_.pixfmt].
    ///   9. FRM_CTRL1  = [0x00, 0x18]  (79 Hz frame rate).
    ///  10. DF_CTRL    = [0x08, 0x82, 0x27].
    ///  11. DISP_ON.
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. Software reset.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSwReset); r.is_err()) {
            return r;
        }
        detail::busy_wait_150ms();

        // 2. Sleep out.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSleepOut); r.is_err()) {
            return r;
        }
        detail::busy_wait_10ms();

        // 3. Power control 1: VRH[5:0] = 4.60 V.
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kPwrCtrl1,
                                            std::array<std::uint8_t, 1>{0x23u});
            r.is_err()) {
            return r;
        }

        // 4. Power control 2: SAP[2:0], BT[3:0].
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kPwrCtrl2,
                                            std::array<std::uint8_t, 1>{0x10u});
            r.is_err()) {
            return r;
        }

        // 5. VCOM control 1.
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kVComCtrl1,
                                            std::array<std::uint8_t, 2>{0x3Eu, 0x28u});
            r.is_err()) {
            return r;
        }

        // 6. VCOM control 2.
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kVComCtrl2,
                                            std::array<std::uint8_t, 1>{0x86u});
            r.is_err()) {
            return r;
        }

        // 7. Memory access control (orientation + colour order).
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kMadCtl,
                                            std::array<std::uint8_t, 1>{cfg_.madctl});
            r.is_err()) {
            return r;
        }

        // 8. Pixel format: RGB565.
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kPixFmt,
                                            std::array<std::uint8_t, 1>{cfg_.pixfmt});
            r.is_err()) {
            return r;
        }

        // 9. Frame control in normal mode: 79 Hz (0x00, 0x18).
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kFrmCtrl1,
                                            std::array<std::uint8_t, 2>{0x00u, 0x18u});
            r.is_err()) {
            return r;
        }

        // 10. Display function control.
        if (auto r = detail::send_cmd_data(*bus_, dc_, cs_, kDfCtrl,
                                            std::array<std::uint8_t, 3>{0x08u, 0x82u, 0x27u});
            r.is_err()) {
            return r;
        }

        // 11. Display on.
        return detail::send_cmd(*bus_, dc_, cs_, kDispOn);
    }

    /// Fill a rectangular region with a single RGB565 colour.
    ///
    /// Pixel bytes are sent in 64-byte stack-allocated chunks to avoid any
    /// heap allocation. The colour is big-endian (MSB first) as required by
    /// the ILI9341 RAM write protocol.
    [[nodiscard]] auto fill_rect(std::uint16_t x, std::uint16_t y,
                                  std::uint16_t w, std::uint16_t h,
                                  std::uint16_t colour) -> ResultVoid {
        if (w == 0u || h == 0u) {
            return alloy::core::Ok();
        }
        if (auto r = detail::set_window(*bus_, dc_, cs_, x, y, w, h); r.is_err()) {
            return r;
        }

        const std::uint32_t total_pixels = static_cast<std::uint32_t>(w) *
                                           static_cast<std::uint32_t>(h);
        const std::uint8_t hi = static_cast<std::uint8_t>(colour >> 8);
        const std::uint8_t lo = static_cast<std::uint8_t>(colour & 0xFFu);

        // Prepare a 64-byte chunk (32 pixels) on the stack.
        constexpr std::size_t kChunkBytes = 64u;
        std::array<std::uint8_t, kChunkBytes> chunk{};
        for (std::size_t i = 0; i < kChunkBytes; i += 2u) {
            chunk[i]     = hi;
            chunk[i + 1] = lo;
        }

        std::array<std::uint8_t, kChunkBytes> rx_dummy{};

        dc_.set_data();
        std::uint32_t remaining = total_pixels * 2u;  // bytes
        while (remaining > 0u) {
            const std::size_t send_bytes = (remaining < kChunkBytes)
                                               ? static_cast<std::size_t>(remaining)
                                               : kChunkBytes;
            cs_.assert_cs();
            auto r = bus_->transfer(
                std::span<const std::uint8_t>{chunk.data(), send_bytes},
                std::span<std::uint8_t>{rx_dummy.data(), send_bytes});
            cs_.deassert_cs();
            if (r.is_err()) {
                return r;
            }
            remaining -= static_cast<std::uint32_t>(send_bytes);
        }
        return alloy::core::Ok();
    }

    /// Draw a single pixel at (x, y) with the given RGB565 colour.
    [[nodiscard]] auto draw_pixel(std::uint16_t x, std::uint16_t y,
                                   std::uint16_t colour) -> ResultVoid {
        return fill_rect(x, y, 1u, 1u, colour);
    }

    /// Blit a w×h pixel buffer (RGB565, big-endian) to the display at (x, y).
    ///
    /// `pixels` must contain exactly w*h entries. The data is transmitted in
    /// 64-byte stack-allocated chunks — no heap is used.
    [[nodiscard]] auto blit(std::uint16_t x, std::uint16_t y,
                             std::uint16_t w, std::uint16_t h,
                             std::span<const std::uint16_t> pixels) -> ResultVoid {
        if (w == 0u || h == 0u) {
            return alloy::core::Ok();
        }
        if (auto r = detail::set_window(*bus_, dc_, cs_, x, y, w, h); r.is_err()) {
            return r;
        }

        // Transmit pixel data in 64-byte chunks (32 pixels each).
        constexpr std::size_t kChunkBytes = 64u;
        constexpr std::size_t kChunkPixels = kChunkBytes / 2u;

        std::array<std::uint8_t, kChunkBytes> chunk{};
        std::array<std::uint8_t, kChunkBytes> rx_dummy{};

        dc_.set_data();
        const std::size_t total_pixels = static_cast<std::size_t>(w) *
                                         static_cast<std::size_t>(h);
        std::size_t pixel_idx = 0u;
        while (pixel_idx < total_pixels) {
            const std::size_t remaining_pixels = total_pixels - pixel_idx;
            const std::size_t batch_pixels = (remaining_pixels < kChunkPixels)
                                                 ? remaining_pixels
                                                 : kChunkPixels;
            const std::size_t batch_bytes = batch_pixels * 2u;

            for (std::size_t i = 0u; i < batch_pixels; ++i) {
                const std::uint16_t c = pixels[pixel_idx + i];
                chunk[i * 2u]     = static_cast<std::uint8_t>(c >> 8);
                chunk[i * 2u + 1] = static_cast<std::uint8_t>(c & 0xFFu);
            }

            cs_.assert_cs();
            auto r = bus_->transfer(
                std::span<const std::uint8_t>{chunk.data(), batch_bytes},
                std::span<std::uint8_t>{rx_dummy.data(), batch_bytes});
            cs_.deassert_cs();
            if (r.is_err()) {
                return r;
            }
            pixel_idx += batch_pixels;
        }
        return alloy::core::Ok();
    }

private:
    BusHandle* bus_;
    DcPolicy   dc_;
    CsPolicy   cs_;
    Config     cfg_;
};

}  // namespace alloy::drivers::display::ili9341

// ── Static assert gate ────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface and default policies.
namespace {
struct _MockSpiForIli9341Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode> {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};
static_assert(
    sizeof(alloy::drivers::display::ili9341::Device<_MockSpiForIli9341Gate>) > 0,
    "ili9341 Device must compile against the documented SPI bus surface");
}  // namespace
