// Bosch BME280 combined temperature / pressure / humidity sensor over I2C.
//
// Constrained ONLY on the alloy bus concepts — no chip, no family, no board, no
// register address baked into any call site the framework owns. Behavior from
// the Bosch BME280 datasheet (BST-BME280-DS002, rev 1.6): the driver reads the
// chip-id at 0xD0 (must be 0x60), slurps the factory trimming/calibration words
// (0x88.., 0xA1, 0xE1..), then for each measurement writes ctrl_hum (0xF2) and
// ctrl_meas (0xF4) to trigger a single forced-mode conversion, waits the worst
// case conversion time, burst-reads the eight data bytes at 0xF7, and applies
// the datasheet's integer compensation.
//
// Compensation is INTEGER FIXED-POINT (the datasheet's *_int32 / *_int64 paths,
// bit-exact to Bosch's reference source). The struct exposes the physical units
// as float purely at the boundary:
//   temperature_c  = T_fixed / 100      (0.01 °C resolution)
//   pressure_pa    = P_fixed / 256      (Q24.8 Pa)
//   humidity_pct   = H_fixed / 1024     (Q22.10 %RH)
// A NACK or a wrong chip-id yields valid = false rather than garbage.

#pragma once

#include <cstdint>
#include <span>

#include "alloy/concepts.hpp"

namespace alloy::lib {

template <class Bus, class Delay>
    requires alloy::I2cBus<Bus> && alloy::DelayNs<Delay>
class bme280 {
public:
    static constexpr std::uint8_t addr_primary = 0x76;  // SDO tied to GND
    static constexpr std::uint8_t addr_alt = 0x77;       // SDO tied to VDD
    static constexpr std::uint8_t chip_id = 0x60;        // fixed BME280 id byte

    struct reading {
        float temperature_c = 0.0f;
        float pressure_pa = 0.0f;
        float humidity_pct = 0.0f;
        bool valid = false;  // false on NACK or failed init — never trust values
    };

    constexpr bme280(const Bus& bus, Delay& delay, std::uint8_t address = addr_primary)
        : bus_(bus), delay_(delay), addr_(address) {}
    bme280(const Bus&&, Delay&, std::uint8_t = addr_primary) = delete;  // no dangling bus

    // Verify the chip-id and latch the factory calibration. Returns false on any
    // NACK or if the id byte is not 0x60. Must succeed before measure() is valid.
    [[nodiscard]] bool init() {
        std::uint8_t id = 0;
        if (!read_regs(kRegChipId, {&id, 1}) || id != chip_id) {
            return false;
        }
        std::uint8_t c0[26] = {};  // 0x88..0xA1: temp + pressure words, then H1
        std::uint8_t c1[7] = {};    // 0xE1..0xE7: remaining humidity words
        if (!read_regs(kRegCalib00, c0) || !read_regs(kRegCalib26, c1)) {
            return false;
        }
        cal_.t1 = u16(c0[0], c0[1]);
        cal_.t2 = s16(c0[2], c0[3]);
        cal_.t3 = s16(c0[4], c0[5]);
        cal_.p1 = u16(c0[6], c0[7]);
        cal_.p2 = s16(c0[8], c0[9]);
        cal_.p3 = s16(c0[10], c0[11]);
        cal_.p4 = s16(c0[12], c0[13]);
        cal_.p5 = s16(c0[14], c0[15]);
        cal_.p6 = s16(c0[16], c0[17]);
        cal_.p7 = s16(c0[18], c0[19]);
        cal_.p8 = s16(c0[20], c0[21]);
        cal_.p9 = s16(c0[22], c0[23]);
        // c0[24] is reserved (0xA0); c0[25] is dig_H1.
        cal_.h1 = c0[25];
        cal_.h2 = s16(c1[0], c1[1]);
        cal_.h3 = c1[2];
        // dig_H4/H5 are 12-bit values split across the 0xE4..0xE6 nibbles.
        cal_.h4 = static_cast<std::int16_t>(
            (static_cast<std::int16_t>(c1[3]) << 4) | (c1[4] & 0x0F));
        cal_.h5 = static_cast<std::int16_t>(
            (static_cast<std::int16_t>(c1[5]) << 4) | (c1[4] >> 4));
        cal_.h6 = static_cast<std::int8_t>(c1[6]);
        ready_ = true;
        return true;
    }

    // Trigger one forced-mode measurement (oversampling x1 on all three axes),
    // wait the worst-case conversion time, read and compensate. valid == false
    // on a NACK or if init() has not latched the calibration.
    [[nodiscard]] reading measure() const {
        if (!ready_) {
            return {};
        }
        const std::uint8_t hum[2] = {kRegCtrlHum, kOsrsX1};
        const std::uint8_t meas[2] = {kRegCtrlMeas, kCtrlMeasForced};
        if (!bus_.write(addr_, hum) || !bus_.write(addr_, meas)) {
            return {};
        }
        delay_.delay_ns(kConversionNs);
        std::uint8_t raw[8] = {};
        if (!read_regs(kRegData, raw)) {
            return {};
        }
        const std::int32_t adc_p = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[0]) << 12) |
            (static_cast<std::uint32_t>(raw[1]) << 4) | (raw[2] >> 4));
        const std::int32_t adc_t = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[3]) << 12) |
            (static_cast<std::uint32_t>(raw[4]) << 4) | (raw[5] >> 4));
        const std::int32_t adc_h = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[6]) << 8) | raw[7]);

        std::int32_t t_fine = 0;
        reading r;
        r.temperature_c = static_cast<float>(compensate_t(adc_t, t_fine)) / 100.0f;
        r.pressure_pa = static_cast<float>(compensate_p(adc_p, t_fine)) / 256.0f;
        r.humidity_pct = static_cast<float>(compensate_h(adc_h, t_fine)) / 1024.0f;
        r.valid = true;
        return r;
    }

    // Soft reset (write 0xB6 to 0xE0). Returns false on NACK. Calibration must
    // be re-latched with init() afterwards.
    [[nodiscard]] bool reset() {
        const std::uint8_t cmd[2] = {kRegReset, kResetWord};
        ready_ = false;
        return bus_.write(addr_, cmd);
    }

private:
    static constexpr std::uint8_t kRegChipId = 0xD0;
    static constexpr std::uint8_t kRegReset = 0xE0;
    static constexpr std::uint8_t kRegCalib00 = 0x88;  // 0x88..0xA1 (26 bytes)
    static constexpr std::uint8_t kRegCalib26 = 0xE1;   // 0xE1..0xE7 (7 bytes)
    static constexpr std::uint8_t kRegCtrlHum = 0xF2;
    static constexpr std::uint8_t kRegCtrlMeas = 0xF4;
    static constexpr std::uint8_t kRegData = 0xF7;      // burst: press/temp/hum

    static constexpr std::uint8_t kResetWord = 0xB6;
    static constexpr std::uint8_t kOsrsX1 = 0x01;  // ctrl_hum: osrs_h = x1
    // ctrl_meas: osrs_t(x1)=001, osrs_p(x1)=001, mode=forced(01).
    static constexpr std::uint8_t kCtrlMeasForced =
        static_cast<std::uint8_t>((1u << 5) | (1u << 2) | 0x01u);
    // Worst-case forced conversion time at x1 on all axes (~9.3 ms), rounded up.
    static constexpr std::uint32_t kConversionNs = 10'000'000u;

    struct calib {
        std::uint16_t t1 = 0;
        std::int16_t t2 = 0, t3 = 0;
        std::uint16_t p1 = 0;
        std::int16_t p2 = 0, p3 = 0, p4 = 0, p5 = 0, p6 = 0, p7 = 0, p8 = 0, p9 = 0;
        std::uint8_t h1 = 0, h3 = 0;
        std::int16_t h2 = 0, h4 = 0, h5 = 0;
        std::int8_t h6 = 0;
    };

    static constexpr std::uint16_t u16(std::uint8_t lo, std::uint8_t hi) {
        return static_cast<std::uint16_t>(lo | (static_cast<std::uint16_t>(hi) << 8));
    }
    static constexpr std::int16_t s16(std::uint8_t lo, std::uint8_t hi) {
        return static_cast<std::int16_t>(u16(lo, hi));
    }

    // Read `dst.size()` bytes starting at register `reg` (write-then-read).
    [[nodiscard]] bool read_regs(std::uint8_t reg, std::span<std::uint8_t> dst) const {
        const std::uint8_t r = reg;
        return bus_.write_read(addr_, {&r, 1}, dst);
    }

    // Datasheet BME280_compensate_T_int32: returns °C * 100; sets t_fine.
    std::int32_t compensate_t(std::int32_t adc_t, std::int32_t& t_fine) const {
        const std::int32_t t1 = cal_.t1;
        std::int32_t var1 = (((adc_t >> 3) - (t1 << 1)) * cal_.t2) >> 11;
        std::int32_t var2 =
            ((((adc_t >> 4) - t1) * ((adc_t >> 4) - t1)) >> 12) * cal_.t3 >> 14;
        t_fine = var1 + var2;
        return (t_fine * 5 + 128) >> 8;
    }

    // Datasheet BME280_compensate_P_int64: returns Pa in Q24.8 (Pa * 256).
    std::uint32_t compensate_p(std::int32_t adc_p, std::int32_t t_fine) const {
        std::int64_t var1 = static_cast<std::int64_t>(t_fine) - 128000;
        std::int64_t var2 = var1 * var1 * cal_.p6;
        var2 += (var1 * cal_.p5) << 17;
        var2 += static_cast<std::int64_t>(cal_.p4) << 35;
        var1 = ((var1 * var1 * cal_.p3) >> 8) + ((var1 * cal_.p2) << 12);
        var1 = (((static_cast<std::int64_t>(1) << 47) + var1) * cal_.p1) >> 33;
        if (var1 == 0) {
            return 0;  // avoid division by zero
        }
        std::int64_t p = 1048576 - adc_p;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (static_cast<std::int64_t>(cal_.p9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (static_cast<std::int64_t>(cal_.p8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (static_cast<std::int64_t>(cal_.p7) << 4);
        return static_cast<std::uint32_t>(p);
    }

    // Datasheet bme280_compensate_H_int32: returns %RH in Q22.10 (%RH * 1024).
    std::uint32_t compensate_h(std::int32_t adc_h, std::int32_t t_fine) const {
        std::int32_t v = t_fine - 76800;
        v = (((((adc_h << 14) - (static_cast<std::int32_t>(cal_.h4) << 20) -
                (cal_.h5 * v)) +
               16384) >>
              15) *
             (((((((v * cal_.h6) >> 10) *
                  (((v * cal_.h3) >> 11) + 32768)) >>
                 10) +
                2097152) *
                   cal_.h2 +
               8192) >>
              14));
        v -= (((((v >> 15) * (v >> 15)) >> 7) * static_cast<std::int32_t>(cal_.h1)) >> 4);
        v = v < 0 ? 0 : v;
        v = v > 419430400 ? 419430400 : v;
        return static_cast<std::uint32_t>(v >> 12);
    }

    const Bus& bus_;
    Delay& delay_;
    std::uint8_t addr_;
    bool ready_ = false;
    calib cal_{};
};

}  // namespace alloy::lib
