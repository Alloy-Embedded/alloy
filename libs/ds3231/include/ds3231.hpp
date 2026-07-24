// Maxim/Analog DS3231 high-accuracy real-time clock over I2C.
//
// Constrained ONLY on the alloy bus concepts — no chip, no family, no board.
// Behavior from the DS3231 datasheet (Rev 10): the timekeeping registers 0x00..
// 0x06 hold seconds/minutes/hours/day-of-week/date/month/year packed as BCD.
// This driver converts BCD<->binary at the register edge, so user code only ever
// sees the plain-binary alloy::datetime. The temperature register pair 0x11/0x12
// reports the die temperature as a 10-bit two's-complement value with a 0.25 C
// resolution (upper two bits of 0x12 are the fraction).

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus>
    requires alloy::I2cBus<Bus>
class ds3231 {
public:
    static constexpr std::uint8_t addr = 0x68;  // fixed 7-bit I2C address

    struct temperature {
        float celsius = 0.0f;
        bool valid = false;  // false on NACK / bus error — never trust the value
    };

    constexpr explicit ds3231(const Bus& bus) : bus_(bus) {}
    ds3231(const Bus&&) = delete;  // no dangling bus

    // Read the wall clock. On a bus error the returned datetime is the default
    // (all-zero time, day/month = 1) — callers that must distinguish an error
    // should use now(datetime&), which reports success explicitly.
    [[nodiscard]] alloy::datetime now() const {
        alloy::datetime dt;
        (void)now(dt);
        return dt;
    }

    // Read the wall clock, reporting bus success. dt is left untouched on error.
    [[nodiscard]] bool now(alloy::datetime& dt) const {
        static constexpr std::uint8_t ptr[1] = {reg_seconds};
        std::uint8_t raw[7] = {};
        if (!bus_.write_read(addr, ptr, raw)) {
            return false;
        }
        dt.second = from_bcd(raw[0] & 0x7Fu);         // bit 7 is unused
        dt.minute = from_bcd(raw[1] & 0x7Fu);
        dt.hour = from_bcd(raw[2] & 0x3Fu);           // 24-hour mode (bit 6 = 0)
        // raw[3] is day-of-week (1..7); alloy::datetime has no weekday field.
        dt.day = from_bcd(raw[4] & 0x3Fu);            // date, 1..31
        dt.month = from_bcd(raw[5] & 0x1Fu);          // bit 7 is the century flag
        dt.year = from_bcd(raw[6]);                   // 0..99
        return true;
    }

    // Set the wall clock. Forces 24-hour mode and clears the century bit; the
    // day-of-week register is written to 1 (unused by alloy::datetime). Returns
    // false on NACK / bus error.
    [[nodiscard]] bool set(const alloy::datetime& dt) const {
        const std::uint8_t frame[8] = {
            reg_seconds,
            to_bcd(dt.second),
            to_bcd(dt.minute),
            to_bcd(dt.hour),   // bit 6 = 0 -> 24-hour mode
            0x01u,             // day-of-week, unused
            to_bcd(dt.day),
            to_bcd(dt.month),  // bit 7 = 0 -> century 20xx
            to_bcd(dt.year),
        };
        return bus_.write(addr, frame);
    }

    // Read the die temperature (10-bit, 0.25 C/LSB). valid = false on bus error.
    [[nodiscard]] temperature read_temperature_c() const {
        static constexpr std::uint8_t ptr[1] = {reg_temp_msb};
        std::uint8_t raw[2] = {};
        if (!bus_.write_read(addr, ptr, raw)) {
            return {};
        }
        // 0x11 is the signed integer part; the top two bits of 0x12 are the
        // fraction in quarter-degree steps. Sign-extend the 8-bit integer part.
        const auto msb = static_cast<std::int8_t>(raw[0]);
        const std::uint8_t frac = static_cast<std::uint8_t>(raw[1] >> 6);
        temperature t;
        t.celsius = static_cast<float>(msb) + 0.25f * static_cast<float>(frac);
        t.valid = true;
        return t;
    }

private:
    static constexpr std::uint8_t reg_seconds = 0x00;
    static constexpr std::uint8_t reg_temp_msb = 0x11;

    static constexpr std::uint8_t to_bcd(std::uint8_t v) {
        return static_cast<std::uint8_t>(((v / 10u) << 4) | (v % 10u));
    }
    static constexpr std::uint8_t from_bcd(std::uint8_t v) {
        return static_cast<std::uint8_t>((v >> 4) * 10u + (v & 0x0Fu));
    }

    const Bus& bus_;
};

}  // namespace alloy::lib
