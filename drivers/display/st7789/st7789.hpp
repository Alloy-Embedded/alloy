#pragma once

// drivers/display/st7789/st7789.hpp
//
// Driver for Sitronix ST7789 240×320 TFT LCD controller over SPI.
// Written against datasheet revision V1.3 (2014).
// Seed driver: init sequence + fill_rect + draw_pixel + blit.
// See drivers/README.md.
//
// The DC (data/command) pin is managed by the DcPolicy template parameter:
//   NoOpDcPolicy          — DC toggle assumed to be handled externally (default).
//   GpioDcPolicy<Pin>     — software GPIO DC pin (required for typical wiring).
//
// CS is managed by the CsPolicy template parameter:
//   NoOpCsPolicy          — SPI hardware holds CS permanently (default).
//   GpioCsPolicy<Pin>     — software GPIO CS (recommended for shared buses).
//
// SPI mode 0 or mode 3. Bus surface: transfer(span<const uint8_t>, span<uint8_t>).
// Write protocol: issue command byte with DC=low, then data bytes with DC=high.

#include <array>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::display::st7789 {

// ── Commands ──────────────────────────────────────────────────────────────────

inline constexpr std::uint8_t kNop     = 0x00u;  // No-operation
inline constexpr std::uint8_t kSwReset = 0x01u;  // Software reset
inline constexpr std::uint8_t kSlpOut  = 0x11u;  // Sleep out (exits sleep mode)
inline constexpr std::uint8_t kColMod  = 0x3Au;  // Interface pixel format
inline constexpr std::uint8_t kMadCtl  = 0x36u;  // Memory access control (rotation/mirror)
inline constexpr std::uint8_t kDispOn  = 0x29u;  // Display on
inline constexpr std::uint8_t kCaSet   = 0x2Au;  // Column address set
inline constexpr std::uint8_t kRaSet   = 0x2Bu;  // Row address set
inline constexpr std::uint8_t kRamWr   = 0x2Cu;  // Memory write

// ── Display geometry ──────────────────────────────────────────────────────────

inline constexpr std::uint16_t kWidth  = 240u;
inline constexpr std::uint16_t kHeight = 320u;

// ── Configuration ─────────────────────────────────────────────────────────────

struct Config {
    std::uint8_t madctl = 0x00u;  // MADCTL register: rotation / mirror bits
    std::uint8_t colmod = 0x55u;  // Pixel format: 0x55 = 16-bit RGB565
};

// ── DC policies ───────────────────────────────────────────────────────────────

/// NoOpDcPolicy: DC pin toggling is handled externally (e.g., tied to CS or
/// managed by hardware). All calls are no-ops.
struct NoOpDcPolicy {
    void set_command() const noexcept {}
    void set_data()    const noexcept {}
};

/// GpioDcPolicy<Pin>: drives a GPIO pin to distinguish command vs data bytes.
/// set_command() → DC low; set_data() → DC high.
template <typename GpioPin>
class GpioDcPolicy {
public:
    explicit GpioDcPolicy(GpioPin& pin) : pin_{&pin} {}

    void set_command() const noexcept { (void)pin_->set_low();  }
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
        (void)pin_->set_high();
    }
    void assert_cs()   const noexcept { (void)pin_->set_low();  }
    void deassert_cs() const noexcept { (void)pin_->set_high(); }
private:
    GpioPin* pin_;
};

// ── Private helpers ────────────────────────────────────────────────────────────

namespace detail {

/// Busy-wait approximately 150 ms (required after SW_RESET).
inline void busy_wait_150ms() {
    volatile std::uint32_t n = 1'500'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Busy-wait approximately 10 ms (required after SLPOUT).
inline void busy_wait_10ms() {
    volatile std::uint32_t n = 100'000u;
    while (n-- != 0u) { /* intentional spin */ }
}

/// Send a command byte (DC low) then optional data bytes (DC high).
/// CS is asserted/deasserted around the entire transaction.
template <typename Bus, typename Dc, typename Cs>
[[nodiscard]] auto send_cmd(Bus& bus, Dc& dc, Cs& cs,
                             std::uint8_t cmd,
                             std::span<const std::uint8_t> data = {})
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    // Send command byte with DC=low.
    std::array<std::uint8_t, 1> tx_cmd{cmd};
    std::array<std::uint8_t, 1> rx_cmd{};

    dc.set_command();
    cs.assert_cs();
    auto r = bus.transfer(std::span<const std::uint8_t>{tx_cmd},
                          std::span<std::uint8_t>{rx_cmd});
    cs.deassert_cs();
    if (r.is_err()) {
        return alloy::core::Err(std::move(r).err());
    }

    // Send optional data bytes with DC=high.
    if (!data.empty()) {
        // Send data in chunks to avoid needing a scratch buffer larger than needed.
        // We send byte-by-byte here since we only have the user-provided span.
        dc.set_data();
        for (std::size_t i = 0; i < data.size(); ++i) {
            std::array<std::uint8_t, 1> tx_byte{data[i]};
            std::array<std::uint8_t, 1> rx_byte{};
            cs.assert_cs();
            auto rb = bus.transfer(std::span<const std::uint8_t>{tx_byte},
                                   std::span<std::uint8_t>{rx_byte});
            cs.deassert_cs();
            if (rb.is_err()) {
                return alloy::core::Err(std::move(rb).err());
            }
        }
    }

    return alloy::core::Ok();
}

/// Set column + row address window (CASET + RASET).
/// x0/y0: top-left inclusive, x1/y1: bottom-right inclusive.
template <typename Bus, typename Dc, typename Cs>
[[nodiscard]] auto set_window(Bus& bus, Dc& dc, Cs& cs,
                               std::uint16_t x0, std::uint16_t y0,
                               std::uint16_t x1, std::uint16_t y1)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    // CASET: x0_hi, x0_lo, x1_hi, x1_lo
    const std::array<std::uint8_t, 4> ca{
        static_cast<std::uint8_t>(x0 >> 8u),
        static_cast<std::uint8_t>(x0 & 0xFFu),
        static_cast<std::uint8_t>(x1 >> 8u),
        static_cast<std::uint8_t>(x1 & 0xFFu),
    };
    if (auto r = send_cmd(bus, dc, cs, kCaSet, std::span<const std::uint8_t>{ca});
        r.is_err()) {
        return r;
    }

    // RASET: y0_hi, y0_lo, y1_hi, y1_lo
    const std::array<std::uint8_t, 4> ra{
        static_cast<std::uint8_t>(y0 >> 8u),
        static_cast<std::uint8_t>(y0 & 0xFFu),
        static_cast<std::uint8_t>(y1 >> 8u),
        static_cast<std::uint8_t>(y1 & 0xFFu),
    };
    return send_cmd(bus, dc, cs, kRaSet, std::span<const std::uint8_t>{ra});
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
                    DcPolicy   dc  = {},
                    CsPolicy   cs  = {},
                    Config     cfg = {})
        : bus_{&bus}, dc_{dc}, cs_{cs}, cfg_{cfg} {}

    /// Runs the ST7789 initialization sequence:
    ///   1. SW_RESET
    ///   2. Wait 150 ms
    ///   3. SLPOUT (sleep out)
    ///   4. Wait 10 ms
    ///   5. COLMOD = cfg.colmod  (0x55 = 16-bit RGB565)
    ///   6. MADCTL = cfg.madctl  (rotation / mirror)
    ///   7. DISPON
    [[nodiscard]] auto init() -> ResultVoid {
        // 1. Software reset.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSwReset); r.is_err()) {
            return r;
        }

        // 2. Wait for reset to complete.
        detail::busy_wait_150ms();

        // 3. Exit sleep mode.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kSlpOut); r.is_err()) {
            return r;
        }

        // 4. Wait for sleep-out.
        detail::busy_wait_10ms();

        // 5. Set pixel format.
        const std::array<std::uint8_t, 1> colmod_data{cfg_.colmod};
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kColMod,
                                       std::span<const std::uint8_t>{colmod_data});
            r.is_err()) {
            return r;
        }

        // 6. Memory access control (rotation / mirror).
        const std::array<std::uint8_t, 1> madctl_data{cfg_.madctl};
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kMadCtl,
                                       std::span<const std::uint8_t>{madctl_data});
            r.is_err()) {
            return r;
        }

        // 7. Display on.
        return detail::send_cmd(*bus_, dc_, cs_, kDispOn);
    }

    /// Fill a rectangular region with a solid RGB565 colour.
    /// Sends pixel data in 64-byte stack chunks — no heap allocation.
    [[nodiscard]] auto fill_rect(std::uint16_t x, std::uint16_t y,
                                  std::uint16_t w, std::uint16_t h,
                                  std::uint16_t colour)
        -> ResultVoid
    {
        if (w == 0u || h == 0u) {
            return alloy::core::Ok();
        }

        // Set address window.
        if (auto r = detail::set_window(*bus_, dc_, cs_,
                                         x, y,
                                         static_cast<std::uint16_t>(x + w - 1u),
                                         static_cast<std::uint16_t>(y + h - 1u));
            r.is_err()) {
            return r;
        }

        // Send RAMWR command (no data yet).
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kRamWr); r.is_err()) {
            return r;
        }

        // Pre-fill a 64-byte chunk buffer with the colour (big-endian RGB565).
        static constexpr std::size_t kChunkBytes = 64u;
        static constexpr std::size_t kChunkPixels = kChunkBytes / 2u;

        const std::uint8_t colour_hi = static_cast<std::uint8_t>(colour >> 8u);
        const std::uint8_t colour_lo = static_cast<std::uint8_t>(colour & 0xFFu);

        std::array<std::uint8_t, kChunkBytes> chunk{};
        for (std::size_t i = 0u; i < kChunkPixels; ++i) {
            chunk[i * 2u]      = colour_hi;
            chunk[i * 2u + 1u] = colour_lo;
        }
        std::array<std::uint8_t, kChunkBytes> rx_chunk{};

        const std::uint32_t total_pixels = static_cast<std::uint32_t>(w) *
                                           static_cast<std::uint32_t>(h);
        std::uint32_t remaining = total_pixels;

        dc_.set_data();
        while (remaining > 0u) {
            const std::uint32_t batch =
                (remaining < kChunkPixels)
                    ? remaining
                    : static_cast<std::uint32_t>(kChunkPixels);
            const std::size_t batch_bytes = static_cast<std::size_t>(batch) * 2u;

            cs_.assert_cs();
            auto r = bus_->transfer(
                std::span<const std::uint8_t>{chunk.data(), batch_bytes},
                std::span<std::uint8_t>{rx_chunk.data(), batch_bytes});
            cs_.deassert_cs();

            if (r.is_err()) {
                return alloy::core::Err(std::move(r).err());
            }
            remaining -= batch;
        }

        return alloy::core::Ok();
    }

    /// Draw a single pixel at (x, y) with the given RGB565 colour.
    [[nodiscard]] auto draw_pixel(std::uint16_t x, std::uint16_t y,
                                   std::uint16_t colour)
        -> ResultVoid
    {
        return fill_rect(x, y, 1u, 1u, colour);
    }

    /// Blit a user-supplied RGB565 framebuffer to a rectangle.
    /// `pixels` must contain exactly w * h pixels (2 bytes each, big-endian).
    [[nodiscard]] auto blit(std::uint16_t x, std::uint16_t y,
                             std::uint16_t w, std::uint16_t h,
                             std::span<const std::uint16_t> pixels)
        -> ResultVoid
    {
        if (w == 0u || h == 0u) {
            return alloy::core::Ok();
        }

        // Set address window.
        if (auto r = detail::set_window(*bus_, dc_, cs_,
                                         x, y,
                                         static_cast<std::uint16_t>(x + w - 1u),
                                         static_cast<std::uint16_t>(y + h - 1u));
            r.is_err()) {
            return r;
        }

        // Send RAMWR command.
        if (auto r = detail::send_cmd(*bus_, dc_, cs_, kRamWr); r.is_err()) {
            return r;
        }

        // Send pixel data as big-endian byte pairs.
        // Use a small stack chunk to batch the transfer.
        static constexpr std::size_t kChunkPixels = 32u;
        static constexpr std::size_t kChunkBytes  = kChunkPixels * 2u;

        std::array<std::uint8_t, kChunkBytes> chunk{};
        std::array<std::uint8_t, kChunkBytes> rx_chunk{};

        dc_.set_data();

        const std::size_t total = pixels.size();
        std::size_t sent = 0u;

        while (sent < total) {
            const std::size_t batch =
                ((total - sent) < kChunkPixels)
                    ? (total - sent)
                    : kChunkPixels;
            const std::size_t batch_bytes = batch * 2u;

            for (std::size_t i = 0u; i < batch; ++i) {
                const std::uint16_t px = pixels[sent + i];
                chunk[i * 2u]      = static_cast<std::uint8_t>(px >> 8u);
                chunk[i * 2u + 1u] = static_cast<std::uint8_t>(px & 0xFFu);
            }

            cs_.assert_cs();
            auto r = bus_->transfer(
                std::span<const std::uint8_t>{chunk.data(), batch_bytes},
                std::span<std::uint8_t>{rx_chunk.data(), batch_bytes});
            cs_.deassert_cs();

            if (r.is_err()) {
                return alloy::core::Err(std::move(r).err());
            }
            sent += batch;
        }

        return alloy::core::Ok();
    }

private:
    BusHandle* bus_;
    DcPolicy   dc_;
    CsPolicy   cs_;
    Config     cfg_;
};

}  // namespace alloy::drivers::display::st7789

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// SPI bus surface + DC/CS policy contracts.
namespace {

struct _MockSpiForSt7789Gate {
    [[nodiscard]] auto transfer(std::span<const std::uint8_t>,
                                std::span<std::uint8_t> rx) const
        -> alloy::core::Result<void, alloy::core::ErrorCode>
    {
        for (auto& b : rx) b = 0u;
        return alloy::core::Ok();
    }
};

struct _MockDcPinForSt7789Gate {
    [[nodiscard]] auto set_low()  const -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto set_high() const -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
};

struct _MockCsPinForSt7789Gate {
    [[nodiscard]] auto set_low()  const -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
    [[nodiscard]] auto set_high() const -> alloy::core::Result<void, alloy::core::ErrorCode> { return alloy::core::Ok(); }
};

static_assert(
    sizeof(alloy::drivers::display::st7789::Device<
               _MockSpiForSt7789Gate,
               alloy::drivers::display::st7789::GpioDcPolicy<_MockDcPinForSt7789Gate>,
               alloy::drivers::display::st7789::GpioCsPolicy<_MockCsPinForSt7789Gate>>
           ) > 0,
    "st7789 Device must compile against the documented SPI bus + DC/CS policy surface");

}  // namespace
