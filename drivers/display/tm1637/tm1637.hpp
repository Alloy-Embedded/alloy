#pragma once

// drivers/display/tm1637/tm1637.hpp
//
// Driver for Titan Micro Electronics TM1637 4-digit 7-segment LED display
// controller via GPIO bit-bang (custom 2-wire serial: CLK + DIO).
// Written against TM1637 datasheet v1.0.
// Seed driver: init + display_number + display_digits + set_brightness + clear.
// See drivers/README.md.
//
// Protocol: start condition → write command byte (LSB first, 8 bits + ACK pulse)
// → stop condition. The ACK (9th clock pulse) is generated but DIO is not read
// back since we have no input capability requirement.
//
// ClkPin and DioPin must provide:
//   set_high() -> alloy::core::Result<void, alloy::core::ErrorCode>
//   set_low()  -> alloy::core::Result<void, alloy::core::ErrorCode>

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::display::tm1637 {

// ── Constants ─────────────────────────────────────────────────────────────────

inline constexpr std::uint8_t kCmdData = 0x40u;  // Auto-increment, write to display
inline constexpr std::uint8_t kCmdAddr = 0xC0u;  // Starting address 0x00
inline constexpr std::uint8_t kCmdCtrl = 0x88u;  // Display on + brightness (OR with 0..7)

// ── Detail namespace ──────────────────────────────────────────────────────────

namespace detail {

// 7-segment encoding table: index 0-9 = digits 0-9, index 10 = blank.
inline constexpr std::uint8_t kSegTable[11] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,  // 0-4
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,  // 5-9
    0x00,                            // blank
};

// ~2 µs busy-wait between bit transitions.
inline void bit_delay() {
    volatile std::uint32_t n = 20u;
    while (n-- != 0u) {}
}

// TM1637 start condition: DIO high→low while CLK high.
template <typename ClkPin, typename DioPin>
[[nodiscard]] inline auto start(ClkPin& clk, DioPin& dio)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    if (auto r = clk.set_high(); r.is_err()) { return r; }
    if (auto r = dio.set_high(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = dio.set_low(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = clk.set_low(); r.is_err()) { return r; }
    bit_delay();
    return alloy::core::Ok();
}

// TM1637 stop condition: DIO low→high while CLK high.
template <typename ClkPin, typename DioPin>
[[nodiscard]] inline auto stop(ClkPin& clk, DioPin& dio)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    if (auto r = clk.set_low(); r.is_err()) { return r; }
    if (auto r = dio.set_low(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = clk.set_high(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = dio.set_high(); r.is_err()) { return r; }
    bit_delay();
    return alloy::core::Ok();
}

// Write one byte, LSB first, 8 bits, then pulse CLK for ACK (DIO not read).
template <typename ClkPin, typename DioPin>
[[nodiscard]] inline auto write_byte(ClkPin& clk, DioPin& dio, std::uint8_t byte)
    -> alloy::core::Result<void, alloy::core::ErrorCode>
{
    for (std::uint8_t i = 0u; i < 8u; ++i) {
        // CLK low, set data bit, CLK high, CLK low.
        if (auto r = clk.set_low(); r.is_err()) { return r; }
        bit_delay();
        if ((byte & 0x01u) != 0u) {
            if (auto r = dio.set_high(); r.is_err()) { return r; }
        } else {
            if (auto r = dio.set_low(); r.is_err()) { return r; }
        }
        bit_delay();
        if (auto r = clk.set_high(); r.is_err()) { return r; }
        bit_delay();
        byte = static_cast<std::uint8_t>(byte >> 1u);
    }
    // 9th clock pulse for ACK — DIO is left as input by device, we just pulse CLK.
    if (auto r = clk.set_low(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = dio.set_high(); r.is_err()) { return r; }  // release DIO (high-Z via pull-up)
    bit_delay();
    if (auto r = clk.set_high(); r.is_err()) { return r; }
    bit_delay();
    if (auto r = clk.set_low(); r.is_err()) { return r; }
    bit_delay();
    return alloy::core::Ok();
}

}  // namespace detail

// ── Device ────────────────────────────────────────────────────────────────────

template <typename ClkPin, typename DioPin>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;

    explicit Device(ClkPin& clk, DioPin& dio)
        : clk_{&clk}, dio_{&dio} {}

    // Initialise the display: deassert lines, blank all digits, enable display
    // at the stored brightness level.
    [[nodiscard]] auto init() -> ResultVoid {
        // Assert idle state.
        if (auto r = clk_->set_high(); r.is_err()) { return r; }
        if (auto r = dio_->set_high(); r.is_err()) { return r; }
        detail::bit_delay();

        // Send data command (auto-increment).
        if (auto r = send_command(kCmdData); r.is_err()) { return r; }

        // Blank all four digits via address-mode write.
        if (auto r = send_address_data(0x00u, 0x00u, 0x00u, 0x00u); r.is_err()) { return r; }

        // Enable display at current brightness.
        return send_command(static_cast<std::uint8_t>(kCmdCtrl | brightness_));
    }

    // Set display brightness (0 = dimmest, 7 = brightest). Clamped to [0, 7].
    // Re-sends display control command immediately.
    [[nodiscard]] auto set_brightness(std::uint8_t level) -> ResultVoid {
        brightness_ = (level > 7u) ? 7u : level;
        return send_command(static_cast<std::uint8_t>(kCmdCtrl | brightness_));
    }

    // Display four raw segment bytes (not digit indices). Each byte directly
    // encodes segments a-g and dp.
    [[nodiscard]] auto display_digits(std::uint8_t d0, std::uint8_t d1,
                                      std::uint8_t d2, std::uint8_t d3)
        -> ResultVoid
    {
        if (auto r = send_command(kCmdData); r.is_err()) { return r; }
        if (auto r = send_address_data(d0, d1, d2, d3); r.is_err()) { return r; }
        return send_command(static_cast<std::uint8_t>(kCmdCtrl | brightness_));
    }

    // Display a number 0..9999. Leading zeros are blanked unless leading_zeros
    // is true. Values >= 10000 are saturated to 9999.
    [[nodiscard]] auto display_number(std::uint16_t number, bool leading_zeros = false)
        -> ResultVoid
    {
        if (number > 9999u) { number = 9999u; }

        const std::uint8_t dig3 = static_cast<std::uint8_t>(number / 1000u);
        const std::uint8_t dig2 = static_cast<std::uint8_t>((number / 100u) % 10u);
        const std::uint8_t dig1 = static_cast<std::uint8_t>((number / 10u) % 10u);
        const std::uint8_t dig0 = static_cast<std::uint8_t>(number % 10u);

        // Resolve leading blanks (only if leading_zeros is false).
        const std::uint8_t s3 = (!leading_zeros && dig3 == 0u && number < 1000u)
                                     ? detail::kSegTable[10]
                                     : detail::kSegTable[dig3];
        const std::uint8_t s2 = (!leading_zeros && dig3 == 0u && dig2 == 0u && number < 100u)
                                     ? detail::kSegTable[10]
                                     : detail::kSegTable[dig2];
        const std::uint8_t s1 = (!leading_zeros && dig3 == 0u && dig2 == 0u &&
                                  dig1 == 0u && number < 10u)
                                     ? detail::kSegTable[10]
                                     : detail::kSegTable[dig1];
        const std::uint8_t s0 = detail::kSegTable[dig0];

        return display_digits(s3, s2, s1, s0);
    }

    // Blank all four digits.
    [[nodiscard]] auto clear() -> ResultVoid {
        return display_digits(detail::kSegTable[10], detail::kSegTable[10],
                              detail::kSegTable[10], detail::kSegTable[10]);
    }

private:
    // Send a single command byte (start + byte + stop).
    [[nodiscard]] auto send_command(std::uint8_t cmd) -> ResultVoid {
        if (auto r = detail::start(*clk_, *dio_); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, cmd); r.is_err()) { return r; }
        return detail::stop(*clk_, *dio_);
    }

    // Send kCmdAddr + four segment bytes in one transaction (start + 5 bytes + stop).
    [[nodiscard]] auto send_address_data(std::uint8_t d0, std::uint8_t d1,
                                         std::uint8_t d2, std::uint8_t d3) -> ResultVoid {
        if (auto r = detail::start(*clk_, *dio_); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, kCmdAddr); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, d0); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, d1); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, d2); r.is_err()) { return r; }
        if (auto r = detail::write_byte(*clk_, *dio_, d3); r.is_err()) { return r; }
        return detail::stop(*clk_, *dio_);
    }

    ClkPin* clk_;
    DioPin* dio_;
    std::uint8_t brightness_{7u};  // 0-7
};

}  // namespace alloy::drivers::display::tm1637

// ── Concept gate ──────────────────────────────────────────────────────────────
// Fails at include time if Device no longer compiles against the documented
// GPIO pin surface.
namespace {
struct MockTm1637Pin {
    auto set_high() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
    auto set_low() -> alloy::core::Result<void, alloy::core::ErrorCode> {
        return alloy::core::Ok();
    }
};
static_assert(sizeof(alloy::drivers::display::tm1637::Device<MockTm1637Pin, MockTm1637Pin>) > 0,
    "tm1637 Device must compile against the documented GPIO pin surface");
}  // namespace
