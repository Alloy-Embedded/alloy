// User-facing CAN controller: a stateless handle over the generated FDCAN
// instance, satisfying the CanBus concept.
//
//   board::can.enable();
//   board::can.send({.id = 0x123, .len = 2, .data = {0xAB, 0xCD}});
//   alloy::can_frame rx;
//   if (board::can.receive(rx)) { ... }
//
// Bring-up defaults to internal loopback (self-test without a transceiver);
// classic CAN, standard 11-bit IDs.

#pragma once

#include "alloy/concepts.hpp"
#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for the chip that actually has the CAN IP.
#include "alloy/hal/can/can_impl.hpp"

namespace alloy::can {

template <class Inst>
class controller {
public:
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
};

// No-op stand-in for boards without a can role — keeps board::can.*()
// compiling everywhere (send/receive report failure).
struct null_controller {
    void enable() const {}
    [[nodiscard]] bool send(const can_frame&) const { return false; }
    [[nodiscard]] bool receive(can_frame&) const { return false; }
};

}  // namespace alloy::can
