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
// TWO SHAPES, because two of them are legitimate:
//
//   exclusive<Inst, P>()   one owner, full stop. A UART has one TX pin; a
//                          second binder on it is always a bug.
//   shared<Inst, P>(w)     several claimants of ONE personality, which must
//                          agree on the block-scoped value `w`. Two PWM
//                          channels on one timer are legal; two PWM channels
//                          asking that timer for different FREQUENCIES are
//                          not, because there is one PSC/ARR pair.
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
// Claims are never released. A peripheral opened once stays owned for the life
// of the image — alloy has no close(), by design, and a handle that could be
// dropped and reopened is a lifetime question this framework does not have.

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

}  // namespace claim
}  // namespace alloy
