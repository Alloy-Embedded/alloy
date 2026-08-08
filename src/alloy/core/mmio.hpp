// Memory-mapped I/O primitives.
//
// Generated IP headers (alloy::ip::*) describe register layouts as plain
// structs of rw32/ro32/wo32 members plus `field<...>` accessors. These
// primitives are the only place raw volatile access lives.

#pragma once

#include <cstdint>
#include <type_traits>

namespace alloy {

using rw32 = volatile std::uint32_t;
using ro32 = volatile const std::uint32_t;
using wo32 = volatile std::uint32_t;

// Not every register is a word. Renesas RL78, AVR and MSP430 declare 8- and
// 16-bit SFRs, several of them explicitly single-width — a 32-bit read-modify-
// write on an 8-bit register does not merely waste a cycle, it touches the
// three registers next to it.
using rw16 = volatile std::uint16_t;
using ro16 = volatile const std::uint16_t;
using wo16 = volatile std::uint16_t;
using rw8 = volatile std::uint8_t;
using ro8 = volatile const std::uint8_t;
using wo8 = volatile std::uint8_t;

// The width of the register a pointer-to-member points at. Deduced rather than
// declared: the generated struct already says whether a register is rw8 or
// rw32, and asking the member pointer means a field can never disagree with
// the register it belongs to.
namespace detail {
template <class T>
struct member_word;
template <class Regs, class Word>
struct member_word<Word Regs::*> {
    using type = std::remove_cv_t<Word>;
};
}  // namespace detail

template <auto Member>
using reg_word_t = typename detail::member_word<decltype(Member)>::type;

// A bitfield inside a register, addressed by pointer-to-member so the field
// is bound to its register at compile time and cannot be applied to the
// wrong one.
template <auto Member, unsigned Pos, unsigned Width = 1>
struct field_t {
    //: The register's own width, from its declaration.
    using word = reg_word_t<Member>;
    static constexpr unsigned bits = sizeof(word) * 8u;

    static_assert(Width >= 1 && Pos + Width <= bits,
                  "field does not fit the register it names");

    static constexpr unsigned pos = Pos;

    // All mask arithmetic happens in std::uint32_t and is narrowed at the end.
    // Doing it in `word` would be wrong twice over: `uint16_t{1} << 15` promotes
    // to a SIGNED int, and on a 16-bit-int target that shift is undefined — the
    // exact target this width support exists for.
    static constexpr std::uint32_t wide_raw_mask =
        (Width >= 32) ? 0xFFFF'FFFFu : ((std::uint32_t{1} << Width) - 1u);
    static constexpr std::uint32_t wide_mask = wide_raw_mask << Pos;

    static constexpr word raw_mask = static_cast<word>(wide_raw_mask);
    static constexpr word mask = static_cast<word>(wide_mask);

    template <class Regs>
    static void write(Regs& r, std::uint32_t value) {
        const std::uint32_t cur = static_cast<std::uint32_t>(r.*Member);
        r.*Member = static_cast<word>((cur & ~wide_mask) |
                                      ((value & wide_raw_mask) << Pos));
    }

    template <class Regs>
    [[nodiscard]] static std::uint32_t read(Regs& r) {
        return (static_cast<std::uint32_t>(r.*Member) & wide_mask) >> Pos;
    }

    template <class Regs>
    static void set(Regs& r) requires (Width == 1) {
        r.*Member = static_cast<word>(static_cast<std::uint32_t>(r.*Member) | wide_mask);
    }

    template <class Regs>
    static void clear(Regs& r) {
        r.*Member = static_cast<word>(static_cast<std::uint32_t>(r.*Member) & ~wide_mask);
    }
};

template <auto Member, unsigned Pos, unsigned Width = 1>
inline constexpr field_t<Member, Pos, Width> field{};

// ── Typed register flags ────────────────────────────────────────────────
//
// A per-register scoped enum whose values ARE bit masks (single-bit fields)
// or shifted field values (multi-bit fields, e.g. CHRL_8 = 3u << 6). The
// generated IP header emits one such enum per register and opts it in below,
// so a driver writes `r().CR = cr::rxen | cr::txen;` — reads like the manual,
// and mixing registers (`cr::txen | mr::chrl_8`) is a compile error. Only
// opted-in enums get these operators; unrelated enum class types are untouched.
// Zero runtime cost: every op folds at compile time to the same OR of masks.

// The generated header specializes this to true for each register-flags enum.
template <class E>
inline constexpr bool reg_flags_enabled = false;

// A set of bits belonging to one register's flag enum E.
template <class E>
class flags {
    std::uint32_t bits_{0};

public:
    constexpr flags() = default;
    constexpr flags(E e) : bits_(static_cast<std::uint32_t>(e)) {}  // NOLINT: implicit by design
    constexpr explicit flags(std::uint32_t raw) : bits_(raw) {}

    [[nodiscard]] constexpr std::uint32_t value() const { return bits_; }
    constexpr operator std::uint32_t() const { return bits_; }  // NOLINT: write whole register
    [[nodiscard]] constexpr bool any(E e) const {
        return (bits_ & static_cast<std::uint32_t>(e)) != 0u;
    }
};

template <class E>
    requires reg_flags_enabled<E>
constexpr flags<E> operator|(E a, E b) {
    return flags<E>{static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)};
}
template <class E>
    requires reg_flags_enabled<E>
constexpr flags<E> operator|(flags<E> a, E b) {
    return flags<E>{a.value() | static_cast<std::uint32_t>(b)};
}
template <class E>
    requires reg_flags_enabled<E>
constexpr flags<E> operator|(E a, flags<E> b) {
    return flags<E>{static_cast<std::uint32_t>(a) | b.value()};
}
template <class E>
    requires reg_flags_enabled<E>
constexpr flags<E> operator|(flags<E> a, flags<E> b) {
    return flags<E>{a.value() | b.value()};
}
// Bit test against a read register word: `if (r().SR & sr::txrdy) ...`.
template <class E>
    requires reg_flags_enabled<E>
[[nodiscard]] constexpr std::uint32_t operator&(std::uint32_t reg, E e) {
    return reg & static_cast<std::uint32_t>(e);
}
// Mask to clear: `r().MR = raw & ~mr::wdrsten;`.
template <class E>
    requires reg_flags_enabled<E>
[[nodiscard]] constexpr std::uint32_t operator~(E e) {
    return ~static_cast<std::uint32_t>(e);
}

// Field of a register accessed by computed address (register ARRAYS like
// RP2040 IO_BANK0 GPIOx_CTRL), where pointer-to-member binding is impossible.
struct raw_field {
    unsigned pos;
    unsigned width;

    [[nodiscard]] constexpr std::uint32_t raw_mask() const {
        return (width == 32u) ? 0xFFFF'FFFFu : ((std::uint32_t{1} << width) - 1u);
    }
    [[nodiscard]] constexpr std::uint32_t mask() const { return raw_mask() << pos; }

    void write(rw32& r, std::uint32_t value) const {
        r = (r & ~mask()) | ((value & raw_mask()) << pos);
    }
    [[nodiscard]] std::uint32_t read(const volatile std::uint32_t& r) const {
        return (r & mask()) >> pos;
    }
};

// Element i of a register array: base + offset + i*stride.
inline rw32& reg_at(std::uintptr_t base, std::uintptr_t offset,
                    unsigned stride, unsigned i) {
    return *reinterpret_cast<rw32*>(base + offset +
                                    static_cast<std::uintptr_t>(stride) * i);
}

}  // namespace alloy
