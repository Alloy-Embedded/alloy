// I2C controller driver for the ST i2c_v2 IP (TIMINGR/ISR era).
//
// Timing is COMPUTED, not a magic constant: PRESC divides the kernel clock
// to a 4 MHz (250 ns) base tick, then fixed SCLL/SCLH counts satisfy the
// I2C spec conservatively (AN4235-style):
//   100 kHz: SCLL=19 (5.0 µs ≥ 4.7), SCLH=15 (4.0 µs ≥ 4.0) → ~106 kHz
//   400 kHz: SCLL=5 (1.25 µs ≥ 1.3*), SCLH=3 (1.0 µs ≥ 0.6) → ~385 kHz
// All transfers are AUTOEND with NACK detection; bool false = NACK/error.

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

#include "alloy/core/types.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/ip/st/i2c_v2.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::i2c_v2>
struct i2c_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t speed_hz) {
        alloy::gate_on(Inst::gate);
        IP::pe.clear(r());
        const std::uint32_t presc = kernel_hz / 4'000'000u - 1u;  // 4 MHz base
        std::uint32_t timingr;
        if (speed_hz > 100'000u) {  // fast mode 400 kHz
            timingr = (presc << IP::presc.pos) | (3u << IP::scldel.pos) |
                      (2u << IP::sdadel.pos) | (3u << IP::sclh.pos) | 5u;
        } else {  // standard mode 100 kHz
            timingr = (presc << IP::presc.pos) | (4u << IP::scldel.pos) |
                      (2u << IP::sdadel.pos) | (15u << IP::sclh.pos) | 19u;
        }
        r().TIMINGR = timingr;
        IP::pe.set(r());
    }

    // Wait for a flag or NACK; true when the flag arrived, false on NACK.
    template <class Flag>
    static bool wait_flag(Flag flag) {
        while (true) {
            if (IP::nackf.read(r()) != 0u) {
                while (IP::stopf.read(r()) == 0u) {
                }
                r().ICR = IP::nackcf.mask | IP::stopcf.mask;
                return false;
            }
            if (flag.read(r()) != 0u) {
                return true;
            }
        }
    }

    static std::uint32_t cr2_base(std::uint8_t addr, std::size_t n) {
        return (static_cast<std::uint32_t>(addr) << 1) |
               (static_cast<std::uint32_t>(n) << IP::nbytes.pos);
    }

    [[nodiscard]] static bool write(std::uint8_t addr, std::span<const std::uint8_t> data) {
        r().CR2 = cr2_base(addr, data.size()) | IP::autoend.mask | IP::start.mask;
        for (auto byte : data) {
            if (!wait_flag(IP::txis)) {
                return false;
            }
            r().TXDR = byte;
        }
        if (!wait_flag(IP::stopf)) {
            return false;
        }
        r().ICR = IP::stopcf.mask;
        return true;
    }

    [[nodiscard]] static bool read(std::uint8_t addr, std::span<std::uint8_t> data) {
        r().CR2 = cr2_base(addr, data.size()) | IP::rd_wrn.mask |
                  IP::autoend.mask | IP::start.mask;
        for (auto& byte : data) {
            if (!wait_flag(IP::rxne)) {
                return false;
            }
            byte = static_cast<std::uint8_t>(r().RXDR);
        }
        if (!wait_flag(IP::stopf)) {
            return false;
        }
        r().ICR = IP::stopcf.mask;
        return true;
    }

    [[nodiscard]] static bool write_read(std::uint8_t addr,
                                         std::span<const std::uint8_t> wr,
                                         std::span<std::uint8_t> rd) {
        // Write phase without AUTOEND, then repeated-start read.
        r().CR2 = cr2_base(addr, wr.size()) | IP::start.mask;
        for (auto byte : wr) {
            if (!wait_flag(IP::txis)) {
                return false;
            }
            r().TXDR = byte;
        }
        if (!wait_flag(IP::tc)) {
            return false;
        }
        return read(addr, rd);
    }
};

}  // namespace alloy::hal
