// Calendar driver for the STM32 RTC "v3" (G0/L4/L5/C0-era).
//
// BEHAVIOR only: register overlay + APB gate come from generated data; the RTC
// clock source (LSI) is selected by the board clock program (RCC.BDCR), so this
// driver only touches RTC registers. RM0444 §35 bring-up: unlock the write
// protection (WPR 0xCA then 0x53), enter init mode (ICSR.INIT, wait INITF),
// program the prescaler for 1 Hz, load BCD time/date, exit init. Time is stored
// BCD on the wire; user code sees plain binary via alloy::datetime.
//
// The calendar lives in the always-on backup domain: once set it keeps ticking
// across resets (ICSR.INITS then reads 1, so callers can skip re-setting it).

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/rtc/rtc_impl.hpp"
#include "alloy/ip/st/rtc_v3.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::rtc_v3>
struct rtc_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    static std::uint32_t to_bcd(std::uint8_t v) {
        return static_cast<std::uint32_t>(((v / 10u) << 4u) | (v % 10u));
    }
    static std::uint8_t from_bcd(std::uint32_t b) {
        return static_cast<std::uint8_t>(((b >> 4u) * 10u) + (b & 0xFu));
    }

    static void unlock() {
        r().WPR = 0xCAu;
        r().WPR = 0x53u;
    }
    static void lock() { r().WPR = 0xFFu; }

    // True once the calendar has been set (survives reset in the backup domain).
    [[nodiscard]] static bool initialized() {
        alloy::gate_on(Inst::gate);
        return IP::inits.read(r()) != 0u;
    }

    static void set(const alloy::datetime& dt) {
        alloy::gate_on(Inst::gate);  // RTC APB interface clock
        unlock();
        // Enter init mode: the calendar is frozen while we load it.
        IP::init.set(r());
        while (IP::initf.read(r()) == 0u) {
        }
        // 24-hour format; bypass the shadow registers so now() reads the live
        // calendar directly (no RSF handshake).
        r().CR = IP::bypshad.mask;
        // LSI ~32 kHz -> 1 Hz: (PREDIV_A+1)(PREDIV_S+1) = 128 * 250 = 32000.
        // PRER wants two writes (sync factor first, then async), per RM0444.
        r().PRER = 249u << IP::prediv_s.pos;
        r().PRER = (127u << IP::prediv_a.pos) | (249u << IP::prediv_s.pos);
        r().TR = to_bcd(dt.second) | (to_bcd(dt.minute) << 8u) | (to_bcd(dt.hour) << 16u);
        r().DR = to_bcd(dt.day) | (to_bcd(dt.month) << 8u) | (to_bcd(dt.year) << 16u) |
                 (1u << 13u);  // weekday=1 (Monday); the calendar rejects 0
        IP::init.clear(r());   // exit init -> the clock starts counting
        lock();
    }

    [[nodiscard]] static alloy::datetime now() {
        alloy::gate_on(Inst::gate);
        const std::uint32_t tr = r().TR;
        const std::uint32_t dr = r().DR;
        alloy::datetime dt;
        dt.second = from_bcd(tr & 0x7Fu);
        dt.minute = from_bcd((tr >> 8u) & 0x7Fu);
        dt.hour = from_bcd((tr >> 16u) & 0x3Fu);
        dt.day = from_bcd(dr & 0x3Fu);
        dt.month = from_bcd((dr >> 8u) & 0x1Fu);
        dt.year = from_bcd((dr >> 16u) & 0xFFu);
        return dt;
    }
};

}  // namespace alloy::hal
