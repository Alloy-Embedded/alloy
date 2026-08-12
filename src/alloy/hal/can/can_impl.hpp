// can_impl<Inst> — primary template, intentionally undefined.
// One partial specialization per CAN IP version, constrained on the instance's
// IP tag type. Operations: enable(), send(frame), receive(frame), and the
// acceptance-filter pair accept_all()/accept_only(list, n).
//
// This header also carries the SHARED VOCABULARY the facade and the drivers
// both need — the same arrangement uart_impl.hpp uses for `hal::parity`. A
// type the facade re-exports as `alloy::can::filter` cannot live in the facade
// itself: the driver that encodes it into silicon would then have to include
// the facade that includes the driver.

#pragma once

#include <cstdint>

namespace alloy::hal {

// The classic-CAN standard identifier is eleven bits (ISO 11898-1) — the same
// fact `alloy::can_frame::id` already states in prose. It is a PROTOCOL width,
// not a silicon one: no controller widens it, and a controller that offers
// 29-bit extended identifiers offers them as a separate frame format, not as a
// wider standard id. Drivers cross-check it against their own curated field
// width, so a mistake in either place is a compile error rather than a filter
// that silently matches the wrong frames.
inline constexpr std::uint32_t can_standard_id_mask = 0x7FFu;

// An acceptance filter, in the only shape every CAN controller in the world
// agrees on: an identifier, and which of its bits have to match.
//
// `mask` is the ST/Bosch/Microchip/NXP convention — a 1 bit MUST match, a 0
// bit is don't-care — so `{0x123, can_standard_id_mask}` is one exact id and
// `{0x200, 0x7F0}` is the sixteen ids 0x200..0x20F.
struct can_filter {
    std::uint32_t id{};
    std::uint32_t mask{};

    // The same predicate the silicon implements, available to host tests and
    // to any backend that has to filter in software (a CAN-over-USB adapter,
    // a controller whose filter bank is already full).
    [[nodiscard]] constexpr bool matches(std::uint32_t rx_id) const {
        return ((rx_id ^ id) & mask) == 0u;
    }
};

template <class Inst>
struct can_impl;

}  // namespace alloy::hal
