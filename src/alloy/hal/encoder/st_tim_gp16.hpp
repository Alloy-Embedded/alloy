// Quadrature encoder driver for ST 16-bit general-purpose timers (tim_gp16:
// TIM2/TIM3/TIM4 class). Channels 1 and 2 become TI1/TI2 inputs, the counter
// is clocked by their edges, and CNT is a position.
//
// THE SECOND PERSONALITY of a block whose first one is alloy/hal/pwm/
// st_tim_gp16.hpp. Same registers, same instance, mutually exclusive — see
// encoder_impl.hpp for what "mutually exclusive" is enforced by, and
// alloy-devices registers/st/tim_gp16.yaml `personalities: [encoder]` for
// how the generator learns that this file exists at all.
//
// BEHAVIOR only: bases/gates/fields from generated headers.
//
// NOT SILICON-VALIDATED, and it cannot be validated in emulation either —
// Renode's Timers.STM32_Timer answers a write of SMCR.SMS with "Unhandled
// write to offset 0x8 ... Tags: SMS" and an input-capture CCMR1 with
// "Channel 1: input capture mode is not supported". There is no model in
// which this register sequence can be observed to count. Every claim about
// the hardware below is read from the reference data, not witnessed.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/core/types.hpp"
#include "alloy/hal/encoder/encoder_impl.hpp"
#include "alloy/ip/st/tim_gp16.hpp"

namespace alloy::hal {

// LAYER 2 for this IP. One knob, and its value set is the reason it is here
// rather than in encoder_config: the timer offers THREE encoder modes, and
// they do not factor into a portable "x2 / x4". Modes 1 and 2 both count one
// input's edges — half the resolution of mode 3 — but they differ in WHICH
// input, and that choice matters when the two channels do not have equally
// clean signals. A portable enum spelled {x2, x4} could not express it, and
// one spelled {ti1, ti2, both} is not portable. Layer 2 is the answer to
// exactly that shape.
template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_gp16>
struct encoder_opts<Inst> {
    //: Which edges advance the counter. `both_edges` (encoder mode 3) is x4
    //: on a quadrature source and is the default because it is the only one
    //: that never loses a transition.
    enum class count_on : std::uint8_t { both_edges, ti1_edges, ti2_edges };
    encoder_opts::count_on edges = count_on::both_edges;
};

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::tim_gp16>
struct encoder_impl<Inst> {
    using IP = typename Inst::ip;

    static typename IP::regs& r() {
        return *reinterpret_cast<typename IP::regs*>(Inst::base);
    }

    // THE DEGREE FACT, READ FROM THE CURATED FIELD AND NOT RESTATED. ARR is
    // how far this counter can wrap, so the widest modulo it can hold is
    // its mask plus one. tim_gp16 declares ARR 16 bits wide, so this is
    // 65536 — but it is 65536 *because the data says so*, and an IP whose
    // ARR is 32 bits gets the right number here with no edit. This is the
    // "a quantity is a generated number" rule with no new `feat` entry
    // needed: the register description already stated it.
    static constexpr std::uint32_t max_period = IP::arr.wide_raw_mask + 1u;

    // Layer 1 (runtime `c`) + Layer 2 (compile-time `O`).
    template <encoder_opts<Inst> O = {}>
    static void enable(hal::encoder_config c) {
        alloy::gate_on(Inst::gate);
        // Stop before changing what the block IS. Every register below means
        // something different from what it meant a moment ago if this
        // instance was ever a PWM generator.
        IP::cen.clear(r());

        // WHOLE-REGISTER ASSIGNMENT, NOT READ-MODIFY-WRITE, and this is the
        // one place the personality doctrine shows up as machine code. A
        // read-modify-write preserves the bits it does not name; here the
        // bits it does not name belong to the OTHER personality's layout of
        // the same word — CCMR1's OC1M/OC1PE are the very bits that the
        // input view calls ICPSC/ICF. Preserving them would carry a PWM
        // mode field into an input filter setting. So both of these
        // registers are stated in full, from the generated flags enums.
        r().SMCR = alloy::flags{sms_for(O.edges)};
        r().CCMR1 = IP::ccmr1::cc1s_ti1 | IP::ccmr1::cc2s_ti2;
        // CCER: capture polarity. Non-inverted on both inputs counts one way;
        // inverting TI1 alone reverses the counting sense, which is what
        // `reverse` is. (Reference data, not witnessed — see the header note.)
        r().CCER = c.reverse ? std::uint32_t{IP::cc1p.mask} : 0u;

        // PSC MUST BE ZERO. In the PWM personality the prescaler divides the
        // kernel clock to make a timebase; here the counter is clocked by
        // the encoder's own edges and a prescaler would silently drop
        // counts, which is a lost position rather than a wrong frequency.
        r().PSC = 0u;
        r().ARR = c.period - 1u;
        r().EGR = IP::ug.mask;  // latch PSC/ARR
        r().CNT = 0u;
        IP::cen.set(r());
    }

    [[nodiscard]] static std::uint32_t count() {
        return static_cast<std::uint32_t>(r().CNT);
    }

    // DIR is written by the HARDWARE in this personality — the driver never
    // sets it — so reading it is reading the encoder, not reading back a
    // setting. 0 = up, 1 = down.
    [[nodiscard]] static hal::encoder_direction direction() {
        return IP::dir.read(r()) != 0u ? hal::encoder_direction::down
                                       : hal::encoder_direction::up;
    }

    static void reset() { r().CNT = 0u; }

private:
    // THE MODE NUMBERS ARE CROSSED, AND THAT IS THE MANUAL'S DOING, not a
    // typo here: SMS=1 ("encoder mode 1") counts on TI2's edges and SMS=2
    // ("encoder mode 2") counts on TI1's. Both the RM's SMS table and the
    // pinned upstream (enum/SMS in timer_v3.json) say so —
    //   Encoder_Mode_1: "counts up/down on TI2FP1 edge depending on TI1FP2 level"
    //   Encoder_Mode_2: "counts up/down on TI1FP2 edge depending on TI2FP1 level"
    // — which is exactly why this option is named after the INPUT it counts
    // and not after the mode number a caller would otherwise have to decode.
    static constexpr typename IP::smcr sms_for(
        typename encoder_opts<Inst>::count_on e) {
        using count_on = typename encoder_opts<Inst>::count_on;
        switch (e) {
            case count_on::ti1_edges: return IP::smcr::sms_encoder_2;
            case count_on::ti2_edges: return IP::smcr::sms_encoder_1;
            case count_on::both_edges: break;
        }
        return IP::smcr::sms_encoder_3;
    }
};

}  // namespace alloy::hal
