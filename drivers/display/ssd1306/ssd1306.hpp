#pragma once

// drivers/display/ssd1306/ssd1306.hpp
//
// Driver for Solomon Systech SSD1306 128x64 OLED over I2C.
// Written against datasheet revision 1.1 (April 2008).
// Seed driver: covers init + clear + per-pixel draw + 5x7 text + flush.
// Commands are issued via the "Co=0, D/C#=0" control byte (0x00) and framebuffer
// data is issued via the "Co=0, D/C#=1" control byte (0x40). See drivers/README.md.

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "core/error_code.hpp"
#include "core/result.hpp"

#include "drivers/display/ssd1306/font_5x7.hpp"

namespace alloy::drivers::display::ssd1306 {

inline constexpr std::uint16_t kDefaultAddress = 0x3C;
inline constexpr std::uint16_t kWidthPixels = 128;
inline constexpr std::uint16_t kHeightPixels = 64;
inline constexpr std::uint16_t kPageCount = kHeightPixels / 8;
inline constexpr std::size_t kFramebufferBytes = kWidthPixels * kPageCount;

struct Config {
    std::uint16_t address = kDefaultAddress;
    bool external_vcc = false;  // Charge pump enabled when false (typical).
    bool flip_horizontal = false;
    bool flip_vertical = false;
};

template <typename BusHandle>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    [[nodiscard]] auto init() -> ResultVoid {
        const std::uint8_t remap = cfg_.flip_horizontal ? 0xA0 : 0xA1;
        const std::uint8_t com_dir = cfg_.flip_vertical ? 0xC0 : 0xC8;
        const std::uint8_t pump = cfg_.external_vcc ? 0x10 : 0x14;

        const std::array<std::uint8_t, 26> init_seq{
            0xAE,         // Display OFF
            0xD5, 0x80,   // Clock divide / oscillator
            0xA8, 0x3F,   // Multiplex ratio = 63 (64 rows)
            0xD3, 0x00,   // Display offset 0
            0x40,         // Start line 0
            0x8D, pump,   // Charge pump
            0x20, 0x00,   // Horizontal addressing mode
            remap,        // Segment remap
            com_dir,      // COM output scan direction
            0xDA, 0x12,   // COM pins hardware config (128x64)
            0x81, 0xCF,   // Contrast
            0xD9, 0xF1,   // Pre-charge period
            0xDB, 0x40,   // VCOMH deselect
            0xA4,         // Display all on = resume to RAM
            0xA6,         // Normal (non-inverted)
            0x2E,         // Deactivate scroll
            0xAF,         // Display ON
        };

        if (auto r = send_command_sequence(init_seq); r.is_err()) {
            return r;
        }
        clear();
        return flush();
    }

    void clear() { framebuffer_.fill(0); }

    [[nodiscard]] auto draw_pixel(std::uint16_t x, std::uint16_t y, bool on)
        -> ResultVoid {
        if (x >= kWidthPixels || y >= kHeightPixels) {
            return alloy::core::Err(alloy::core::ErrorCode::OutOfRange);
        }
        const std::size_t page = y / 8;
        const std::uint8_t bit = static_cast<std::uint8_t>(1u << (y % 8));
        const std::size_t idx = page * kWidthPixels + x;
        if (on) {
            framebuffer_[idx] = static_cast<std::uint8_t>(framebuffer_[idx] | bit);
        } else {
            framebuffer_[idx] = static_cast<std::uint8_t>(framebuffer_[idx] & ~bit);
        }
        return alloy::core::Ok();
    }

    // Writes ASCII text using the bundled 5x7 font. Each glyph consumes 6 columns
    // (5 pixels + 1 spacing) and 8 vertical pixels. (x, y) is the top-left corner.
    // Returns OutOfRange if any glyph would fall off the display.
    [[nodiscard]] auto draw_text(std::uint16_t x, std::uint16_t y,
                                 std::string_view text) -> ResultVoid {
        if (y + kFontGlyphHeight >= kHeightPixels) {
            return alloy::core::Err(alloy::core::ErrorCode::OutOfRange);
        }
        std::uint16_t cursor = x;
        for (char c : text) {
            if (cursor + kFontGlyphWidth > kWidthPixels) {
                return alloy::core::Err(alloy::core::ErrorCode::OutOfRange);
            }
            const std::uint8_t* glyph = font_glyph(c);
            for (std::uint8_t col = 0; col < kFontGlyphWidth; ++col) {
                const std::uint8_t bits = glyph[col];
                for (std::uint8_t row = 0; row < kFontGlyphHeight; ++row) {
                    const bool on = ((bits >> row) & 0x01) != 0;
                    if (auto r = draw_pixel(static_cast<std::uint16_t>(cursor + col),
                                            static_cast<std::uint16_t>(y + row), on);
                        r.is_err()) {
                        return r;
                    }
                }
            }
            cursor = static_cast<std::uint16_t>(cursor + kFontGlyphWidth + 1);
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto flush() -> ResultVoid {
        const std::array<std::uint8_t, 6> window{
            0x21, 0x00, 0x7F,  // column address 0..127
            0x22, 0x00, 0x07,  // page address 0..7
        };
        if (auto r = send_command_sequence(window); r.is_err()) {
            return r;
        }
        return send_data(framebuffer_);
    }

    [[nodiscard]] auto framebuffer() -> std::span<std::uint8_t> {
        return std::span<std::uint8_t>{framebuffer_};
    }

private:
    [[nodiscard]] auto send_command_sequence(std::span<const std::uint8_t> cmds)
        -> ResultVoid {
        // SSD1306 accepts multi-command payloads by repeating the "Co=0, D/C#=0"
        // control byte pattern; we split the stream into single-command writes to
        // avoid depending on Co=1 and to keep each I2C transaction small.
        std::array<std::uint8_t, 2> frame{0x00, 0x00};
        for (std::uint8_t c : cmds) {
            frame[1] = c;
            if (auto r = bus_->write(cfg_.address, frame); r.is_err()) {
                return r;
            }
        }
        return alloy::core::Ok();
    }

    [[nodiscard]] auto send_data(std::span<const std::uint8_t> data) -> ResultVoid {
        // Prefix 0x40 (Co=0, D/C#=1) + framebuffer. Using a single large write so
        // the I2C backend issues one addressed transaction.
        std::array<std::uint8_t, kFramebufferBytes + 1> payload{};
        payload[0] = 0x40;
        std::memcpy(payload.data() + 1, data.data(), data.size());
        return bus_->write(cfg_.address,
                           std::span<const std::uint8_t>{payload.data(), data.size() + 1});
    }

    BusHandle* bus_;
    Config cfg_;
    std::array<std::uint8_t, kFramebufferBytes> framebuffer_{};
};

}  // namespace alloy::drivers::display::ssd1306
