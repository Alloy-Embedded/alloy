// encoder_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per IP version, constrained on the instance's
// IP tag type, exactly like pwm_impl<Inst>.
//
// AND THAT SENTENCE IS THE POINT: `pwm_impl<tim2_t>` and
// `encoder_impl<tim2_t>` are two drivers over ONE block. They are not two
// modes of one driver and there is no flag that switches between them,
// because the thing that differs is not a setting — it is what the counter
// MEANS. In PWM the counter is a time base: PSC and ARR set a frequency,
// every channel is an output, and the block-scoped value two channels must
// agree on is that frequency. In encoder mode the counter is a POSITION:
// PSC must be zero because the encoder's edges are the clock, ARR is a
// modulo rather than a period, DIR is written by the hardware instead of by
// the driver, and channels 1 and 2 are inputs. Nothing survives the change
// except the register addresses.
//
// That is why the exclusion is enforced at the personality (alloy::claim)
// and not left to a `mode` field: a field can be set twice, a personality
// cannot be claimed twice.

#pragma once

#include <cstdint>

namespace alloy::hal {

// ── Layer 1 vocabulary ──────────────────────────────────────────────────

// Which way the counter last moved. Two values, because a quadrature
// counter has two: it is not "stopped/up/down" — a counter that has not
// moved still has a last direction, and inventing a third value would make
// every driver decide how long "stopped" lasts.
enum class encoder_direction : std::uint8_t { up, down };

// The portable encoder shape.
//
// ADMISSION TEST, and it had to be applied with n=1 driver, which is the
// honest caveat: alloy ships exactly one encoder_impl today, so "every
// driver programs it" cannot distinguish a portable knob from an ST one.
// The test used instead is STRUCTURAL — is this knob a property of
// *counting quadrature edges*, or a property of *this silicon*:
//
//   period   yes. Every hardware quadrature counter counts modulo
//            something. Naming the modulus is what turns a raw count into a
//            position, and a driver that could not honour it would have to
//            fake it in software on every read.
//   reverse  yes. Which way is "forwards" is a property of how the encoder
//            was wired, not of the peripheral. Every counter that can count
//            down can be made to count down for the other rotation.
//
// NOT here, and it is the more interesting half:
//   * resolution / which input is counted. ST offers three encoder modes
//     and they are NOT x1/x2/x4 — modes 1 and 2 both quadruple nothing and
//     double the count, but differ in WHICH input's edges are counted,
//     which a generic "x2" cannot say. A portable name would have to lie
//     about the value set. It is Layer 2: `Role::opts`.
//   * the input filter. Not a layering decision at all — the field is not
//     curated and cannot be, see alloy-devices registers/st/tim_gp16.yaml.
//   * an index/Z channel. Real, wanted, and a third pin plus a reset
//     source; it is a binder tag when it arrives, not a bool here (same
//     reason as uart::de<Pin>).
struct encoder_config {
    //: Counts before the counter wraps. Set it to the encoder's counts per
    //: revolution and `count()` is an angle index with no software modulo.
    //: DEFAULT is the full range of a 16-bit counter — and on an instance
    //: whose counter is narrower this default is a COMPILE ERROR rather
    //: than a silent truncation, which is the behaviour we want from it.
    std::uint32_t period = 65'536;
    //: Swap which rotation counts up.
    bool reverse = false;
};

// ── Layer 2 ─────────────────────────────────────────────────────────────
//
// Empty primary, so `encoder_opts<Inst>{}` compiles for an IP nobody has
// taught any vendor knobs. Each driver header declares its own partial
// specialization listing exactly the knobs its IP has. Same contract as
// hal::uart_opts<Inst>, including the unenforced half: a feature that two
// IPs share must get the same member name in both, and nothing checks it.
template <class Inst>
struct encoder_opts {};

template <class Inst>
struct encoder_impl;

}  // namespace alloy::hal
