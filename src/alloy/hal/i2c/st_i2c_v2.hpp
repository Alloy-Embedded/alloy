// I2C controller driver for the ST i2c_v2 IP (TIMINGR/ISR era).
//
// Timing is COMPUTED, not a magic constant: PRESC divides the kernel clock
// to a 4 MHz (250 ns) base tick, then fixed SCLL/SCLH counts satisfy the
// I2C spec conservatively (AN4235-style):
//   100 kHz: SCLL=19 (5.0 µs ≥ 4.7), SCLH=15 (4.0 µs ≥ 4.0) → ~106 kHz
//   400 kHz: SCLL=5 (1.25 µs ≥ 1.3*), SCLH=3 (1.0 µs ≥ 0.6) → ~385 kHz
// All transfers are AUTOEND with NACK detection; bool false = NACK/error.
//
// Two transfer paths: the polled write()/read()/write_read() (bounded spins,
// unchanged) and an interrupt-driven one — see the block comment above
// start_transfer_irq() for why they must not overlap on one bus.

#pragma once

#include <concepts>
#include <cstdint>
#include <span>

#include "alloy/core/types.hpp"
#include "alloy/hal/i2c/i2c_impl.hpp"
#include "alloy/ip/st/i2c_v2.hpp"
#include "alloy/irq.hpp"

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

    // Poll budget: an iteration cap that bounds a wedged bus (SDA held low by a
    // stuck slave, missing pull-ups, arbitration loss) instead of spinning
    // forever. Sized like the ESP32 driver's spin — far above any legal
    // transfer's flag latency, so it only ever fires on a genuine fault.
    static constexpr std::uint32_t kPollBudget = 4'000'000u;

    // Wait for a flag; true when it arrives, false on NACK, bus error
    // (BERR/ARLO) or timeout. Honors the facade contract "false = NACK / bus
    // error" (i2c.hpp) instead of hanging on a jammed bus.
    template <class Flag>
    static bool wait_flag(Flag flag) {
        using icr = typename IP::icr;
        for (std::uint32_t spin = 0; spin < kPollBudget; ++spin) {
            if (IP::nackf.read(r()) != 0u) {
                // Address/data NACK: wait (bounded) for the AUTOEND STOP, then
                // clear both flags so the peripheral is idle for the next call.
                for (std::uint32_t s = 0; s < kPollBudget; ++s) {
                    if (IP::stopf.read(r()) != 0u) {
                        break;
                    }
                }
                r().ICR = icr::nackcf | icr::stopcf;
                return false;
            }
            if ((IP::berr.read(r()) | IP::arlo.read(r())) != 0u) {
                r().ICR = icr::berrcf | icr::arlocf;  // clear so the bus recovers
                return false;
            }
            if (flag.read(r()) != 0u) {
                return true;
            }
        }
        return false;  // bus wedged: budget exhausted with no flag, NACK or error
    }

    // NBYTES is an 8-bit CR2 field: a length > 255 would overflow it and set the
    // RELOAD/AUTOEND bits above it, silently corrupting the transfer. v1 rejects
    // it (RELOAD/TCR chunking is a v2 feature) rather than mis-programming CR2.
    static constexpr std::size_t kMaxNbytes = 255;

    static std::uint32_t cr2_base(std::uint8_t addr, std::size_t n) {
        return (static_cast<std::uint32_t>(addr) << 1) |
               (static_cast<std::uint32_t>(n) << IP::nbytes.pos);
    }

    [[nodiscard]] static bool write(std::uint8_t addr, std::span<const std::uint8_t> data) {
        if (data.size() > kMaxNbytes) {
            return false;  // > NBYTES (8-bit); would corrupt CR2 — see kMaxNbytes
        }
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
        if (data.size() > kMaxNbytes) {
            return false;  // > NBYTES (8-bit); would corrupt CR2 — see kMaxNbytes
        }
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
        if (wr.size() > kMaxNbytes || rd.size() > kMaxNbytes) {
            return false;  // > NBYTES (8-bit) on either phase — see kMaxNbytes
        }
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

    // --- Interrupt-driven transfer: the same AUTOEND sequence, no spinning ---
    //
    // write()/read() above wait_flag() once PER BYTE and once more for STOP: an
    // N-byte transfer is N+1 bounded spins with nothing else running, and at
    // 100 kHz each byte is ~90 µs of burnt CPU. start_transfer_irq() programs
    // CR2 and returns; the ISR moves every byte and calls back on STOP.
    //
    // Driven by TXIS (write) or RXNE (read) for the data, and STOPF for
    // completion. Those two data flags are cleared by WRITING TXDR / READING
    // RXDR — the ISR doing its actual job, not a separate acknowledge — so
    // neither can storm. STOPF is different: it is w1c through ICR, and it is
    // exactly the flag the DMA driver's software latch exists to protect. The
    // protection here is structural instead: STOPIE/TXIE/RXIE are armed only
    // between start_transfer_irq() and completion, and the ISR disarms all
    // three before it returns. With no async transfer in flight the enables are
    // clear, so the polled path above still sees every flag it waits for and is
    // left BYTE-FOR-BYTE as it was.
    //
    // What that does NOT cover is calling a polled write()/read() WHILE an
    // async transfer is in flight — the ISR consumes the TXIS/RXNE/STOPF the
    // polled loop is waiting for and both give wrong answers. The two APIs must
    // not overlap on one bus; ask transfer_busy() first.
    //
    // ERRIE and NACKIE are deliberately never enabled. A NACK on an AUTOEND
    // transfer raises STOPF anyway, so completion still arrives and the ISR
    // reads NACKF there to decide success — arming a second interrupt for a
    // condition that already reaches us would only add a flag to acknowledge.
    // BERR/ARLO do NOT necessarily produce a STOP, so a bus error can leave the
    // transfer stalled: that is what the BOUNDED wait in i2c.hpp turns into an
    // honest false, and disable_transfer_irq() is the escape hatch. That path
    // has never been exercised.
    inline static const std::uint8_t* tx_ = nullptr;
    inline static std::uint8_t* rx_ = nullptr;
    inline static std::uint16_t len_ = 0;
    inline static std::uint16_t idx_ = 0;
    inline static void (*done_fn_)(void*) = nullptr;
    inline static void* done_ctx_ = nullptr;
    inline static volatile bool busy_ = false;
    inline static volatile bool ok_ = false;
    inline static bool attached_ = false;

    static void disarm() {
        IP::txie.clear(r());
        IP::rxie.clear(r());
        IP::stopie.clear(r());
    }

    static void transfer_isr(void*) {
        // Shared-vector contract (alloy::irq): one I2C line carries TXIS, RXNE,
        // TC, STOPF, NACKF and ADDR, and on some parts more than one peripheral
        // shares a vector. A cause counts as ours only if WE armed its enable —
        // a co-sharing polled transfer legitimately leaves its own flags set.
        const bool tx_pending = IP::txie.read(r()) != 0u && IP::txis.read(r()) != 0u;
        const bool rx_pending = IP::rxie.read(r()) != 0u && IP::rxne.read(r()) != 0u;
        const bool end_pending = IP::stopie.read(r()) != 0u && IP::stopf.read(r()) != 0u;
        if (!tx_pending && !rx_pending && !end_pending) {
            return;
        }
        if (!busy_) {
            disarm();  // armed with no transfer: disarm rather than storm
            r().ICR = IP::stopcf.mask;
            return;
        }
        if (tx_pending) {
            if (idx_ < len_) {
                r().TXDR = tx_[idx_];  // this write IS the TXIS clear
                ++idx_;
            }
            if (idx_ >= len_) {
                IP::txie.clear(r());  // no byte left to send: cannot re-enter
            }
            return;  // STOPF arrives as its own interrupt
        }
        if (rx_pending) {
            // Bind the read to a local: discarding it with `(void)` would not
            // be a volatile access at all, so RXNE would never clear.
            const auto got = static_cast<std::uint8_t>(r().RXDR);
            if (idx_ < len_) {
                rx_[idx_] = got;
                ++idx_;
            }
            if (idx_ >= len_) {
                IP::rxie.clear(r());
            }
            return;
        }
        finish();
    }

    static void finish() {
        using icr = typename IP::icr;
        // A byte can land in the same interrupt as STOP on a 1-byte read; take
        // it before declaring the transfer over.
        while (idx_ < len_ && rx_ != nullptr && IP::rxne.read(r()) != 0u) {
            rx_[idx_] = static_cast<std::uint8_t>(r().RXDR);
            ++idx_;
        }
        const bool nacked = IP::nackf.read(r()) != 0u;
        const bool bus_error = (IP::berr.read(r()) | IP::arlo.read(r())) != 0u;
        r().ICR = icr::stopcf | icr::nackcf | icr::berrcf | icr::arlocf;
        disarm();
        ok_ = !nacked && !bus_error && idx_ >= len_;
        busy_ = false;
        if (done_fn_ != nullptr) {
            // The callback sees a finished, disarmed transfer, so it may start
            // the next one without racing its own completion.
            done_fn_(done_ctx_);
        }
    }

    // rx == nullptr sends `tx`; tx == nullptr receives into `rx`. Exactly one
    // direction per call — AUTOEND, like write()/read().
    static void start_transfer_irq(std::uint8_t addr, const std::uint8_t* tx, std::uint8_t* rx,
                                   std::uint16_t len, void (*fn)(void*), void* ctx) {
        using icr = typename IP::icr;
        if (busy_ || len == 0u || len > kMaxNbytes || (tx == nullptr) == (rx == nullptr)) {
            __builtin_trap();  // concurrent, empty, oversized or ambiguous: honest guard
        }
        tx_ = tx;
        rx_ = rx;
        len_ = len;
        idx_ = 0;
        done_fn_ = fn;
        done_ctx_ = ctx;
        ok_ = false;
        if (!attached_) {
            alloy::irq::attach(Inst::irq, &transfer_isr);
            alloy::irq::enable(Inst::irq);
            attached_ = true;
        }
        r().ICR = icr::stopcf | icr::nackcf | icr::berrcf | icr::arlocf;  // stale flags
        busy_ = true;
        // Armed BEFORE START so completion cannot slip through between the CR2
        // write and the arming. These are level-sensitive, so arming early
        // costs nothing: no flag is set yet on an idle bus.
        if (rx != nullptr) {
            IP::rxie.set(r());
        } else {
            IP::txie.set(r());
        }
        IP::stopie.set(r());
        r().CR2 = cr2_base(addr, len) | (rx != nullptr ? IP::rd_wrn.mask : 0u) |
                  IP::autoend.mask | IP::start.mask;
    }

    [[nodiscard]] static bool transfer_busy() { return busy_; }

    // False after a transfer that NACKed, hit BERR/ARLO, or was abandoned —
    // the async equivalent of write()/read() returning false.
    [[nodiscard]] static bool transfer_ok() { return ok_; }

    // Disarm and forget any in-flight transfer: the escape hatch when a bus
    // error stalls it and no STOP ever comes, so the bus is not left busy_
    // forever. Never exercised — see the header note.
    static void disable_transfer_irq() {
        disarm();
        if (attached_) {
            alloy::irq::detach(Inst::irq, &transfer_isr);
            attached_ = false;
        }
        busy_ = false;
        ok_ = false;
        done_fn_ = nullptr;
        done_ctx_ = nullptr;
    }
};

}  // namespace alloy::hal
