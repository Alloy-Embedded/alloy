// ROHM BH1750FVI ambient-light sensor over I2C.
//
// Constrained ONLY on the alloy bus concepts — no chip, no family, no board.
// Behavior from the ROHM BH1750FVI datasheet (Rev.D, Nov 2011): after
// power-on (opcode 0x01) the part is told to run a Continuously H-Resolution
// measurement (opcode 0x10), which takes typ 120 ms / max 180 ms; the result
// is then read back as two bytes, MSB first. Illuminance is the raw 16-bit
// count divided by the sensor's 1.2 counts-per-lux factor, giving lux.
//
// A NACK on any transfer marks the reading invalid rather than returning a
// garbage value.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus, class Delay>
    requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>
class bh1750 {
public:
    static constexpr std::uint8_t addr_low = 0x23;   // ADDR pin low  (default)
    static constexpr std::uint8_t addr_high = 0x5C;  // ADDR pin high (alt)

    struct reading {
        float lux = 0.0f;
        bool valid = false;  // false on NACK — never trust the value
    };

    constexpr bh1750(const Bus& bus, Delay& delay, std::uint8_t address = addr_low)
        : bus_(bus), delay_(delay), addr_(address) {}
    bh1750(const Bus&&, Delay&, std::uint8_t = addr_low) = delete;  // no dangling bus

    // Power up, run one continuous high-resolution measurement, read it back.
    [[nodiscard]] reading measure() const {
        static constexpr std::uint8_t power_on[1] = {kPowerOn};
        static constexpr std::uint8_t cont_hires[1] = {kContHiRes};
        if (!bus_.write(addr_, power_on)) {
            return {};
        }
        if (!bus_.write(addr_, cont_hires)) {
            return {};
        }
        delay_.delay_ns(kConversionNs);
        std::uint8_t raw[2] = {};
        if (!bus_.read(addr_, raw)) {
            return {};
        }
        const auto counts = static_cast<std::uint16_t>((raw[0] << 8) | raw[1]);
        reading r;
        r.lux = static_cast<float>(counts) / 1.2f;  // 1.2 counts per lux (H-res)
        r.valid = true;
        return r;
    }

    // Send the power-down opcode (0x00). Returns false on NACK.
    [[nodiscard]] bool power_down() const {
        static constexpr std::uint8_t cmd[1] = {kPowerDown};
        return bus_.write(addr_, cmd);
    }

private:
    static constexpr std::uint8_t kPowerDown = 0x00;
    static constexpr std::uint8_t kPowerOn = 0x01;
    static constexpr std::uint8_t kContHiRes = 0x10;  // continuous H-resolution
    static constexpr std::uint32_t kConversionNs = 180'000'000u;  // 180 ms, max

    const Bus& bus_;
    Delay& delay_;
    std::uint8_t addr_;
};

}  // namespace alloy::lib
