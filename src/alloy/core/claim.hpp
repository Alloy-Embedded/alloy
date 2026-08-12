// Who owns a peripheral INSTANCE, and in which personality.
//
// The layered config surface (docs/reference/peripheral-surface.md) makes a
// CALL SITE impossible to lie in: a knob the silicon lacks is not a member of
// `opts<Inst>`, and a value too big for a register field is a static_assert.
// It says nothing about who owns the peripheral. Two `uart::bind<>`
// specializations naming the same `usart2_t` — a second, legitimately routed
// pin pair — used to carry one `detail_opened` flag EACH, so opening both
// compiled clean and the second `open()` silently reprogrammed the port under
// the first handle.
//
// This header moves the flag from the BINDER TYPE to the INSTANCE. `owner<Inst>`
// is one inline variable per instance, so it is one object across the whole
// image however many translation units, binders or roles name that peripheral.
//
// TWO SCOPES, because a peripheral is not always the resource being contested:
//
//   owner<Inst>            a whole peripheral — usart2, i2c1, tim2.
//   sub_owner<Inst, N>     one numbered part of it — TIM2 channel 1, DMA1
//                          channel 3, EXTI line 5. Keyed on the instance and
//                          the ORDINAL and nothing else, which is the entire
//                          point: see the comment on sub_exclusive().
//
// TWO SHAPES AT EACH SCOPE, because both are legitimate at both:
//
//   exclusive<Inst, P>()   one owner, full stop. A UART has one TX pin; a
//                          second binder on it is always a bug.
//   shared<Inst, P>(w)     several claimants of ONE personality, which must
//                          agree on the block-scoped value `w`. Two PWM
//                          channels on one timer are legal; two PWM channels
//                          asking that timer for different FREQUENCIES are
//                          not, because there is one PSC/ARR pair.
//   sub_exclusive<..>()    the same two, one level down. The `shared` twin was
//   sub_shared<..>(w)      argued away when this file was written and the EXTI
//                          line refuted the argument — see the comment above
//                          sub_shared().
//
// A PERSONALITY is a mutually exclusive whole-block mode — timer as PWM vs as
// encoder, USART as UART vs as SPI master. Those are different binders in
// different namespaces, and this is what stops a program naming two of them
// for one block.
//
// RUNTIME, not compile time, and for the reason NORTH_STAR guard #7 already
// concedes: C++ cannot see across translation units at compile time. What is
// new is that the check is now per instance and cross-TU, where before it was
// per binder type and per TU. The board generator refuses the same conflict
// EARLIER, when it can see it (emit/board.py, role personalities).
//
// Claims are released only where the facade HAS a release. A peripheral opened
// once stays owned for the life of the image — alloy has no close(), by design,
// and a handle that could be dropped and reopened is a lifetime question this
// framework does not have. The one exception found so far is a sub-resource
// with a documented disarm (`gpio::input::clear_on_edge()` frees an EXTI line);
// see sub_release() at the bottom of this file, and note that it is offered at
// the SUB scope only, because that is the only scope where anything in alloy
// can give a resource back.

#pragma once

#include <cstdint>

namespace alloy {

// Why a guard fired. The review found `open({.baud = 0})` trapping at the
// DOUBLE-OPEN `udf`, so the crash pointed at the wrong bug. The reason code
// rides into the trap in a register — `movs rN, #code`, two bytes, in the cold
// path — which is enough for a disassembly or a fault report that stacks the
// register set to tell the guards apart.
//
// HONEST LIMIT: it does not give each guard its own PC. GCC tail-merges the
// `udf` itself, so the trap ADDRESS is still shared; what is no longer shared
// is the basic block and the register value that reaches it. Measured on the
// G071RB: the two claim failures are two distinct blocks, the second of which
// is literally `movs r3, #2` then a branch into the merged `udf`.
enum class trap_code : std::uint32_t {
    instance_owned = 1,         // a second binder opened a port already open
    personality_conflict = 2,   // one block, two mutually exclusive modes
    block_config_conflict = 3,  // one block, two channels, disagreeing config
    impossible_config = 4,      // a Layer-1 value no divisor can reach
    not_open = 5,               // reconfigure() on a port nobody opened
    opts_mismatch = 6,          // reconfigure<Opts> disagreeing with open<Opts>
    sub_resource_owned = 7,     // a second binder opens one channel of a block
    sub_config_conflict = 8,    // one channel, two claimants, disagreeing config
};

template <trap_code Code>
[[noreturn]] inline void trap() {
    // `volatile` with an input operand forces the code into a register and
    // forbids the optimizer from folding two different traps together.
    __asm__ __volatile__("" ::"r"(static_cast<std::uint32_t>(Code)));
    __builtin_trap();
}

namespace claim {

// The mutually exclusive whole-block modes alloy models. This is a CLOSED
// vocabulary on purpose: a personality is a facade (`alloy::uart`,
// `alloy::pwm`, ...), and alloy ships the facades. `user_a`/`user_b` exist so
// an out-of-tree facade can claim a block without patching this header.
//
// ONE ENUMERATOR PER FACADE, NOT PER BUS. A second personality of the same
// bus needs its own value or the conflict it exists to catch will not be
// caught: `spi::slave::bind` and `spi::bind` on one instance must NOT both
// read as `spi`, or they agree and neither traps. `spi_slave`, `i2c_slave`
// and `capture` join this list on the day their facade does.
//
// AND THE SAME VOCABULARY THE BOARD GENERATOR USES. `emit/board.py`'s
// ROLE_PERSONALITY maps each board role to one of these names, and
// `tools/alloy/tests/test_emit.py` fails if it ever names one that is not in
// this list — two halves of one rule cannot be allowed to drift apart just
// because one of them is written in Python. That test is why `ethernet` is
// here (it was in the generator and nowhere else) and why `dma` is (it was a
// facade with no enumerator at all).
//
// `none` is 0 so the flag lives in .bss and costs no startup code.
enum class personality : std::uint8_t {
    none = 0,
    uart,
    i2c,
    spi,
    adc,
    dac,
    pwm,
    encoder,
    capture,
    can,
    wdt,
    rtc,
    flash,
    dma,
    ethernet,
    exti,
    user_a,
    user_b,
};

// One byte per peripheral instance, for the whole image. Instantiated only for
// instances something actually claims, so a chip's 65 peripherals cost nothing
// for the 3 a program opens.
template <class Inst>
inline personality owner = personality::none;

// The block-scoped value shared claimants must agree on. Instantiated only by
// shared<>(), so an exclusively claimed instance costs one byte, exactly what
// the per-binder `bool` cost before.
template <class Inst>
inline std::uint32_t witness = 0;

// One owner, full stop. ONE comparison on the path that succeeds — telling the
// two failures apart happens inside the branch that already lost, so the
// distinction costs nothing in a program that is correct.
template <class Inst, personality P>
inline void exclusive() {
    if (owner<Inst> != personality::none) {
        if (owner<Inst> == P) {
            alloy::trap<trap_code::instance_owned>();
        }
        alloy::trap<trap_code::personality_conflict>();
    }
    owner<Inst> = P;
}

// Several claimants of one personality, agreeing on the block-scoped value.
template <class Inst, personality P>
inline void shared(std::uint32_t w) {
    if (owner<Inst> == personality::none) {
        owner<Inst> = P;
        witness<Inst> = w;
        return;
    }
    if (owner<Inst> != P) {
        alloy::trap<trap_code::personality_conflict>();
    }
    if (witness<Inst> != w) {
        alloy::trap<trap_code::block_config_conflict>();
    }
}

// True when this personality already owns the instance — the precondition for
// reconfiguring a running port.
template <class Inst, personality P>
[[nodiscard]] inline bool held() {
    return owner<Inst> == P;
}

// ── The second scope: one numbered part of a block ──────────────────────
//
// `owner<Inst>` answers "who owns TIM2". Nothing answered "who owns TIM2
// CHANNEL 1", and a timer channel is a resource in its own right: it has its
// own CCR, its own CCMR mode field, and its own output pin.
//
// KEYED ON THE INSTANCE AND THE ORDINAL, NOTHING ELSE. That is the whole
// point. `pwm::bind` is templated on <Inst, Channel, Pin, Signal, Clock>, so a
// flag stored in the BINDER gives one channel as many flags as there are pins
// routed to it — the identical defect `owner<>` retired at block scope, one
// level down and still live in shipped code until this existed. TIM2_CH1 has
// four routes on the G0B1RE (PA0, PA5, PA15, PC4), so the two binders that
// defeat a per-binder flag are both legal to write and both compile.
//
// The ordinal is `unsigned` and its meaning belongs to the caller: for `pwm`
// it is the timer channel `bind` already takes, for `dma` the channel index.
// Nothing here checks it against a `feat` count — that is a DEGREE question
// the facade answers at compile time, not an ownership question.
template <class Inst, unsigned Sub>
inline personality sub_owner = personality::none;

// The value a SHARED sub-resource's claimants must agree on. Instantiated only
// by sub_shared(), so a channel claimed exclusively costs one byte, exactly as
// before this existed.
template <class Inst, unsigned Sub>
inline std::uint32_t sub_witness = 0;

// One owner of one sub-resource, full stop. Two claimants of the same DMA
// channel or the same timer channel are always a bug, so this shape does not
// admit a second one at all.
template <class Inst, unsigned Sub, personality P>
inline void sub_exclusive() {
    if (sub_owner<Inst, Sub> != personality::none) {
        if (sub_owner<Inst, Sub> == P) {
            alloy::trap<trap_code::sub_resource_owned>();
        }
        alloy::trap<trap_code::personality_conflict>();
    }
    sub_owner<Inst, Sub> = P;
}

template <class Inst, unsigned Sub, personality P>
[[nodiscard]] inline bool sub_held() {
    return sub_owner<Inst, Sub> == P;
}

// ── ...and this scope needed a `shared` twin after all ───────────────────
//
// WHAT THIS FILE USED TO CLAIM, in as many words: "There is deliberately no
// sub_shared. A sub-resource is a sub-resource BECAUSE it has no block-scoped
// register for two claimants to disagree about." That was derived from the two
// sub-resources alloy had — a timer channel and a DMA channel — and the EXTI
// LINE refutes it. A line has its own port-select field (EXTICR), its own
// trigger pair and its own callback slot, and the claimants competing for it
// are PINS: PA5 and PB5 are both routed to line 5, and until this existed
// arming both compiled clean, silently handed the line to PB5, and left PA5's
// handle reporting PB5's edges as its own (proven under Renode in 9.1 s).
//
// So the sub scope gets the same two shapes the block scope has, and the
// witness is what says WHICH claimant: the port index for an EXTI line. Same
// value = the same pin re-arming, which the driver documents as legal;
// different value = a second pin stealing the line, which is the bug.
template <class Inst, unsigned Sub, personality P>
inline void sub_shared(std::uint32_t w) {
    if (sub_owner<Inst, Sub> == personality::none) {
        sub_owner<Inst, Sub> = P;
        sub_witness<Inst, Sub> = w;
        return;
    }
    if (sub_owner<Inst, Sub> != P) {
        alloy::trap<trap_code::personality_conflict>();
    }
    if (sub_witness<Inst, Sub> != w) {
        alloy::trap<trap_code::sub_config_conflict>();
    }
}

// AND THE ONE RELEASE IN THIS FILE, which is also new evidence: "claims are
// never released" (the header comment above) held while every claim was made
// by an open() with no close() to match. The EXTI line has one — disarming a
// pin's edge interrupt genuinely frees the line, and `gpio::input::
// clear_on_edge()` is a shipped, documented call. Refusing PB5 forever because
// PA5 was armed once and let go would be a new lie in place of the old one.
//
// Releasing a line NOBODY owns stays a no-op, because disarming a pin that was
// never armed was always harmless. Releasing a line owned by a DIFFERENT
// claimant is the bug this catches: `pb5.clear_on_edge()` used to silently
// disarm PA5's interrupt.
template <class Inst, unsigned Sub, personality P>
inline void sub_release(std::uint32_t w) {
    if (sub_owner<Inst, Sub> == personality::none) {
        return;
    }
    if (sub_owner<Inst, Sub> != P) {
        alloy::trap<trap_code::personality_conflict>();
    }
    if (sub_witness<Inst, Sub> != w) {
        alloy::trap<trap_code::sub_config_conflict>();
    }
    sub_owner<Inst, Sub> = personality::none;
}

}  // namespace claim
}  // namespace alloy
