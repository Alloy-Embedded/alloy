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
// WHAT THAT MEANS FOR A READER WITH AN INVERTER: the arithmetic is covered by
// tests/test_bridge.cpp, which sweeps every boundary of the DTG encoding
// exhaustively (that is why it lives in st_tim_adv_dtg.hpp — a host test
// cannot include this file, which needs a generated IP header). The register
// WRITES below are covered by nothing. Put a scope on CH1 and CH1N before you
// connect a DC link.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/bridge/bridge_impl.hpp"
#include "alloy/hal/bridge/st_tim_adv_dtg.hpp"
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
    // The arithmetic ITSELF is in alloy/hal/bridge/st_tim_adv_dtg.hpp — the
    // piecewise DTG encoding, the three roundings and their directions, and
    // the argument for each — so that tests/test_bridge.cpp can sweep it. It
    // could not, from here: this file needs a generated `alloy/ip/st/tim_adv.hpp`
    // that exists only inside a built project, and host tests do not read
    // generated headers. What stays here is the BINDING of that arithmetic to
    // this IP's curated field widths, which is the part that is about ST.

    //: The DTG field's curated width, as a mask. Every function below is
    //: parameterised on it rather than on the number 1008.
    static constexpr std::uint32_t dtg_mask = IP::dtg.wide_raw_mask;

    [[nodiscard]] static constexpr std::uint32_t dtg_ticks(std::uint32_t code) {
        return detail::dtg_ticks(code, dtg_mask);
    }

    static constexpr std::uint32_t max_dtg_ticks = detail::dtg_max_ticks(dtg_mask);

    [[nodiscard]] static constexpr std::uint32_t dtg_code(std::uint32_t ticks) {
        return detail::dtg_code(ticks, dtg_mask);
    }

    // t_DTS = t_CK_INT << CKD, and this driver programs CKD = 0, so the
    // dead-time tick IS the timer's kernel tick. Stated as a function rather
    // than assumed anywhere: the conversion is the one place a wrong
    // assumption about CKD would silently halve the dead time.
    [[nodiscard]] static constexpr std::uint32_t dts_hz(std::uint32_t kernel_hz) {
        return kernel_hz;
    }

    //: Nanoseconds -> ticks of t_DTS, rounded UP (a request).
    [[nodiscard]] static constexpr std::uint32_t ns_to_ticks(std::uint32_t ns,
                                                             std::uint32_t kernel_hz) {
        return detail::dtg_ns_to_ticks(ns, dts_hz(kernel_hz));
    }

    //: Ticks -> nanoseconds, rounded DOWN (a report — never overstate the
    //: protection the silicon is actually inserting).
    [[nodiscard]] static constexpr std::uint32_t ticks_to_ns(std::uint32_t ticks,
                                                             std::uint32_t kernel_hz) {
        return detail::dtg_ticks_to_ns(ticks, dts_hz(kernel_hz));
    }

    //: The longest dead time programmable on this instance, in nanoseconds.
    //: Rounds DOWN, which is not the same conversion as the report — see the
    //: detail header, where getting that wrong is a fixed bug with a sweep.
    [[nodiscard]] static constexpr std::uint32_t max_dead_time_ns(std::uint32_t kernel_hz) {
        return detail::dtg_max_dead_time_ns(dtg_mask, dts_hz(kernel_hz));
    }

    //: What the hardware will actually insert for a requested nanosecond
    //: figure. Reported by the handle; never assumed equal to the request.
    [[nodiscard]] static constexpr std::uint32_t achieved_dead_time_ns(
        std::uint32_t requested_ns, std::uint32_t kernel_hz) {
        return detail::dtg_achieved_ns(requested_ns, dtg_mask, dts_hz(kernel_hz));
    }

    // ── The timebase ─────────────────────────────────────────────────────
    //
    // Also in the detail header, and for the same reason — the argument for
    // PSC = 0 (duty resolution, not LED convenience) is written there.
    using timebase = detail::tim_adv_timebase;

    [[nodiscard]] static constexpr timebase timebase_for(std::uint32_t kernel_hz,
                                                         std::uint32_t freq_hz,
                                                         bool center) {
        return detail::tim_adv_timebase_for(kernel_hz, freq_hz, center,
                                            max_period_ticks);
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
