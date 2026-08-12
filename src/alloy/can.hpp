// User-facing CAN controller: a stateless handle over the generated FDCAN
// instance, satisfying the CanBus concept.
//
//   board::can.enable();
//   board::can.send({.id = 0x123, .len = 2, .data = {0xAB, 0xCD}});
//   alloy::can_frame rx;
//   if (board::can.receive(rx)) { ... }
//
// Acceptance filtering, after enable():
//
//   board::can.accept_only(alloy::can::match(0x123),
//                          alloy::can::match_masked(0x200, 0x7F0));
//   board::can.accept_all();   // back to the bring-up default
//
// Bring-up defaults to internal loopback (self-test without a transceiver);
// classic CAN, standard 11-bit IDs, every frame accepted.

#pragma once

#include <concepts>
#include <cstdint>

#include "alloy/concepts.hpp"
#include "alloy/core/admit.hpp"
#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the CAN IP.
#include "alloy/hal/can/can_impl.hpp"

namespace alloy::can {

// LAYER 1. Re-exported from the shared vocabulary rather than declared here,
// for the reason can_impl.hpp gives: the driver that encodes a filter cannot
// include the facade that includes the driver.
using filter = hal::can_filter;
inline constexpr std::uint32_t standard_id_mask = hal::can_standard_id_mask;

// Exactly one identifier.
[[nodiscard]] constexpr filter match(std::uint32_t id) {
    return {id, standard_id_mask};
}
// One identifier and its don't-care bits: a 0 in `mask` accepts either value.
// `match_masked(0x200, 0x7F0)` is 0x200..0x20F.
[[nodiscard]] constexpr filter match_masked(std::uint32_t id, std::uint32_t mask) {
    return {id, mask};
}

namespace detail {

// DEGREE, asked the way the layered-surface page asks it — against a generated
// number, at compile time, with both numbers in the diagnostic. They are in
// the diagnostic because they are TEMPLATE ARGUMENTS: GCC and clang print
// `[with unsigned int Asked = 29; unsigned int Capacity = 28]` above the
// static_assert, which no C++23 static_assert message can interpolate itself.
template <unsigned Asked, unsigned Capacity>
struct filters_fit {
    static_assert(Asked <= Capacity,
                  "alloy::can::accept_only: more acceptance filters than this "
                  "controller's message RAM holds — the two numbers are the "
                  "template arguments of this instantiation, and the capacity "
                  "is also readable as decltype(board::can)::filter_capacity");
};

}  // namespace detail

template <class Inst>
class controller {
public:
    // How many acceptance filters this instance can hold. Read off the
    // COMPANION message RAM, not off the controller — see the long note in
    // hal/can/st_fdcan_v1.hpp for why the controller's own RXGFC.LSS field is
    // the wrong place to ask.
    static constexpr unsigned filter_capacity = hal::can_impl<Inst>::filter_capacity;

    // Shared with a constant witness, for the reason spelled out in
    // alloy/dac.hpp: `enable()` takes no configuration, so there is nothing
    // for two claimants to contradict, and the claim exists to put the block
    // on record in its CAN personality. `send`/`receive` claim nothing.
    void enable() const {
        alloy::claim::shared<Inst, alloy::claim::personality::can>(0u);
        hal::can_impl<Inst>::enable();
    }
    [[nodiscard]] bool send(const can_frame& f) const { return hal::can_impl<Inst>::send(f); }
    [[nodiscard]] bool receive(can_frame& f) const { return hal::can_impl<Inst>::receive(f); }

    // Accept ONLY frames matching one of these filters; the controller drops
    // the rest before they ever reach a FIFO. Call after enable() — enable()
    // installs the accept-everything default and would undo this.
    //
    // Variadic rather than a span or a builder, and that is the whole point:
    // `sizeof...(F)` is a compile-time count, so "too many filters for this
    // silicon" is a static_assert against a generated number instead of a
    // runtime error nobody checks. It is also ONE call, which matters here
    // more than it reads: the list size lives in a register the controller
    // only accepts writes to inside a config window, so every extra call
    // would be another window and another moment off the bus.
    template <class... F>
    void accept_only(F... f) const {
        static_assert((std::same_as<F, filter> && ...),
                      "alloy::can::accept_only takes alloy::can::filter values — "
                      "build them with match() or match_masked()");
        static_assert(sizeof...(F) >= 1u,
                      "alloy::can::accept_only() with no filters would accept "
                      "nothing at all; say accept_all() if that is not what you meant");
        (void)detail::filters_fit<sizeof...(F), filter_capacity>{};
        // Layer 1 admits only values the wire format can carry, by default —
        // spelled at the call site rather than in a helper, for the reason
        // core/admit.hpp gives. Folded over the whole pack in ONE expression
        // so a list of literals stays a compile-time constant.
        const bool ok = ((f.id <= standard_id_mask && f.mask <= standard_id_mask) && ...);
        if (__builtin_constant_p(ok) && !ok) {
            alloy::core::admit::can_filter_id();
        }
        if (!ok) {
            alloy::trap<alloy::trap_code::impossible_config>();
        }
        const filter list[sizeof...(F)] = {f...};
        hal::can_impl<Inst>::accept_only(list, sizeof...(F));
    }

    // Back to the bring-up default: every frame reaches the RX FIFO.
    void accept_all() const { hal::can_impl<Inst>::accept_all(); }
};

// No-op stand-in for boards without a can role — keeps board::can.*()
// compiling everywhere (send/receive report failure).
//
// NOTE the filter surface here has no capacity check, deliberately. A board
// with no CAN role compiles the same `accept_only(...)` line the CAN board
// does — that is what `if constexpr (board::caps::can)` costs in a non-template
// main(), where the discarded branch is still compiled — so a static_assert
// against capacity 0 would turn the zero-preprocessor promise into a build
// break. Nothing is programmed because there is nothing to program.
struct null_controller {
    static constexpr unsigned filter_capacity = 0u;
    void enable() const {}
    [[nodiscard]] bool send(const can_frame&) const { return false; }
    [[nodiscard]] bool receive(can_frame&) const { return false; }
    template <class... F>
    void accept_only(F...) const {}
    void accept_all() const {}
};

}  // namespace alloy::can
