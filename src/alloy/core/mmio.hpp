// Memory-mapped I/O primitives.
//
// Generated IP headers (alloy::ip::*) describe register layouts as plain
// structs of rw32/ro32/wo32 members plus `field<...>` accessors. These
// primitives are the only place raw volatile access lives.

#pragma once

#include <cstdint>

namespace alloy {

using rw32 = volatile std::uint32_t;
using ro32 = volatile const std::uint32_t;
using wo32 = volatile std::uint32_t;

// A bitfield inside a register, addressed by pointer-to-member so the field
// is bound to its register at compile time and cannot be applied to the
// wrong one.
template <auto Member, unsigned Pos, unsigned Width = 1>
struct field_t {
    static_assert(Pos < 32 && Width >= 1 && Width <= 32 && Pos + Width <= 32);

    static constexpr std::uint32_t raw_mask =
        (Width == 32) ? 0xFFFF'FFFFu : ((1u << Width) - 1u);
    static constexpr std::uint32_t mask = raw_mask << Pos;

    template <class Regs>
    static void write(Regs& r, std::uint32_t value) {
        r.*Member = (r.*Member & ~mask) | ((value & raw_mask) << Pos);
    }

    template <class Regs>
    [[nodiscard]] static std::uint32_t read(Regs& r) {
        return (r.*Member & mask) >> Pos;
    }

    template <class Regs>
    static void set(Regs& r) requires (Width == 1) {
        r.*Member = r.*Member | mask;
    }

    template <class Regs>
    static void clear(Regs& r) {
        r.*Member = r.*Member & ~mask;
    }
};

template <auto Member, unsigned Pos, unsigned Width = 1>
inline constexpr field_t<Member, Pos, Width> field{};

}  // namespace alloy
