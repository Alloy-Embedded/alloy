// Does the hardware CRC compute the SAME number as the software one?
//
// That question is the whole reason alloy has a CRC facade, and it is the one
// question no board in this project can answer: no STM32 is on hand, and Renode
// 1.16.1 does not model the block (its stm32g0 platform carries a bare address
// Tag for the CRC region, which reads back zero — an emulated run would
// "verify" every image as having checksum 0xFFFF'FFFF, which is worse than no
// run at all because it passes).
//
// So this file answers it the only way left, and is careful about what that is
// worth. It builds a MODEL of the block from its documented semantics — seed,
// polynomial, MSB-first shift, input reversal by granularity, output reversal —
// drives the SHIPPED feed sequence (alloy::hal::crc_detail::feed) and the
// SHIPPED parameters (crc_detail::iso_hdlc) into it, and compares the result
// with alloy::ota::crc::crc32_of over every length from 0 to 64 bytes at four
// alignments, plus the standard check vector.
//
// WHAT THAT PROVES: the CONFIGURATION alloy chose is the configuration that
// computes CRC-32/ISO-HDLC. If REV_IN were set to the wrong granularity, or the
// words were assembled in the wrong order, or the final XOR were forgotten,
// these tests fail — and the negative controls at the bottom show they fail,
// rather than being ways of writing `assert(true)`.
//
// "OR THE FINAL XOR WERE FORGOTTEN" WAS NOT TRUE WHEN IT WAS FIRST WRITTEN, and
// that is worth recording next to the claim it corrects. The xorout was one
// expression inside st_crc_v3.hpp::value(), which this file cannot include, so
// the only thing the negative control below tested was the MODEL's copy of it:
// deleting the driver's left all 390 tests green. It now lives in
// crc_detail::finalize() — same split as st_wwdg_detail.hpp and
// st_tim_adv_dtg.hpp — the model routes through it, the driver calls it, and
// crc_finalize_is_the_xorout_the_silicon_has_no_register_for tests it head-on.
// The gap that remains is that nothing here can prove the DRIVER still calls
// it; see crc_impl.hpp's note on finalize.
//
// WHAT IT DOES NOT PROVE: that the silicon behaves as its manual describes.
// Nothing available to this project proves that, and the driver header says so
// in the same words.
//
// It also does NOT read the generated `alloy/ip/st/crc_v3.hpp` — host tests
// build against src/ only, with no chip database — so the bridge from "reverse
// the input by byte" to "CR bits 6:5 = 01" is checked elsewhere: by
// alloy-devices' tests/test_crc_uid_data.py, which pins REV_IN's encoding and
// POL's and INIT's reset values, and by the driver naming those enumerators
// instead of literals. Measured on the built firmware for the record: the
// nucleo_g0b1re device_id example emits `str r3, [r4, #8]` with r3 = 0xA0
// (REV_OUT | REV_IN_BYTE | POLYSIZE_32), INIT = 0xFFFFFFFF, POL = 0x04C11DB7,
// then `mvns` on the value read back — which is this configuration, in machine
// code.

#include <alloy/crc.hpp>
#include <alloy/hal/crc/crc_impl.hpp>
#include <alloy/ota/crc32.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "alloy_test.hpp"

namespace {

using spec = alloy::hal::crc_detail::iso_hdlc;

std::uint32_t bitrev32(std::uint32_t v) {
    std::uint32_t r = 0;
    for (int i = 0; i < 32; ++i) {
        r = (r << 1) | ((v >> i) & 1u);
    }
    return r;
}

// The STM32 CRC calculation unit, as its manual describes it and with nothing
// else in it. Deliberately written from the SEMANTICS (a shift register with a
// polynomial, an input permutation, an output permutation) rather than from any
// table or reference implementation, so that agreeing with zlib's answer is a
// result and not a restatement.
class block_model {
public:
    std::uint32_t INIT = 0;
    std::uint32_t POL = 0;
    unsigned reverse_input_bits = 0;  // 0 = CR.REV_IN NONE; else 8/16/32
    bool reverse_output = false;

    void reset() { dr_ = INIT; }

    // One write to CRC_DR, of `bits` width (8, 16 or 32 — the three access
    // widths the block architects at that one address).
    void write_dr(std::uint32_t v, unsigned bits) {
        v = permute_in(v, bits);
        for (int i = static_cast<int>(bits) - 1; i >= 0; --i) {
            const std::uint32_t top = (dr_ >> 31) & 1u;
            const std::uint32_t bit = (v >> i) & 1u;
            dr_ = (dr_ << 1) ^ (((top ^ bit) != 0u) ? POL : 0u);
        }
    }

    [[nodiscard]] std::uint32_t read_dr() const { return reverse_output ? bitrev32(dr_) : dr_; }

private:
    std::uint32_t permute_in(std::uint32_t v, unsigned bits) const {
        if (reverse_input_bits == 0u) {
            return v;
        }
        // A byte write cannot be word-reversed: the granularity is capped by
        // the width of the access, which is what makes an 8-bit write to DR
        // meaningful under REV_IN = WORD too.
        const unsigned g = reverse_input_bits > bits ? bits : reverse_input_bits;
        std::uint32_t out = 0;
        for (unsigned lane = 0; lane * g < bits; ++lane) {
            const unsigned sh = lane * g;
            const std::uint32_t mask = (g >= 32u) ? 0xFFFF'FFFFu : ((std::uint32_t{1} << g) - 1u);
            std::uint32_t lane_v = (v >> sh) & mask;
            std::uint32_t rev = 0;
            for (unsigned i = 0; i < g; ++i) {
                rev = (rev << 1) | ((lane_v >> i) & 1u);
            }
            out |= rev << sh;
        }
        return out;
    }

    std::uint32_t dr_ = 0;
};

// The sink alloy::hal::crc_detail::feed() drives — the same shape the ST
// driver's dr_sink has, pointed at the model instead of at a volatile address.
struct model_sink {
    block_model* m;
    void word(std::uint32_t w) const { m->write_dr(w, 32); }
    void byte(std::uint8_t b) const { m->write_dr(b, 8); }
};

// A whole checksum the way the driver computes one: configure, reset, feed, and
// apply the xorout the silicon does not have.
std::uint32_t model_crc(std::span<const std::uint8_t> data,
                        unsigned reverse_input_bits = spec::reverse_input_bits,
                        bool reverse_output = spec::reverse_output,
                        bool apply_xorout = true) {
    block_model m;
    m.INIT = spec::seed;
    m.POL = spec::polynomial;
    m.reverse_input_bits = reverse_input_bits;
    m.reverse_output = reverse_output;
    m.reset();
    alloy::hal::crc_detail::feed(model_sink{&m}, data.data(), data.size());
    // finalize() is the SHIPPED xorout — the same function st_crc_v3.hpp's
    // value() calls — so every equivalence check below runs through the line
    // the driver runs through, instead of through a copy of it.
    return apply_xorout ? alloy::hal::crc_detail::finalize(m.read_dr()) : m.read_dr();
}

// A deterministic pseudo-random blob — no <random>, so the bytes are the same
// on every host and a failure is reproducible from the test name alone.
struct blob {
    std::uint8_t b[200]{};
    blob() {
        std::uint32_t x = 0x1234'5678u;
        for (auto& v : b) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            v = static_cast<std::uint8_t>(x);
        }
    }
};

}  // namespace

// ── the claim ───────────────────────────────────────────────────────────

ALLOY_TEST(crc_hardware_configuration_matches_the_software_crc32) {
    const blob data;
    // Every length 0..64 at four offsets: 0-3 covers the tail cases (a length
    // that is not a multiple of four) and the unaligned starts an OTA payload
    // actually has, since it begins wherever a 32-byte header ended.
    for (std::size_t len = 0; len <= 64u; ++len) {
        for (std::size_t off = 0; off < 4u; ++off) {
            const std::span<const std::uint8_t> s{data.b + off, len};
            ALLOY_CHECK_EQ(model_crc(s), alloy::ota::crc::crc32_of(s));
        }
    }
}

ALLOY_TEST(crc_hardware_configuration_produces_the_published_check_value) {
    // Every catalogued CRC publishes its value over the nine ASCII digits
    // "123456789". CRC-32/ISO-HDLC's is 0xCBF43926, and this number came from
    // the catalogue, not from running this code — which is what makes it an
    // independent check on both implementations at once.
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const std::span<const std::uint8_t> s{check, sizeof check};
    ALLOY_CHECK_EQ(model_crc(s), 0xCBF43926u);
    ALLOY_CHECK_EQ(alloy::ota::crc::crc32_of(s), 0xCBF43926u);
}

ALLOY_TEST(crc_finalize_is_the_xorout_the_silicon_has_no_register_for) {
    // The driver's LAST STEP, tested directly rather than only through an
    // equivalence that happens to route past it. Until this function existed
    // the xorout was `^ spec::seed` inside st_crc_v3.hpp::value(), a file the
    // host suite cannot include (it needs the generated IP header) — so
    // deleting it left every one of these tests green while the driver computed
    // a different, respectable, useless checksum.
    using alloy::hal::crc_detail::finalize;
    static_assert(finalize(0u) == 0xFFFF'FFFFu, "an untouched remainder is not zero");
    static_assert(finalize(0xFFFF'FFFFu) == 0u);
    static_assert(finalize(finalize(0x1234'5678u)) == 0x1234'5678u, "xorout is an involution");

    // And the load-bearing shape: what the silicon leaves in DR is NOT the
    // answer, and finalize is exactly the distance between the two.
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const std::span<const std::uint8_t> s{check, sizeof check};
    const std::uint32_t in_dr = model_crc(s, spec::reverse_input_bits, spec::reverse_output,
                                          /*apply_xorout=*/false);
    ALLOY_CHECK(in_dr != 0xCBF43926u);
    ALLOY_CHECK_EQ(finalize(in_dr), 0xCBF43926u);
}

ALLOY_TEST(crc_incremental_feeding_matches_one_shot_at_every_chunk_size) {
    // The reason CR.REV_IN is BYTE and not WORD. Under BYTE the driver never
    // has to carry a partial word between update() calls, so feeding the same
    // bytes in ragged chunks must give the same answer as feeding them at once
    // — which is exactly what a bootloader hashing a slot page by page does.
    const blob data;
    for (const std::size_t chunk : {std::size_t{1}, std::size_t{2}, std::size_t{3},
                                    std::size_t{5}, std::size_t{7}, std::size_t{64}}) {
        block_model m;
        m.INIT = spec::seed;
        m.POL = spec::polynomial;
        m.reverse_input_bits = spec::reverse_input_bits;
        m.reverse_output = spec::reverse_output;
        m.reset();
        for (std::size_t at = 0; at < sizeof data.b; at += chunk) {
            const std::size_t n = (at + chunk > sizeof data.b) ? sizeof data.b - at : chunk;
            alloy::hal::crc_detail::feed(model_sink{&m}, data.b + at, n);
        }
        ALLOY_CHECK_EQ(alloy::hal::crc_detail::finalize(m.read_dr()),
                       alloy::ota::crc::crc32_of({data.b, sizeof data.b}));
    }
}

// ── negative controls: the three ways this driver could be silently wrong ──
//
// Without these the tests above are three spellings of "the model agrees with
// itself". Each of these is a one-line change to the driver that leaves it
// compiling, running, and returning a stable 32-bit number.

ALLOY_TEST(crc_without_the_software_xorout_it_is_a_different_checksum) {
    // The parameter the silicon does not have. Dropping `^ seed` from value()
    // yields CRC-32/JAMCRC — respectable, stable, and equal to ISO-HDLC on
    // nothing, so a bootloader would reject every image the updater wrote.
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const std::span<const std::uint8_t> s{check, sizeof check};
    const std::uint32_t no_xor = model_crc(s, spec::reverse_input_bits, spec::reverse_output,
                                           /*apply_xorout=*/false);
    ALLOY_CHECK(no_xor != alloy::ota::crc::crc32_of(s));
    ALLOY_CHECK_EQ(no_xor ^ 0xFFFF'FFFFu, alloy::ota::crc::crc32_of(s));
}

ALLOY_TEST(crc_without_input_reversal_it_is_a_different_checksum) {
    // CR.REV_IN = NONE. This is the mistake with no symptom: the checksum is
    // still a checksum, still stable, still 32 bits.
    const blob data;
    const std::span<const std::uint8_t> s{data.b, 32u};
    ALLOY_CHECK(model_crc(s, /*reverse_input_bits=*/0u) != alloy::ota::crc::crc32_of(s));
}

ALLOY_TEST(crc_without_output_reversal_it_is_a_different_checksum) {
    const blob data;
    const std::span<const std::uint8_t> s{data.b, 32u};
    ALLOY_CHECK(model_crc(s, spec::reverse_input_bits, /*reverse_output=*/false) !=
                alloy::ota::crc::crc32_of(s));
}

ALLOY_TEST(crc_word_reversal_needs_the_other_byte_order_so_it_is_not_a_free_swap) {
    // The alternative configuration the data file describes: REV_IN = WORD is
    // also correct, but only with LITTLE-endian word assembly. Feeding it the
    // big-endian words feed() produces gives a wrong answer — which is why the
    // granularity and the assembly order are one decision and not two.
    const blob data;
    const std::span<const std::uint8_t> s{data.b, 32u};
    ALLOY_CHECK(model_crc(s, /*reverse_input_bits=*/32u) != alloy::ota::crc::crc32_of(s));
}

// ── the facade ──────────────────────────────────────────────────────────

namespace {

// A chip, declared here the way codegen would declare one, so the REAL
// alloy::crc::engine/handle run their real claim and dispatch with only the
// register pokes replaced. Same seam test_encoder.cpp uses.
struct fake_crc_ip {};
struct fake_crc {
    using ip = fake_crc_ip;
};

block_model g_model;

}  // namespace

namespace alloy::hal {
template <>
struct crc_impl<fake_crc> {
    using spec = crc_detail::iso_hdlc;
    static void open() {
        g_model.INIT = spec::seed;
        g_model.POL = spec::polynomial;
        g_model.reverse_input_bits = spec::reverse_input_bits;
        g_model.reverse_output = spec::reverse_output;
        reset();
    }
    static void reset() { g_model.reset(); }
    static void update(const std::uint8_t* p, std::size_t n) {
        crc_detail::feed(model_sink{&g_model}, p, n);
    }
    [[nodiscard]] static std::uint32_t value() { return crc_detail::finalize(g_model.read_dr()); }
};
}  // namespace alloy::hal

static_assert(alloy::crc::Checksum32<alloy::crc::software>,
              "the software CRC must satisfy the shape the facade promises");
static_assert(alloy::crc::Checksum32<alloy::crc::handle<fake_crc>>,
              "a hardware handle and the software class must be interchangeable, "
              "or the fallback in board.hpp is a different API wearing one name");

ALLOY_TEST(crc_facade_opens_once_and_computes_the_same_value_as_software) {
    auto h = alloy::crc::engine<fake_crc>{}.open();
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const std::span<const std::uint8_t> s{check, sizeof check};
    ALLOY_CHECK_EQ(h.checksum(s), alloy::ota::crc::crc32_of(s));
    // checksum() resets first, so a second call over the same bytes must give
    // the same answer rather than continuing the first.
    ALLOY_CHECK_EQ(h.checksum(s), alloy::ota::crc::crc32_of(s));
}

ALLOY_TEST(crc_software_engine_is_a_drop_in_for_a_chip_without_the_block) {
    // What board.hpp hands out when the die has no CRC unit. The point of the
    // stand-in is that it is not a stub: it returns the RIGHT NUMBER, which is
    // the only condition under which a silent substitution is honest.
    auto c = alloy::crc::software_engine{}.open();
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    c.update({check, sizeof check});
    ALLOY_CHECK_EQ(c.value(), 0xCBF43926u);
}

// A generic verifier, written ONCE against the concept and run against both
// implementations. This is the claim crc.hpp's page makes in prose — "same
// methods, same number, no `if constexpr` at the call site" — as a function
// that would not compile if it were false. It was false: the software class had
// no checksum(), and every call site that wrote one built on the boards with
// the block and nowhere else.
namespace {
template <alloy::crc::Checksum32 C>
std::uint32_t verify_the_way_a_program_would(C& c, std::span<const std::uint8_t> b) {
    // Both shapes: the one-shot, then the incremental over the same bytes.
    const std::uint32_t one_shot = c.checksum(b);
    c.reset();
    for (std::size_t at = 0; at < b.size(); at += 3u) {
        const std::size_t n = (at + 3u > b.size()) ? b.size() - at : 3u;
        c.update(b.subspan(at, n));
    }
    return (one_shot == std::as_const(c).value()) ? one_shot : ~one_shot;
}
}  // namespace

ALLOY_TEST(crc_one_generic_verifier_runs_on_the_handle_and_on_the_fallback) {
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    const std::span<const std::uint8_t> s{check, sizeof check};
    // The handle directly, not a second engine::open() — the claim above is
    // EXCLUSIVE and a second open of the same instance traps, which is the
    // behaviour the test above bought. Configure the model the way open() does
    // and take the (zero-size, claim-free) handle.
    alloy::hal::crc_impl<fake_crc>::open();
    alloy::crc::handle<fake_crc> hw{};
    ALLOY_CHECK_EQ(verify_the_way_a_program_would(hw, s), 0xCBF43926u);
    auto sw = alloy::crc::software_engine{}.open();
    ALLOY_CHECK_EQ(verify_the_way_a_program_would(sw, s), 0xCBF43926u);
}

// The table variant exists so a caller holding an interrupt mask across a
// checksum (libs/bus's frame encode) does not spend eight shifts per byte
// there. It is only safe to swap in if it is the SAME function: pinned here
// over every byte value, every length up to a frame, and the catalogue check
// value — so a table folded from a mistyped polynomial fails the build's
// tests rather than the field's links.
ALLOY_TEST(crc32_table_is_the_same_function_as_the_bytewise_one) {
    static constexpr std::uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    ALLOY_CHECK_EQ(alloy::ota::crc::crc32_table_of({check, sizeof check}),
                   0xCBF4'3926u);  // the CRC-32/ISO-HDLC catalogue value
    ALLOY_CHECK_EQ(alloy::ota::crc::crc32_table_of({check, sizeof check}),
                   alloy::ota::crc::crc32_of({check, sizeof check}));

    // Every byte value, at every length a bus frame can reach.
    std::uint8_t buf[160];
    for (std::size_t i = 0; i < sizeof buf; ++i) {
        buf[i] = static_cast<std::uint8_t>(i * 7u + 13u);
    }
    for (std::size_t n = 0; n <= sizeof buf; ++n) {
        const std::span<const std::uint8_t> s{buf, n};
        if (alloy::ota::crc::crc32_table_of(s) != alloy::ota::crc::crc32_of(s)) {
            ALLOY_CHECK(false);
            return;
        }
    }
    ALLOY_CHECK(true);

    // And incrementally, split at an awkward boundary — the receiver feeds
    // one byte at a time, so the streaming path must agree too.
    alloy::ota::crc::crc32_table inc;
    inc.reset();
    for (const std::uint8_t b : buf) {
        inc.update(&b, 1);
    }
    ALLOY_CHECK_EQ(inc.value(), alloy::ota::crc::crc32_of({buf, sizeof buf}));
}
