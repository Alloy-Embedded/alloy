// Solomon Systech SSD1306 128x64 monochrome OLED display controller over I2C.
//
// Constrained ONLY on the alloy I2cBus concept — no chip, no family, no board.
// Behavior from the SSD1306 datasheet (Rev 1.1, 2008). Every I2C transfer to the
// controller is a control byte followed by a payload: control 0x00 marks a stream
// of COMMAND bytes, control 0x40 marks a stream of display-DATA (GDDRAM) bytes.
//
// The 128x64 panel is organized as 8 horizontal "pages", each page one byte tall
// (8 vertical pixels). In horizontal addressing mode GDDRAM is laid out
// column-major within a page: byte index = x + page*128, with bit (y & 7) of that
// byte holding pixel (x, y) — bit 0 is the top row of the page. The framebuffer
// is a plain member array (1024 bytes) so nothing touches the heap; flush()
// programs the column/page address window then streams it in fixed-size chunks.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus>
    requires alloy::I2cBus<Bus>
class ssd1306 {
public:
    static constexpr std::uint8_t addr_primary = 0x3C;    // SA0 tied low (default)
    static constexpr std::uint8_t addr_secondary = 0x3D;  // SA0 tied high

    static constexpr std::uint8_t width = 128;
    static constexpr std::uint8_t height = 64;
    static constexpr std::uint8_t pages = height / 8;  // 8 pages of 8px each
    static constexpr std::size_t fb_size =
        static_cast<std::size_t>(width) * height / 8;  // 1024 bytes

    constexpr explicit ssd1306(const Bus& bus, std::uint8_t address = addr_primary)
        : bus_(bus), addr_(address) {}
    ssd1306(const Bus&&, std::uint8_t = addr_primary) = delete;  // no dangling bus

    // Power-on configuration for the 128x64 panel with the internal charge pump.
    // Returns false on NACK — the caller must not draw to a display that failed.
    [[nodiscard]] bool init() const {
        static constexpr std::uint8_t seq[] = {
            0xAE,        // display off
            0xD5, 0x80,  // clock divide ratio / osc frequency
            0xA8, 0x3F,  // multiplex ratio = 63 (64 rows)
            0xD3, 0x00,  // display offset = 0
            0x40,        // display start line = 0
            0x8D, 0x14,  // charge pump: enable
            0x20, 0x00,  // memory addressing mode: horizontal
            0xA1,        // segment re-map (column 127 -> SEG0)
            0xC8,        // COM output scan direction: remapped
            0xDA, 0x12,  // COM pins hardware config: alternative
            0x81, 0xCF,  // contrast
            0xD9, 0xF1,  // pre-charge period
            0xDB, 0x40,  // VCOMH deselect level
            0xA4,        // resume display from GDDRAM
            0xA6,        // normal (non-inverted) display
            0xAF,        // display on
        };
        return send_commands(seq);
    }

    // Blank the framebuffer (does not touch the panel until flush()).
    void clear() {
        for (std::uint8_t& b : fb_) {
            b = 0;
        }
    }

    // Turn pixel (x, y) on or off in the framebuffer. Out-of-range is a no-op.
    void set_pixel(std::uint8_t x, std::uint8_t y, bool on) {
        if (x >= width || y >= height) {
            return;
        }
        const std::size_t idx =
            static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * width;
        const auto mask = static_cast<std::uint8_t>(1u << (y & 7u));
        if (on) {
            fb_[idx] = static_cast<std::uint8_t>(fb_[idx] | mask);
        } else {
            fb_[idx] = static_cast<std::uint8_t>(fb_[idx] & ~mask);
        }
    }

    // Read a pixel back out of the framebuffer (false when out of range).
    [[nodiscard]] bool get_pixel(std::uint8_t x, std::uint8_t y) const {
        if (x >= width || y >= height) {
            return false;
        }
        const std::size_t idx =
            static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * width;
        const auto mask = static_cast<std::uint8_t>(1u << (y & 7u));
        return (fb_[idx] & mask) != 0u;
    }

    // Program the full-panel address window and stream the framebuffer to GDDRAM.
    // Returns false on the first NACK.
    [[nodiscard]] bool flush() const {
        static constexpr std::uint8_t window[] = {
            0x21, 0x00, static_cast<std::uint8_t>(width - 1),   // column 0..127
            0x22, 0x00, static_cast<std::uint8_t>(pages - 1),   // page   0..7
        };
        if (!send_commands(window)) {
            return false;
        }
        std::uint8_t chunk[1 + kChunk];
        chunk[0] = 0x40;  // data control byte precedes each GDDRAM burst
        for (std::size_t off = 0; off < fb_size; off += kChunk) {
            const std::size_t n = (fb_size - off < kChunk) ? (fb_size - off) : kChunk;
            for (std::size_t i = 0; i < n; ++i) {
                chunk[1 + i] = fb_[off + i];
            }
            if (!bus_.write(addr_, std::span<const std::uint8_t>(chunk, 1 + n))) {
                return false;
            }
        }
        return true;
    }

private:
    static constexpr std::size_t kChunk = 16;  // GDDRAM burst size (keeps stack small)

    // Prepend the 0x00 command control byte and issue the batch in one transfer.
    template <std::size_t N>
    [[nodiscard]] bool send_commands(const std::uint8_t (&cmds)[N]) const {
        std::uint8_t buf[N + 1];
        buf[0] = 0x00;  // Co=0, D/C#=0 -> the rest of the stream is commands
        for (std::size_t i = 0; i < N; ++i) {
            buf[i + 1] = cmds[i];
        }
        return bus_.write(addr_, std::span<const std::uint8_t>(buf, N + 1));
    }

    const Bus& bus_;
    std::uint8_t addr_;
    std::uint8_t fb_[fb_size]{};  // framebuffer — member, no heap
};

}  // namespace alloy::lib
