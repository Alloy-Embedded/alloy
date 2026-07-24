// I2C controller driver for the SAM TWIHS (E70-era).
//
// Quirks honored: 7-bit address goes DIRECTLY in MMR.DADR (no <<1);
// CR.QUICK is the natural zero-length probe (address + stop); SR read
// clears NACK, so each loop samples SR exactly once. Clock: t_half =
// ((DIV * 2^CKDIV) + 3) / kernel.

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

#include "alloy/core/types.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/ip/microchip/twihs_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::microchip::twihs_v1>
struct i2c_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t speed_hz) {
        using cr = typename IP::cr;
        alloy::gate_on(Inst::gate);
        r().CR = IP::swrst.mask;
        // Half-period divider: cycles = kernel/(2*speed) - 3, folded by CKDIV.
        std::uint32_t cycles = kernel_hz / (2u * speed_hz) - 3u;
        std::uint32_t ckdiv = 0;
        while (cycles > 255u && ckdiv < 7u) {
            cycles = (cycles + 1u) / 2u;
            ++ckdiv;
        }
        r().CWGR = (ckdiv << IP::ckdiv.pos) | (cycles << IP::chdiv.pos) |
                   (cycles << IP::cldiv.pos) | (3u << IP::hold.pos);
        r().CR = cr::svdis | cr::msen;
    }

    // Poll budget: an iteration cap that bounds a wedged bus (SDA held low by a
    // stuck slave, missing pull-ups, arbitration loss) instead of spinning
    // forever. Sized like the ESP32 driver's spin — far above any legal
    // transfer's flag latency, so it only ever fires on a genuine fault.
    static constexpr std::uint32_t kPollBudget = 4'000'000u;

    // Bounded wait for `ready_mask` while watching NACK and arbitration loss.
    // true when ready; false on NACK / ARBLST / timeout — honors the facade
    // contract "false = NACK / bus error" (i2c.hpp) instead of hanging on a
    // jammed bus. (An SR read also clears NACK on this IP, so sample it once.)
    static bool wait_ready(std::uint32_t ready_mask) {
        for (std::uint32_t spin = 0; spin < kPollBudget; ++spin) {
            const std::uint32_t sr = r().SR;
            if (sr & (IP::nack.mask | IP::arblst.mask)) {
                return false;
            }
            if (sr & ready_mask) {
                return true;
            }
        }
        return false;  // bus wedged: budget exhausted with no ready flag or error
    }

    // Wait until TXCOMP; false on NACK, arbitration loss, or timeout.
    static bool wait_txcomp() { return wait_ready(IP::txcomp.mask); }

    [[nodiscard]] static bool write(std::uint8_t addr, std::span<const std::uint8_t> data) {
        r().MMR = static_cast<std::uint32_t>(addr) << IP::dadr.pos;  // MREAD=0
        if (data.empty()) {
            r().CR = IP::quick.mask;  // address probe: START+STOP only
            return wait_txcomp();
        }
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (!wait_ready(IP::txrdy.mask)) {
                return false;
            }
            r().THR = data[i];
            if (i + 1 == data.size()) {
                r().CR = IP::stop.mask;
            }
        }
        return wait_txcomp();
    }

    [[nodiscard]] static bool read(std::uint8_t addr, std::span<std::uint8_t> data) {
        using cr = typename IP::cr;
        if (data.empty()) {
            return write(addr, {});
        }
        r().MMR = (static_cast<std::uint32_t>(addr) << IP::dadr.pos) | IP::mread.mask;
        if (data.size() == 1) {
            r().CR = cr::start | cr::stop;
        } else {
            r().CR = IP::start.mask;
        }
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (!wait_ready(IP::rxrdy.mask)) {
                return false;
            }
            if (i + 2 == data.size()) {
                r().CR = IP::stop.mask;  // STOP before reading penultimate byte
            }
            data[i] = static_cast<std::uint8_t>(r().RHR);
        }
        return wait_txcomp();
    }

    [[nodiscard]] static bool write_read(std::uint8_t addr,
                                         std::span<const std::uint8_t> wr,
                                         std::span<std::uint8_t> rd) {
        using cr = typename IP::cr;
        // TWIHS internal-address feature covers the common 1-3 byte case.
        if (wr.empty() || wr.size() > 3 || rd.empty()) {
            return false;
        }
        std::uint32_t iadr = 0;
        for (auto byte : wr) {
            iadr = (iadr << 8) | byte;
        }
        r().MMR = (static_cast<std::uint32_t>(addr) << IP::dadr.pos) |
                  IP::mread.mask |
                  (static_cast<std::uint32_t>(wr.size()) << IP::iadrsz.pos);
        r().IADR = iadr;
        if (rd.size() == 1) {
            r().CR = cr::start | cr::stop;
        } else {
            r().CR = IP::start.mask;
        }
        for (std::size_t i = 0; i < rd.size(); ++i) {
            if (!wait_ready(IP::rxrdy.mask)) {
                return false;
            }
            if (i + 2 == rd.size()) {
                r().CR = IP::stop.mask;
            }
            rd[i] = static_cast<std::uint8_t>(r().RHR);
        }
        return wait_txcomp();
    }
};

}  // namespace alloy::hal
