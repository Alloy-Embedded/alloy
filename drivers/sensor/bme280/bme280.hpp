#pragma once

// drivers/sensor/bme280/bme280.hpp
//
// Driver for Bosch BME280 temperature / pressure / humidity sensor over I2C.
// Written against datasheet revision 1.6 (September 2018).
// Seed driver: chip-ID probe + soft reset + calibration load + one-shot burst
// read with the datasheet's int32 compensation formulas. No float math in the
// compensation path; the final conversion to `float` happens once at the end.
// See drivers/README.md.

#include <array>
#include <cstdint>
#include <span>
#include <utility>

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::drivers::sensor::bme280 {

inline constexpr std::uint16_t kPrimaryAddress = 0x76;    // SDO -> GND
inline constexpr std::uint16_t kSecondaryAddress = 0x77;  // SDO -> VCC
inline constexpr std::uint8_t kExpectedChipId = 0x60;

enum class Oversampling : std::uint8_t {
    Skipped = 0b000,
    X1 = 0b001,
    X2 = 0b010,
    X4 = 0b011,
    X8 = 0b100,
    X16 = 0b101,
};

enum class Mode : std::uint8_t {
    Sleep = 0b00,
    Forced = 0b01,
    Normal = 0b11,
};

enum class Filter : std::uint8_t {
    Off = 0b000,
    X2 = 0b001,
    X4 = 0b010,
    X8 = 0b011,
    X16 = 0b100,
};

struct Config {
    std::uint16_t address = kPrimaryAddress;
    Oversampling osrs_temperature = Oversampling::X1;
    Oversampling osrs_pressure = Oversampling::X1;
    Oversampling osrs_humidity = Oversampling::X1;
    Filter filter = Filter::Off;
    Mode mode = Mode::Normal;
};

struct Measurement {
    float temperature_c;
    float pressure_pa;
    float humidity_pct;
};

namespace detail {

inline constexpr std::uint8_t kRegChipId = 0xD0;
inline constexpr std::uint8_t kRegReset = 0xE0;
inline constexpr std::uint8_t kRegCtrlHum = 0xF2;
inline constexpr std::uint8_t kRegCtrlMeas = 0xF4;
inline constexpr std::uint8_t kRegConfig = 0xF5;
inline constexpr std::uint8_t kRegData = 0xF7;
inline constexpr std::uint8_t kRegCalibT = 0x88;  // 0x88..0xA1 (26 bytes)
inline constexpr std::uint8_t kRegCalibH = 0xE1;  // 0xE1..0xE7 (7 bytes)
inline constexpr std::uint8_t kResetMagic = 0xB6;

struct Calibration {
    std::uint16_t dig_T1;
    std::int16_t dig_T2;
    std::int16_t dig_T3;

    std::uint16_t dig_P1;
    std::int16_t dig_P2;
    std::int16_t dig_P3;
    std::int16_t dig_P4;
    std::int16_t dig_P5;
    std::int16_t dig_P6;
    std::int16_t dig_P7;
    std::int16_t dig_P8;
    std::int16_t dig_P9;

    std::uint8_t dig_H1;
    std::int16_t dig_H2;
    std::uint8_t dig_H3;
    std::int16_t dig_H4;
    std::int16_t dig_H5;
    std::int8_t dig_H6;
};

[[nodiscard]] inline auto u16_le(const std::uint8_t* p) -> std::uint16_t {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8);
}

[[nodiscard]] inline auto s16_le(const std::uint8_t* p) -> std::int16_t {
    return static_cast<std::int16_t>(u16_le(p));
}

// Temperature compensation (datasheet §4.2.3): returns temperature in 0.01 °C
// units (e.g. 5123 -> 51.23 °C) and writes t_fine for downstream formulas.
[[nodiscard]] inline auto compensate_temperature(std::int32_t adc_T,
                                                 const Calibration& c,
                                                 std::int32_t& t_fine) -> std::int32_t {
    const std::int32_t var1 =
        ((((adc_T >> 3) - (static_cast<std::int32_t>(c.dig_T1) << 1))) *
         (static_cast<std::int32_t>(c.dig_T2))) >>
        11;
    const std::int32_t var2 =
        (((((adc_T >> 4) - (static_cast<std::int32_t>(c.dig_T1))) *
           ((adc_T >> 4) - (static_cast<std::int32_t>(c.dig_T1)))) >>
          12) *
         (static_cast<std::int32_t>(c.dig_T3))) >>
        14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

// Pressure compensation (datasheet §4.2.3): returns pressure in Pa as Q24.8
// (i.e. integer Pa = value >> 8). Returns 0 if the divisor would be zero.
[[nodiscard]] inline auto compensate_pressure(std::int32_t adc_P,
                                              const Calibration& c,
                                              std::int32_t t_fine) -> std::uint32_t {
    std::int64_t var1 = static_cast<std::int64_t>(t_fine) - 128000;
    std::int64_t var2 = var1 * var1 * static_cast<std::int64_t>(c.dig_P6);
    var2 = var2 + ((var1 * static_cast<std::int64_t>(c.dig_P5)) << 17);
    var2 = var2 + (static_cast<std::int64_t>(c.dig_P4) << 35);
    var1 = ((var1 * var1 * static_cast<std::int64_t>(c.dig_P3)) >> 8) +
           ((var1 * static_cast<std::int64_t>(c.dig_P2)) << 12);
    var1 = (((static_cast<std::int64_t>(1) << 47) + var1) *
            static_cast<std::int64_t>(c.dig_P1)) >>
           33;
    if (var1 == 0) {
        return 0u;
    }
    std::int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (static_cast<std::int64_t>(c.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (static_cast<std::int64_t>(c.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (static_cast<std::int64_t>(c.dig_P7) << 4);
    return static_cast<std::uint32_t>(p);
}

// Humidity compensation (datasheet §4.2.3): returns RH in Q22.10 (i.e. %RH =
// value / 1024.0).
[[nodiscard]] inline auto compensate_humidity(std::int32_t adc_H,
                                              const Calibration& c,
                                              std::int32_t t_fine) -> std::uint32_t {
    std::int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - ((static_cast<std::int32_t>(c.dig_H4)) << 20) -
            (static_cast<std::int32_t>(c.dig_H5) * v)) +
           16384) >>
          15) *
         (((((((v * static_cast<std::int32_t>(c.dig_H6)) >> 10) *
              (((v * static_cast<std::int32_t>(c.dig_H3)) >> 11) + 32768)) >>
             10) +
            2097152) *
               static_cast<std::int32_t>(c.dig_H2) +
           8192) >>
          14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * static_cast<std::int32_t>(c.dig_H1)) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return static_cast<std::uint32_t>(v >> 12);
}

}  // namespace detail

template <typename BusHandle>
class Device {
public:
    using ResultVoid = alloy::core::Result<void, alloy::core::ErrorCode>;
    using ResultMeasurement = alloy::core::Result<Measurement, alloy::core::ErrorCode>;

    explicit Device(BusHandle& bus, Config cfg = {}) : bus_{&bus}, cfg_{cfg} {}

    [[nodiscard]] auto init() -> ResultVoid {
        std::array<std::uint8_t, 1> id{};
        if (auto r = read_registers(detail::kRegChipId, id); r.is_err()) {
            return r;
        }
        if (id[0] != kExpectedChipId) {
            return alloy::core::Err(alloy::core::ErrorCode::CommunicationError);
        }

        if (auto r = write_register(detail::kRegReset, detail::kResetMagic); r.is_err()) {
            return r;
        }

        if (auto r = load_calibration(); r.is_err()) {
            return r;
        }

        // ctrl_hum must be written before ctrl_meas per §5.4.3.
        if (auto r = write_register(detail::kRegCtrlHum,
                                    static_cast<std::uint8_t>(cfg_.osrs_humidity));
            r.is_err()) {
            return r;
        }

        const std::uint8_t ctrl_meas =
            static_cast<std::uint8_t>((static_cast<std::uint8_t>(cfg_.osrs_temperature) << 5) |
                                      (static_cast<std::uint8_t>(cfg_.osrs_pressure) << 2) |
                                      static_cast<std::uint8_t>(cfg_.mode));
        if (auto r = write_register(detail::kRegCtrlMeas, ctrl_meas); r.is_err()) {
            return r;
        }

        const std::uint8_t config_val =
            static_cast<std::uint8_t>(static_cast<std::uint8_t>(cfg_.filter) << 2);
        return write_register(detail::kRegConfig, config_val);
    }

    [[nodiscard]] auto read() -> ResultMeasurement {
        std::array<std::uint8_t, 8> raw{};
        if (auto r = read_registers(detail::kRegData, raw); r.is_err()) {
            return alloy::core::Err(std::move(r).err());
        }

        const std::int32_t adc_P = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[0]) << 12) |
            (static_cast<std::uint32_t>(raw[1]) << 4) |
            (static_cast<std::uint32_t>(raw[2]) >> 4));
        const std::int32_t adc_T = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[3]) << 12) |
            (static_cast<std::uint32_t>(raw[4]) << 4) |
            (static_cast<std::uint32_t>(raw[5]) >> 4));
        const std::int32_t adc_H = static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw[6]) << 8) |
            static_cast<std::uint32_t>(raw[7]));

        std::int32_t t_fine = 0;
        const std::int32_t t = detail::compensate_temperature(adc_T, calib_, t_fine);
        const std::uint32_t p_q24_8 = detail::compensate_pressure(adc_P, calib_, t_fine);
        const std::uint32_t h_q22_10 = detail::compensate_humidity(adc_H, calib_, t_fine);

        Measurement m{};
        m.temperature_c = static_cast<float>(t) / 100.0f;
        m.pressure_pa = static_cast<float>(p_q24_8) / 256.0f;
        m.humidity_pct = static_cast<float>(h_q22_10) / 1024.0f;
        return alloy::core::Ok(std::move(m));
    }

    [[nodiscard]] auto calibration() const -> const detail::Calibration& { return calib_; }

private:
    [[nodiscard]] auto read_registers(std::uint8_t reg, std::span<std::uint8_t> out)
        -> ResultVoid {
        std::array<std::uint8_t, 1> tx{reg};
        return bus_->write_read(cfg_.address, tx, out);
    }

    [[nodiscard]] auto write_register(std::uint8_t reg, std::uint8_t value) -> ResultVoid {
        std::array<std::uint8_t, 2> tx{reg, value};
        return bus_->write(cfg_.address, tx);
    }

    [[nodiscard]] auto load_calibration() -> ResultVoid {
        std::array<std::uint8_t, 26> block1{};
        if (auto r = read_registers(detail::kRegCalibT, block1); r.is_err()) {
            return r;
        }
        std::array<std::uint8_t, 7> block2{};
        if (auto r = read_registers(detail::kRegCalibH, block2); r.is_err()) {
            return r;
        }

        calib_.dig_T1 = detail::u16_le(&block1[0]);
        calib_.dig_T2 = detail::s16_le(&block1[2]);
        calib_.dig_T3 = detail::s16_le(&block1[4]);
        calib_.dig_P1 = detail::u16_le(&block1[6]);
        calib_.dig_P2 = detail::s16_le(&block1[8]);
        calib_.dig_P3 = detail::s16_le(&block1[10]);
        calib_.dig_P4 = detail::s16_le(&block1[12]);
        calib_.dig_P5 = detail::s16_le(&block1[14]);
        calib_.dig_P6 = detail::s16_le(&block1[16]);
        calib_.dig_P7 = detail::s16_le(&block1[18]);
        calib_.dig_P8 = detail::s16_le(&block1[20]);
        calib_.dig_P9 = detail::s16_le(&block1[22]);
        calib_.dig_H1 = block1[25];

        calib_.dig_H2 = detail::s16_le(&block2[0]);
        calib_.dig_H3 = block2[2];
        calib_.dig_H4 = static_cast<std::int16_t>(
            (static_cast<std::int16_t>(static_cast<std::int8_t>(block2[3])) << 4) |
            (block2[4] & 0x0F));
        calib_.dig_H5 = static_cast<std::int16_t>(
            (static_cast<std::int16_t>(static_cast<std::int8_t>(block2[5])) << 4) |
            (block2[4] >> 4));
        calib_.dig_H6 = static_cast<std::int8_t>(block2[6]);
        return alloy::core::Ok();
    }

    BusHandle* bus_;
    Config cfg_;
    detail::Calibration calib_{};
};

}  // namespace alloy::drivers::sensor::bme280
