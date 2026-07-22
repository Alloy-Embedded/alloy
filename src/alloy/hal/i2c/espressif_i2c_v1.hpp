// I2C controller driver for the classic ESP32 command engine.
//
// A transfer is a short program in the 16 COMD slots (RSTART/WRITE/READ/
// STOP), data staged through the FIFO, kicked by CTR.TRANS_START and
// polled on INT_RAW: TRANS_COMPLETE = ok, ACK_ERR = NACK, ARB_LOST/
// TIME_OUT = bus errors. Bounded transfers only (FIFO is 32 deep).

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/ip/espressif/i2c_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::espressif::i2c_v1>
struct i2c_impl<Inst> {
    using IP = typename Inst::ip;

    enum op : std::uint32_t { kRstart = 0, kWrite = 1, kRead = 2, kStop = 3 };

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }
    static rw32& comd(unsigned slot) {
        return alloy::reg_at(Inst::base, IP::COMD_offset, IP::COMD_stride, slot);
    }

    static constexpr std::uint32_t cmd(op code, std::uint32_t n = 0,
                                       bool ack_check = false, bool ack_value = false) {
        return n | (ack_check ? IP::ack_check_en.mask() : 0u) |
               (ack_value ? IP::ack_value.mask() : 0u) |
               (static_cast<std::uint32_t>(code) << IP::op_code.pos);
    }

    // Kept for recovery: a NACK aborts the COMD program with the FSM mid-
    // transfer and no FSM_RST bit exists on the classic ESP32 — the way out
    // is a DPORT reset toggle followed by re-programming (esp-idf
    // i2c_hw_fsm_reset), which needs the timing parameters again.
    inline static std::uint32_t saved_kernel_hz = 0;
    inline static std::uint32_t saved_speed_hz = 0;

    static void enable(std::uint32_t kernel_hz, std::uint32_t speed_hz) {
        alloy::gate_on(Inst::gate);
        saved_kernel_hz = kernel_hz;
        saved_speed_hz = speed_hz;
        auto& rst = *reinterpret_cast<rw32*>(Inst::reset_clear.reg);
        rst = rst & ~Inst::reset_clear.mask;
        configure(kernel_hz, speed_hz);
    }

    static void configure(std::uint32_t kernel_hz, std::uint32_t speed_hz) {
        r().CTR = IP::sda_force_out.mask | IP::scl_force_out.mask | IP::ms_mode.mask;
        const std::uint32_t half = kernel_hz / speed_hz / 2u;
        r().SCL_LOW_PERIOD = half;
        r().SCL_HIGH_PERIOD = half - half / 10u;  // slight low-bias like esp-idf
        r().SDA_HOLD = half / 2u;
        r().SDA_SAMPLE = half / 2u;
        r().SCL_START_HOLD = half;
        r().SCL_RSTART_SETUP = half;
        r().SCL_STOP_HOLD = half;
        r().SCL_STOP_SETUP = half;
        r().TO = 0xFFFFFu;  // max 20-bit bus timeout
    }

    static void reset_fifos() {
        IP::tx_fifo_rst.write(r(), 1u);
        IP::rx_fifo_rst.write(r(), 1u);
        IP::tx_fifo_rst.write(r(), 0u);
        IP::rx_fifo_rst.write(r(), 0u);
    }

    // Put the block through a DPORT reset pulse and re-program it. Frees the
    // controller after an aborted program; a slave holding SDA mid-byte would
    // additionally need 9 manual SCL pulses (not implemented in v1).
    static void recover() {
        auto& rst = *reinterpret_cast<rw32*>(Inst::reset_clear.reg);
        rst = rst | Inst::reset_clear.mask;
        rst = rst & ~Inst::reset_clear.mask;
        configure(saved_kernel_hz, saved_speed_hz);
    }

    // Run the staged program; true on TRANS_COMPLETE, false on NACK/error.
    static bool run() {
        r().INT_CLR = 0xFFFFFFFFu;
        IP::trans_start.set(r());
        // Bounded spin: worst legal transfer at 100 kHz is well under this.
        for (std::uint32_t spin = 0; spin < 4'000'000u; ++spin) {
            const std::uint32_t raw = r().INT_RAW;
            if (raw & (IP::ack_err.mask | IP::arb_lost.mask | IP::time_out_int.mask)) {
                recover();  // NACK aborts the program mid-FSM, no STOP issued
                return false;
            }
            if (raw & IP::trans_complete.mask) {
                r().INT_CLR = 0xFFFFFFFFu;
                return true;
            }
        }
        recover();
        return false;
    }

    [[nodiscard]] static bool write(std::uint8_t addr, std::span<const std::uint8_t> data) {
        if (data.size() > 30) {
            return false;  // bounded v1: address + data must fit the FIFO
        }
        reset_fifos();
        r().DATA = static_cast<std::uint32_t>(addr) << 1;
        for (auto byte : data) {
            r().DATA = byte;
        }
        comd(0) = cmd(kRstart);
        comd(1) = cmd(kWrite, 1u + data.size(), true);
        comd(2) = cmd(kStop);
        return run();
    }

    [[nodiscard]] static bool read(std::uint8_t addr, std::span<std::uint8_t> data) {
        if (data.empty() || data.size() > 31) {
            return data.empty() ? write(addr, {}) : false;
        }
        reset_fifos();
        r().DATA = (static_cast<std::uint32_t>(addr) << 1) | 1u;
        unsigned slot = 0;
        comd(slot++) = cmd(kRstart);
        comd(slot++) = cmd(kWrite, 1, true);
        if (data.size() > 1) {
            comd(slot++) = cmd(kRead, data.size() - 1);
        }
        comd(slot++) = cmd(kRead, 1, false, true);  // NACK the last byte
        comd(slot++) = cmd(kStop);
        if (!run()) {
            return false;
        }
        for (auto& byte : data) {
            byte = static_cast<std::uint8_t>(r().DATA);
        }
        return true;
    }

    [[nodiscard]] static bool write_read(std::uint8_t addr,
                                         std::span<const std::uint8_t> wr,
                                         std::span<std::uint8_t> rd) {
        if (wr.empty() || rd.empty() || wr.size() > 14 || rd.size() > 15) {
            return false;
        }
        reset_fifos();
        r().DATA = static_cast<std::uint32_t>(addr) << 1;
        for (auto byte : wr) {
            r().DATA = byte;
        }
        r().DATA = (static_cast<std::uint32_t>(addr) << 1) | 1u;
        unsigned slot = 0;
        comd(slot++) = cmd(kRstart);
        comd(slot++) = cmd(kWrite, 1u + wr.size(), true);
        comd(slot++) = cmd(kRstart);
        comd(slot++) = cmd(kWrite, 1, true);
        if (rd.size() > 1) {
            comd(slot++) = cmd(kRead, rd.size() - 1);
        }
        comd(slot++) = cmd(kRead, 1, false, true);
        comd(slot++) = cmd(kStop);
        if (!run()) {
            return false;
        }
        for (auto& byte : rd) {
            byte = static_cast<std::uint8_t>(r().DATA);
        }
        return true;
    }
};

}  // namespace alloy::hal
