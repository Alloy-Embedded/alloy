// Unique-device-ID reader for the STM32 UID v1 block (every STM32 family: 96
// factory-programmed bits, read-only, no clock gate, no reset, no IRQ).
//
// The whole driver is three loads. What makes it a driver rather than three
// literals is that the ADDRESS is a per-chip fact — 0x1FFF_7590 on the G0B1RE,
// somewhere else on an F4 or an L4 — so it comes from the instance descriptor
// and never from this file (NORTH_STAR guard #1).

#pragma once

#include <array>
#include <concepts>
#include <cstdint>

#include "alloy/hal/uid/uid_impl.hpp"
#include "alloy/ip/st/uid_v1.hpp"

namespace alloy::hal {

template <class Inst>
    requires std::same_as<typename Inst::ip, alloy::ip::st::uid_v1>
struct uid_impl<Inst> {
    using IP = typename Inst::ip;

    // The generated degree fact meets the driver. This specialization names
    // three registers, so it is correct only for an IP whose data says three;
    // a uid_v2 with four words would need its own specialization, and this
    // line is what makes that a build failure instead of a silent truncation.
    static_assert(Inst::feat::id_words == 3u,
                  "st_uid_v1 reads UID0/UID1/UID2 — this instance's data "
                  "declares a different number of identifier words");

    static std::array<std::uint32_t, Inst::feat::id_words> read() {
        const auto& r = *reinterpret_cast<const typename IP::regs*>(Inst::base);
        // Low word first, so index i is UID[32i+31 : 32i] — the order the
        // manual numbers them in, not the order they are printed in.
        return {r.UID0, r.UID1, r.UID2};
    }
};

}  // namespace alloy::hal
