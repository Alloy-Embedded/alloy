// PWM driver for the ESP32 LEDC high-speed side.
//
// Quirks honored: timers power up HELD IN RESET (HSTIMER_CONF reset has
// RST=1 — cleared here); div_num is a Q10.8 fixed-point divider of the
// APB tick; DUTY[3:0] are dither bits (integer duty lives at [24:4]);
// HS-side updates apply immediately (no para_up latch). Channel numbers
// are 1-based in the role API → HSCH(channel-1).

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/mmio.hpp"
#include "alloy/core/types.hpp"
#include "alloy/hal/pwm/pwm_impl.hpp"
#include "alloy/ip/espressif/ledc_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::espressif::ledc_v1>
struct pwm_impl<Inst> {
    using IP = typename Inst::ip;

    static constexpr std::uint32_t kDutyResBits = 13;  // 8192 steps

    static rw32& hsch_conf0(unsigned ch) {
        return alloy::reg_at(Inst::base, IP::HSCH_CONF0_offset, IP::HSCH_CONF0_stride, ch);
    }
    static rw32& hsch_hpoint(unsigned ch) {
        return alloy::reg_at(Inst::base, IP::HSCH_HPOINT_offset, IP::HSCH_HPOINT_stride, ch);
    }
    static rw32& hsch_duty(unsigned ch) {
        return alloy::reg_at(Inst::base, IP::HSCH_DUTY_offset, IP::HSCH_DUTY_stride, ch);
    }
    static rw32& hstimer_conf(unsigned t) {
        return alloy::reg_at(Inst::base, IP::HSTIMER_CONF_offset, IP::HSTIMER_CONF_stride, t);
    }

    static void enable(std::uint32_t kernel_hz, std::uint32_t freq_hz, unsigned channel) {
        alloy::gate_on(Inst::gate);
        auto& rst = *reinterpret_cast<rw32*>(Inst::reset_clear.reg);
        rst = rst & ~Inst::reset_clear.mask;  // release from DPORT reset

        // Q10.8 divider of the APB tick for the requested period.
        const std::uint64_t div_q8 =
            (static_cast<std::uint64_t>(kernel_hz) << 8) /
            (static_cast<std::uint64_t>(freq_hz) << kDutyResBits);
        auto& tconf = hstimer_conf(0);
        IP::rst.write(tconf, 1u);
        IP::duty_res.write(tconf, kDutyResBits);
        IP::div_num.write(tconf, static_cast<std::uint32_t>(div_q8));
        IP::tick_sel.write(tconf, 1u);  // APB clock
        IP::rst.write(tconf, 0u);

        const unsigned ch = channel - 1u;
        auto& conf0 = hsch_conf0(ch);
        IP::timer_sel.write(conf0, 0u);
        hsch_hpoint(ch) = 0u;
        hsch_duty(ch) = 0u;
        IP::sig_out_en.write(conf0, 1u);
    }

    static void set_duty(unsigned channel, std::uint16_t duty) {
        const std::uint32_t steps =
            (static_cast<std::uint32_t>(duty) << kDutyResBits) / 65535u;
        hsch_duty(channel - 1u) = steps << 4;  // [3:0] = dither, unused
    }
};

}  // namespace alloy::hal
