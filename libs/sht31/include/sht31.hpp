// Sensirion SHT3x (SHT30/31/35) temperature + humidity sensor over I2C.
//
// Constrained ONLY on the alloy bus concepts — no chip, no family, no board, no
// register address baked into any call site the framework owns. Behavior from
// the Sensirion SHT3x datasheet (v6, May 2019): single-shot high-repeatability
// measurement (command 0x2400, clock stretching disabled) waits <= 15 ms, then
// reads 6 bytes — temp(2)+CRC(1), humidity(2)+CRC(1). Each pair is checked with
// the Sensirion CRC-8 (poly 0x31, init 0xFF) and a mismatch marks the reading
// invalid rather than returning a garbage value.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus, class Delay>
    requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>
class sht31 {
public:
    static constexpr std::uint8_t addr_low = 0x44;   // ADDR pin tied to GND
    static constexpr std::uint8_t addr_high = 0x45;  // ADDR pin tied to VDD

    struct reading {
        float temperature_c = 0.0f;
        float humidity_pct = 0.0f;
        bool valid = false;  // false on NACK or CRC mismatch — never trust values
    };

    constexpr sht31(const Bus& bus, Delay& delay, std::uint8_t address = addr_low)
        : bus_(bus), delay_(delay), addr_(address) {}
    sht31(const Bus&&, Delay&, std::uint8_t = addr_low) = delete;  // no dangling bus

    // One high-repeatability single-shot measurement (clock stretching off).
    [[nodiscard]] reading measure() const {
        static constexpr std::uint8_t cmd[2] = {0x24, 0x00};  // high rep, no stretch
        if (!bus_.write(addr_, cmd)) {
            return {};
        }
        delay_.delay_ns(kConversionNs);
        std::uint8_t raw[6] = {};
        if (!bus_.read(addr_, raw)) {
            return {};
        }
        if (crc8(raw[0], raw[1]) != raw[2] || crc8(raw[3], raw[4]) != raw[5]) {
            return {};
        }
        const auto t_raw = static_cast<std::uint16_t>((raw[0] << 8) | raw[1]);
        const auto h_raw = static_cast<std::uint16_t>((raw[3] << 8) | raw[4]);
        reading r;
        r.temperature_c = -45.0f + 175.0f * static_cast<float>(t_raw) / 65535.0f;
        r.humidity_pct = 100.0f * static_cast<float>(h_raw) / 65535.0f;
        r.valid = true;
        return r;
    }

    // Soft reset (0x30A2). Returns false on NACK.
    [[nodiscard]] bool reset() const {
        static constexpr std::uint8_t cmd[2] = {0x30, 0xA2};
        return bus_.write(addr_, cmd);
    }

private:
    static constexpr std::uint32_t kConversionNs = 15'000'000u;  // 15 ms, high-rep

    // Sensirion CRC-8: polynomial 0x31, init 0xFF, MSB-first, no final xor.
    static std::uint8_t crc8(std::uint8_t b0, std::uint8_t b1) {
        std::uint8_t crc = 0xFF;
        for (std::uint8_t byte : {b0, b1}) {
            crc ^= byte;
            for (int i = 0; i < 8; ++i) {
                crc = (crc & 0x80u) != 0u
                          ? static_cast<std::uint8_t>((crc << 1) ^ 0x31u)
                          : static_cast<std::uint8_t>(crc << 1);
            }
        }
        return crc;
    }

    const Bus& bus_;
    Delay& delay_;
    std::uint8_t addr_;
};

}  // namespace alloy::lib
