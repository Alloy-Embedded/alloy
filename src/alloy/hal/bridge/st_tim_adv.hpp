// Complementary-bridge driver for ST advanced-control timers (tim_adv: TIM1
// on the G0B1RE). Up to three CHx/CHxN pairs, hardware dead time, one break
// input, and a main output enable that keeps every pin off until the
// application says otherwise.
//
// THE THIRD PERSONALITY of a timer block. alloy/hal/pwm/st_tim_gp16.hpp is
// the first, alloy/hal/encoder/st_tim_gp16.hpp the second; this one is on a
// different IP (tim_adv is a superset of tim_gp16) but the same rule holds —
// one block, one personality at a time, enforced by alloy::claim rather than
// documented.
//
// BEHAVIOR only: bases, gates and fields come from generated headers.
//
// NOT SILICON-VALIDATED, and it CANNOT be validated in emulation either.
// Renode 1.16.1's Timers.STM32_Timer has no BDTR at all — no dead-time
// generator, no MOE, no break input, no complementary outputs. There is no
// model in which the register sequence below can be observed to insert a
// dead time or to switch the outputs off on a fault. Every statement about
// the hardware in this file is read from the reference data (the pinned
// stm32-data timer_v3.json, and RM0444 section 21 for the DTG encoding),
// not witnessed. No board is on hand.
//
// WHAT THAT MEANS FOR A READER WITH AN INVERTER: the arithmetic below is
// covered by host tests that pin every boundary of the DTG encoding, and the
// register WRITES are covered by nothing. Put a scope on CH1 and CH1N before
// you connect a DC link.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/bridge/bridge_impl.hpp"
#include "alloy/ip/st/tim_adv.hpp"

namespace alloy::hal {

// LAYER 2 for this IP: three knobs, and each of them is here rather than in
// bridge_config because its VALUE SET is the silicon's.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_adv>
struct bridge_opts<Inst> {
    //: How many consecutive samples of the break input, at what rate, before
    //: the hardware believes a fault. The names are the silicon's because the
    //: quantity is: `dts_div16_n8` means eight samples at t_DTS/16, and there
    //: is no portable spelling of that.
    //:
    //: THE DEFAULT IS `none`, AND THAT IS THE CONSERVATIVE CHOICE HERE even
    //: though it is the noisy one. An unfiltered break trips on a glitch,
    //: which stops a bridge that did not need stopping — recoverable, and
    //: visible. A filtered break ignores a fault shorter than the filter,
    //: which is a desaturation event the bridge rides through. The failure
    //: modes are not symmetric, so the default errs toward tripping. Opt into
    //: a filter when the fault line is measured and noisy.
    enum class break_filter : std::uint8_t {
        none = 0,
        ck_int_n2 = 1, ck_int_n4 = 2, ck_int_n8 = 3,
        dts_div2_n6 = 4, dts_div2_n8 = 5,
        dts_div4_n6 = 6, dts_div4_n8 = 7,
        dts_div8_n6 = 8, dts_div8_n8 = 9,
        dts_div16_n5 = 10, dts_div16_n6 = 11, dts_div16_n8 = 12,
        dts_div32_n5 = 13, dts_div32_n6 = 14, dts_div32_n8 = 15,
    };
    bridge_opts::break_filter filter = break_filter::none;

    //: Which of the three center-aligned modes. They produce the SAME
    //: waveform and differ only in when the compare interrupt/trigger fires:
    //: while counting down (1), up (2), or both (3). Ignored when
    //: `config::align` is `edge`.
    enum class center_variant : std::uint8_t { on_down = 1, on_up = 2, on_both = 3 };
    bridge_opts::center_variant center = center_variant::on_down;

    //: Write-once (per peripheral reset) protection of the break and
    //: dead-time configuration. Level 1 freezes DTG, BKE, BKP, AOE and the
    //: off-state bits; 2 adds the output idle levels and CCxNE; 3 adds the
    //: compare modes and polarities. MOE is never locked, so the application
    //: can still stop and start the bridge.
    //:
    //: NOT the default, for one reason: a lock survives until a peripheral
    //: reset, so a locked bridge cannot be reconfigured by the same program
    //: later, and a framework that did that behind a user's back would be
    //: setting a trap. It is here because a shipped inverter should use it —
    //: it is what stops a wild pointer from clearing the dead time.
    enum class config_lock : std::uint8_t { none = 0, level1 = 1, level2 = 2, level3 = 3 };
    bridge_opts::config_lock lock = config_lock::none;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_adv>
struct bridge_impl<Inst> {
    using IP = typename Inst::ip;
    using opts = bridge_opts<Inst>;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // ── DEGREE, read from the data and not restated ──────────────────────
    //
    // How many complementary pairs this IP has is a `feat` of the IP version
    // (alloy-devices registers/st/tim_adv.yaml), not a number written here,
    // so a timer with one pair rather than three gets the right answer with
    // no edit and a timer with none is refused by the facade's static_assert.
    static constexpr unsigned phases = Inst::feat::complementary_channels;
    static constexpr unsigned break_inputs = Inst::feat::break_inputs;

    // How wide the counter is, from the curated width of ARR — the same
    // derivation encoder_impl uses for its modulo bound.
    static constexpr std::uint32_t max_period_ticks = IP::arr.wide_raw_mask + 1u;

    // ── The dead-time generator's encoding ───────────────────────────────
    //
    // DTG IS NOT A NUMBER OF TICKS. It is four ranges selected by its own top
    // bits (RM0444 section 21, the BDTR description), each with a different
    // step:
    //
    //   DTG[7:5] = 0xx    DT = DTG[7:0]        x  1 x t_DTS   (0 .. 127)
    //   DTG[7:5] = 10x    DT = (64 + DTG[5:0]) x  2 x t_DTS   (128 .. 254)
    //   DTG[7:5] = 110    DT = (32 + DTG[4:0]) x  8 x t_DTS   (256 .. 504)
    //   DTG[7:5] = 111    DT = (32 + DTG[4:0]) x 16 x t_DTS   (512 .. 1008)
    //
    // Two consequences the portable layer above depends on:
    //
    //   NOT EVERY TICK COUNT IS REPRESENTABLE. Above 127 ticks the steps are
    //   2, then 8, then 16, so an exact request usually cannot be met — which
    //   is why the facade reports the ACHIEVED dead time and does not pretend
    //   the requested one was programmed.
    //
    //   THE ROUNDING DIRECTION IS A SAFETY DECISION, and it is up. Rounding
    //   to nearest would, for a request of 129 ticks, program 128 — 15 ns
    //   less dead time than the gate driver was measured to need, on every
    //   edge, forever. Too much dead time distorts the output voltage; too
    //   little destroys the bridge. These are not comparable errors, so this
    //   function never returns less than it was asked for.

    //: Ticks of t_DTS that a given DTG code actually produces. The inverse of
    //: dtg_code(), and the function the facade uses to report reality.
    [[nodiscard]] static constexpr std::uint32_t dtg_ticks(std::uint32_t code) {
        code &= IP::dtg.wide_raw_mask;
        if ((code & 0x80u) == 0u) { return code; }
        if ((code & 0xC0u) == 0x80u) { return (64u + (code & 0x3Fu)) * 2u; }
        if ((code & 0xE0u) == 0xC0u) { return (32u + (code & 0x1Fu)) * 8u; }
        return (32u + (code & 0x1Fu)) * 16u;
    }

    //: The longest dead time this generator can produce, in ticks. DERIVED
    //: from the curated width of DTG — the widest code, decoded — rather than
    //: written as 1008, so an IP with a wider field needs no edit here.
    static constexpr std::uint32_t max_dtg_ticks = dtg_ticks(IP::dtg.wide_raw_mask);

    //: Smallest DTG code whose dead time is >= `ticks`, or a value above the
    //: field's mask when `ticks` cannot be reached at all (the facade admits
    //: that case before ever calling this).
    [[nodiscard]] static constexpr std::uint32_t dtg_code(std::uint32_t ticks) {
        const auto ceil_div = [](std::uint32_t a, std::uint32_t b) {
            return (a + b - 1u) / b;
        };
        if (ticks <= 127u) { return ticks; }
        if (ticks <= 254u) { return 0x80u | (ceil_div(ticks, 2u) - 64u); }
        if (ticks <= 504u) { return 0xC0u | (ceil_div(ticks, 8u) - 32u); }
        if (ticks <= 1008u) { return 0xE0u | (ceil_div(ticks, 16u) - 32u); }
        return IP::dtg.wide_raw_mask + 1u;  // out of range, by construction
    }

    // t_DTS = t_CK_INT << CKD, and this driver programs CKD = 0, so the
    // dead-time tick IS the timer's kernel tick. Stated as a function rather
    // than assumed anywhere: the conversion is the one place a wrong
    // assumption about CKD would silently halve the dead time.
    [[nodiscard]] static constexpr std::uint32_t dts_hz(std::uint32_t kernel_hz) {
        return kernel_hz;
    }

    //: Nanoseconds -> ticks of t_DTS, rounded UP (see above).
    [[nodiscard]] static constexpr std::uint32_t ns_to_ticks(std::uint32_t ns,
                                                             std::uint32_t kernel_hz) {
        const std::uint64_t num =
            static_cast<std::uint64_t>(ns) * static_cast<std::uint64_t>(dts_hz(kernel_hz));
        return static_cast<std::uint32_t>((num + 999'999'999u) / 1'000'000'000u);
    }

    //: Ticks -> nanoseconds, rounded up, so the number a program reports back
    //: is never smaller than the dead time it actually gets.
    [[nodiscard]] static constexpr std::uint32_t ticks_to_ns(std::uint32_t ticks,
                                                             std::uint32_t kernel_hz) {
        const std::uint32_t hz = dts_hz(kernel_hz);
        if (hz == 0u) { return 0u; }
        const std::uint64_t num =
            static_cast<std::uint64_t>(ticks) * 1'000'000'000u + (hz - 1u);
        return static_cast<std::uint32_t>(num / hz);
    }

    //: The longest dead time programmable on this instance, in nanoseconds.
    [[nodiscard]] static constexpr std::uint32_t max_dead_time_ns(std::uint32_t kernel_hz) {
        return ticks_to_ns(max_dtg_ticks, kernel_hz);
    }

    //: What the hardware will actually insert for a requested nanosecond
    //: figure. Reported by the handle; never assumed equal to the request.
    [[nodiscard]] static constexpr std::uint32_t achieved_dead_time_ns(
        std::uint32_t requested_ns, std::uint32_t kernel_hz) {
        return ticks_to_ns(dtg_ticks(dtg_code(ns_to_ticks(requested_ns, kernel_hz))),
                           kernel_hz);
    }

    // ── The timebase ─────────────────────────────────────────────────────
    //
    // PSC = 0 whenever the period fits, which is the opposite of what
    // alloy/hal/pwm/st_tim_gp16.hpp does (a fixed 1 MHz tick). A 1 MHz tick
    // at 20 kHz leaves 50 counts of duty resolution — under 6 bits — which is
    // fine for an LED and not fine for a motor: 2% duty granularity is 2% of
    // the DC link on every phase. At the full kernel clock the same 20 kHz
    // has 3200 counts, and the prescaler only appears when the period would
    // otherwise not fit in ARR.
    //
    // CENTER-ALIGNED COUNTS BOTH WAYS, so its period is 2 x ARR ticks and its
    // ARR is half the edge-aligned one for the same frequency.
    struct timebase {
        std::uint32_t psc;
        std::uint32_t arr;
    };

    [[nodiscard]] static constexpr timebase timebase_for(std::uint32_t kernel_hz,
                                                         std::uint32_t freq_hz,
                                                         bool center) {
        if (freq_hz == 0u) { return {0u, 0u}; }
        const std::uint32_t total = (kernel_hz / freq_hz) / (center ? 2u : 1u);
        std::uint32_t psc = 0u;
        while ((total / (psc + 1u)) > max_period_ticks) { ++psc; }
        const std::uint32_t arr = total / (psc + 1u);
        return {psc, arr == 0u ? 0u : arr - 1u};
    }

    static inline std::uint32_t period_ticks = 0;

    // ── Programming order, and it is the whole safety argument ───────────
    //
    // 1. gate, then CEN = 0. Everything below means something different if
    //    the block was doing anything a moment ago.
    // 2. CR2 = 0. The OUTPUT IDLE LEVELS live here and the driver forces all
    //    six to zero. This is not a knob and will not become one: the idle
    //    pattern is what the pins are driven to after a break, and the only
    //    combination that is safe on a half-bridge is "both off". A gate
    //    driver whose safe input level is HIGH exists, and for that board the
    //    answer is an inverting gate driver or `high_impedance`, not a bit
    //    here that turns a fault into a shoot-through when someone copies a
    //    config between boards.
    // 3. CR1, CCMR1/2, CCER, PSC/ARR/RCR, CCRx = 0 — the ordinary timebase
    //    and channel setup, with every duty at zero.
    // 4. AF1: route the BKIN pin into the break, with its own polarity bit
    //    pinned at 0 so BDTR.BKP is the only polarity in the system.
    // 5. BDTR LAST, and with MOE = 0. This is the ordering that matters most:
    //    the dead time, the off-state selection, the break enable and its
    //    polarity are all in this word, and MOE is the bit that lets any of
    //    it reach a pin. Writing it last with MOE clear means the outputs
    //    have never been enabled with a partial configuration behind them —
    //    the bridge comes up dark and stays dark until enable_outputs().
    //    The optional LOCK rides in this same write, because LOCK takes
    //    effect from the write that sets it and everything it protects is
    //    already in the word.
    // 6. EGR = UG to latch the preloaded PSC/ARR/CCRx, then CEN = 1. The
    //    counter runs; the outputs do not.
    //
    // WHOLE-REGISTER WRITES, not read-modify-write, for the reason the
    // encoder driver states: the bits this function does not name in CCMR1/2
    // are another personality's layout of the same word.
    template <bridge_opts<Inst> O = {}>
    static void enable(std::uint32_t kernel_hz, hal::bridge_config c,
                       unsigned phase_count, bool break_enabled,
                       hal::bridge_break_polarity break_active) {
        alloy::gate_on(Inst::gate);
        IP::cen.clear(r());

        // (2) idle levels: all six low. See the comment above.
        r().CR2 = 0u;

        const bool center = c.align == bridge_alignment::center;

        // (3) timebase and channels.
        const timebase tb = timebase_for(kernel_hz, c.freq_hz, center);
        r().PSC = tb.psc;
        r().ARR = tb.arr;
        period_ticks = tb.arr + 1u;
        // One update event per PWM period. A center-aligned counter reaches
        // its update condition twice per period (overflow and underflow), so
        // REP = 1 halves that back to once — which is what a control loop
        // sampling "one new duty per period" means by a period.
        r().RCR = center ? 1u : 0u;

        r().CCR1 = 0u;
        r().CCR2 = 0u;
        r().CCR3 = 0u;
        r().CCR4 = 0u;

        // PWM mode 1 with the compare preload on, on every channel this
        // instance can pair. Preload (OCxPE) is not optional here: without it
        // a duty written mid-period takes effect immediately and can produce
        // a pulse shorter than the dead time, which is the one pulse a
        // half-bridge cannot survive.
        r().CCMR1 = IP::ccmr1::cc1s_output | IP::ccmr1::oc1m_pwm_mode_1 |
                    IP::ccmr1::oc1pe | IP::ccmr1::cc2s_output |
                    IP::ccmr1::oc2m_pwm_mode_1 | IP::ccmr1::oc2pe;
        r().CCMR2 = IP::ccmr2::cc3s_output | IP::ccmr2::oc3m_pwm_mode_1 |
                    IP::ccmr2::oc3pe | IP::ccmr2::cc4s_output |
                    IP::ccmr2::oc4m_pwm_mode_1 | IP::ccmr2::oc4pe;

        // CCER: enable exactly the pairs the binder declared, both halves of
        // each, active high. A phase the caller did not bind stays disabled,
        // so its pins are never driven — the count comes from the binder's
        // phase tags, not from what the silicon has.
        std::uint32_t ccer = 0u;
        if (phase_count >= 1u) {
            ccer |= static_cast<std::uint32_t>(IP::ccer::cc1e) |
                    static_cast<std::uint32_t>(IP::ccer::cc1ne);
        }
        if (phase_count >= 2u) {
            ccer |= static_cast<std::uint32_t>(IP::ccer::cc2e) |
                    static_cast<std::uint32_t>(IP::ccer::cc2ne);
        }
        if (phase_count >= 3u) {
            ccer |= static_cast<std::uint32_t>(IP::ccer::cc3e) |
                    static_cast<std::uint32_t>(IP::ccer::cc3ne);
        }
        r().CCER = ccer;

        // CR1: ARPE so ARR is preloaded too, CKD = 0 so t_DTS is the kernel
        // tick (the dead-time arithmetic above depends on exactly this), and
        // the alignment. CEN stays clear until the end.
        std::uint32_t cr1 = static_cast<std::uint32_t>(IP::cr1::arpe) |
                            static_cast<std::uint32_t>(IP::cr1::ckd_div1);
        if (center) {
            cr1 |= static_cast<std::uint32_t>(O.center) << IP::cms.pos;
        }
        r().CR1 = cr1;

        // (4) the break input's source. BKINE routes the BKIN PIN into the
        // break; BKINP stays 0 so BDTR.BKP is the only polarity anywhere in
        // this configuration — two spellings of one fact is the shape this
        // project refuses.
        r().AF1 = break_enabled ? std::uint32_t{IP::bkine.wide_mask} : 0u;

        // (5) BDTR last, MOE clear.
        std::uint32_t bdtr = dtg_code(dead_time_ticks(c, kernel_hz));
        if (c.when_off == bridge_off_state::drive_idle_level) {
            bdtr |= static_cast<std::uint32_t>(IP::bdtr::ossi_drive_idle_level) |
                    static_cast<std::uint32_t>(IP::bdtr::ossr_drive_idle_level);
        }
        if (break_enabled) {
            bdtr |= static_cast<std::uint32_t>(IP::bdtr::bke);
            if (break_active == bridge_break_polarity::active_high) {
                bdtr |= static_cast<std::uint32_t>(IP::bdtr::bkp_active_high);
            }
            bdtr |= static_cast<std::uint32_t>(O.filter) << IP::bkf.pos;
        }
        // AOE stays 0. A bridge that re-enables its own outputs on the next
        // update after the fault clears is a compressor that restarts into a
        // shorted phase, once per PWM period, until something gives. Coming
        // back is the application's decision and it is spelled
        // acknowledge_fault() + enable_outputs().
        bdtr |= static_cast<std::uint32_t>(O.lock) << IP::lock.pos;
        r().BDTR = bdtr;

        // (6) latch and run.
        r().EGR = alloy::flags{IP::egr::ug};
        IP::cen.set(r());
    }

    //: Ticks of dead time this config asks for. `external` means the gate
    //: driver inserts it and the timer inserts none, which is the ONLY way
    //: this returns zero — `unstated` never reaches here (the facade admits
    //: it first) and ns(0) is refused by the same admission.
    [[nodiscard]] static constexpr std::uint32_t dead_time_ticks(hal::bridge_config c,
                                                                 std::uint32_t kernel_hz) {
        return c.dead_time.stated() == bridge_dead_time::kind::external
                   ? 0u
                   : ns_to_ticks(c.dead_time.requested_ns(), kernel_hz);
    }

    // ── Running ──────────────────────────────────────────────────────────

    static void set_duty(unsigned phase, std::uint16_t duty) {
        const std::uint32_t ccr =
            (static_cast<std::uint32_t>(duty) * period_ticks) / 65'535u;
        switch (phase) {
            case 1: r().CCR1 = ccr; break;
            case 2: r().CCR2 = ccr; break;
            case 3: r().CCR3 = ccr; break;
            default: break;
        }
    }

    static void outputs_on() { IP::moe.set(r()); }
    static void outputs_off() { IP::moe.clear(r()); }

    [[nodiscard]] static bool outputs_enabled() { return IP::moe.read(r()) != 0u; }

    //: Did the break fire? The flag is set by the HARDWARE and stays set
    //: until software clears it, so this answers "has a fault happened since
    //: I last looked", not "is the fault line asserted right now".
    [[nodiscard]] static bool faulted() { return IP::bif.read(r()) != 0u; }

    //: Clear the flag. Does NOT re-enable the outputs, and cannot: while the
    //: break input is still asserted the hardware holds MOE at 0 and clearing
    //: BIF just lets it be set again.
    static void acknowledge_fault() { IP::bif.clear(r()); }

    //: A break raised in software. Same path, same effect on the outputs as
    //: the pin — which is what makes an emergency stop that does not depend
    //: on the scheduler still running.
    static void force_break() { r().EGR = alloy::flags{IP::egr::bg}; }
};

}  // namespace alloy::hal
