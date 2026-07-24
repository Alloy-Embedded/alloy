// I2C master driver for the ST i2c_v1 IP — the OLD variant (F1/F2/F4).
//
// Completely different from st_i2c_v2 (TIMINGR-based): here timing is
// computed into CR2.FREQ (fPCLK1 in MHz) + CCR (Sm: fPCLK1/(2*fSCL)) + TRISE
// (fPCLK1_MHz+1), all written with PE=0. The transfer is a MANUAL state
// machine over SR1/SR2, whose flags clear by defined read sequences, not
// w1c: SB clears by SR1-read then DR-write(addr); ADDR clears by SR1-read
// then SR2-read (SCL is stretched until you do); AF (NACK) is rc_w0.
//
// The receive path needs the RM's ACK/POS dance: N>2 bytes uses ACK during,
// set STOP after BTF; 2 bytes uses POS+ACK then STOP-after-ADDR; 1 byte
// clears ACK before clearing ADDR then STOP. bool = false on NACK/error.
//
// Not silicon-validated (no F4 board on hand) — tier-2, compile-checked.

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/ip/st/i2c_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::i2c_v1>
struct i2c_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t speed_hz) {
        alloy::gate_on(Inst::gate);
        IP::pe.clear(r());  // config only with PE=0
        const std::uint32_t mhz = kernel_hz / 1'000'000u;
        r().CR2 = mhz & 0x3Fu;  // FREQ = APB1 clock in MHz
        // Standard mode (<=100 kHz): CCR = fPCLK1/(2*fSCL), min 4.
        // Fast mode duty-0: CCR = fPCLK1/(3*fSCL), FS bit set.
        std::uint32_t ccr;
        if (speed_hz <= 100'000u) {
            ccr = kernel_hz / (2u * speed_hz);
            if (ccr < 4u) {
                ccr = 4u;
            }
            r().CCR = ccr & 0x0FFFu;
            r().TRISE = (mhz + 1u) & 0x3Fu;  // Sm: max rise 1000 ns
        } else {
            ccr = kernel_hz / (3u * speed_hz);
            if (ccr < 1u) {
                ccr = 1u;
            }
            r().CCR = IP::fs.mask | (ccr & 0x0FFFu);
            r().TRISE = (mhz * 3u / 10u + 1u) & 0x3Fu;  // Fm: max rise 300 ns
        }
        IP::pe.set(r());
    }

    static void start() {
        IP::ack.set(r());
        IP::start.set(r());
        while (IP::sb.read(r()) == 0u) {  // SB set after START
        }
    }

    static void stop() { IP::stop.set(r()); }

    // Send address; false on NACK. `read` selects the R/W bit.
    [[nodiscard]] static bool send_addr(std::uint8_t addr, bool read) {
        r().DR = static_cast<std::uint8_t>((addr << 1) | (read ? 1u : 0u));  // clears SB
        while (true) {
            const std::uint32_t sr1 = r().SR1;
            if (sr1 & IP::af.mask) {
                IP::af.clear(r());
                stop();
                return false;  // NACK
            }
            if (sr1 & IP::addr.mask) {
                return true;  // caller clears ADDR (SR1 then SR2) at the right moment
            }
        }
    }

    static void clear_addr() {
        (void)r().SR1;
        (void)r().SR2;  // the mandatory SR1-then-SR2 read that unstretches SCL
    }

    [[nodiscard]] static bool write(std::uint8_t addr, std::span<const std::uint8_t> data) {
        while (IP::busy.read(r()) != 0u) {
        }
        start();
        if (!send_addr(addr, false)) {
            return false;
        }
        clear_addr();
        for (auto byte : data) {
            while (true) {
                const std::uint32_t sr1 = r().SR1;
                if (sr1 & IP::af.mask) {
                    IP::af.clear(r());
                    stop();
                    return false;
                }
                if (sr1 & IP::txe.mask) {
                    break;
                }
            }
            r().DR = byte;
        }
        while (IP::btf.read(r()) == 0u) {  // last byte fully shifted
        }
        stop();
        return true;
    }

    [[nodiscard]] static bool read(std::uint8_t addr, std::span<std::uint8_t> data) {
        if (data.empty()) {
            return write(addr, {});  // probe: zero-length is a write-address ack
        }
        while (IP::busy.read(r()) != 0u) {
        }
        start();
        if (!send_addr(addr, true)) {
            return false;
        }
        const std::size_t n = data.size();
        std::size_t i = 0;
        if (n == 1) {
            IP::ack.clear(r());
            clear_addr();
            stop();
            while (IP::rxne.read(r()) == 0u) {
            }
            data[0] = static_cast<std::uint8_t>(r().DR);
            return true;
        }
        clear_addr();
        // N>=2: ACK each byte, drop ACK + STOP once only two remain.
        while (n - i > 2) {
            while (IP::rxne.read(r()) == 0u) {
            }
            data[i++] = static_cast<std::uint8_t>(r().DR);
        }
        while (IP::btf.read(r()) == 0u) {  // two bytes latched (DR + shift)
        }
        IP::ack.clear(r());
        stop();
        data[i++] = static_cast<std::uint8_t>(r().DR);
        while (IP::rxne.read(r()) == 0u) {
        }
        data[i++] = static_cast<std::uint8_t>(r().DR);
        return true;
    }

    [[nodiscard]] static bool write_read(std::uint8_t addr,
                                         std::span<const std::uint8_t> wr,
                                         std::span<std::uint8_t> rd) {
        while (IP::busy.read(r()) != 0u) {
        }
        start();
        if (!send_addr(addr, false)) {
            return false;
        }
        clear_addr();
        for (auto byte : wr) {
            while (true) {
                const std::uint32_t sr1 = r().SR1;
                if (sr1 & IP::af.mask) {
                    IP::af.clear(r());
                    stop();
                    return false;
                }
                if (sr1 & IP::txe.mask) {
                    break;
                }
            }
            r().DR = byte;
        }
        while (IP::btf.read(r()) == 0u) {
        }
        // Repeated START into the read phase (no STOP between).
        return read(addr, rd);
    }
};

}  // namespace alloy::hal
