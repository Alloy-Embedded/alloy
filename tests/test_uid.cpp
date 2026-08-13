// The factory device identifier's portable half (src/alloy/uid.hpp).
//
// The READ is three loads from an address the chip database supplies and
// nothing on a host can check — no STM32 is on hand, and Renode's stm32g0
// platform does not map the system-information region the UID lives in at all,
// so even an emulated read would fault rather than answer. What IS checkable
// here is everything above the load: the text form, the fold, and the
// zero-word identifier a chip without the block hands back — which is the piece
// that has to keep `board::uid.read()` compiling on the eight-board matrix.

#include <alloy/uid.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy_test.hpp"

namespace {

// A chip, declared here the way codegen declares one. The base points at a host
// struct, which is enough because every register is read-only and there is
// nothing to gate, claim or configure — the same reason alloy::uid::reader
// makes no claim.
struct fake_uid_ip {};
struct fake_uid {
    using ip = fake_uid_ip;
    struct feat {
        static constexpr std::uint32_t id_words = 3u;
    };
};

std::uint32_t g_words[3] = {0x0033'002Eu, 0x3138'5105u, 0x2039'3443u};

bool text_is(std::span<const char> got, const char* want) {
    std::size_t i = 0;
    for (; i < got.size(); ++i) {
        if (want[i] == '\0' || got[i] != want[i]) {
            return false;
        }
    }
    return want[i] == '\0';
}

}  // namespace

namespace alloy::hal {
template <>
struct uid_impl<fake_uid> {
    static std::array<std::uint32_t, 3> read() { return {g_words[0], g_words[1], g_words[2]}; }
};
}  // namespace alloy::hal

ALLOY_TEST(uid_reader_returns_the_words_low_first) {
    const auto id = alloy::uid::reader<fake_uid>{}.read();
    ALLOY_CHECK_EQ(decltype(id)::words, 3u);
    ALLOY_CHECK_EQ(decltype(id)::bits, 96u);
    ALLOY_CHECK_EQ(id.word[0], g_words[0]);
    ALLOY_CHECK_EQ(id.word[2], g_words[2]);
}

ALLOY_TEST(uid_text_reads_high_word_first_like_a_number) {
    // Storage order is low word first (the manual's numbering); TEXT order is
    // high word first, because that is how a human reads a 96-bit number. The
    // two disagreeing on purpose is exactly the kind of thing that needs a test
    // rather than a comment.
    const auto id = alloy::uid::reader<fake_uid>{}.read();
    decltype(id)::hex_buffer buf;
    static_assert(decltype(id)::hex_buffer{}.size() == decltype(id)::hex_chars);
    const std::size_t n = id.to_hex(buf);
    ALLOY_CHECK_EQ(n, decltype(id)::hex_chars);
    ALLOY_CHECK(text_is({buf.data(), n}, "20393443-31385105-0033002E"));
}

ALLOY_TEST(uid_to_hex_refuses_a_short_buffer_rather_than_truncating) {
    const auto id = alloy::uid::reader<fake_uid>{}.read();
    char small[decltype(id)::hex_chars - 1] = {};
    ALLOY_CHECK_EQ(id.to_hex(small), std::size_t{0});
    // Nothing written: a partial identifier printed as if it were whole is the
    // failure mode worth refusing, because it still looks like an ID.
    ALLOY_CHECK_EQ(small[0], '\0');
}

ALLOY_TEST(uid_fold32_mixes_every_word) {
    const auto id = alloy::uid::reader<fake_uid>{}.read();
    ALLOY_CHECK_EQ(id.fold32(), g_words[0] ^ g_words[1] ^ g_words[2]);
}

ALLOY_TEST(uid_equality_compares_the_whole_identifier) {
    const auto a = alloy::uid::reader<fake_uid>{}.read();
    auto b = a;
    ALLOY_CHECK(a == b);
    b.word[1] ^= 1u;
    ALLOY_CHECK(!(a == b));
}

ALLOY_TEST(uid_absent_is_zero_words_and_still_compiles_end_to_end) {
    // What board.hpp hands out on a chip with no UID block. Every operation a
    // program writes against a real identifier has to work here too, or the
    // eight-board matrix needs a #if — which is the thing this framework is
    // for. Zero words, not a fabricated value: the degree rule ("0 means
    // absent") applied to a whole quantity rather than to a count.
    const auto id = alloy::uid::null_reader{}.read();
    ALLOY_CHECK_EQ(decltype(id)::words, 0u);
    ALLOY_CHECK_EQ(decltype(id)::bits, 0u);
    ALLOY_CHECK_EQ(decltype(id)::hex_chars, std::size_t{0});
    ALLOY_CHECK_EQ(id.fold32(), 0u);
    char none[1] = {'x'};
    ALLOY_CHECK_EQ(id.to_hex({none, 0u}), std::size_t{0});
    ALLOY_CHECK(id == alloy::uid::null_reader{}.read());

    // THE LINE THAT DID NOT COMPILE. The test above passed a hand-made empty
    // span, which is not what a program writes — a program sizes its buffer
    // from the identifier, and `char text[hex_chars]` is `char text[0]` here,
    // which is not a type and does not convert to a span. examples/device_id
    // wrote exactly that and built on one board of nine. `hex_buffer` is the
    // declaration that survives zero, and this is it being declared.
    decltype(id)::hex_buffer text;
    static_assert(decltype(id)::hex_buffer{}.size() == 0u);
    ALLOY_CHECK_EQ(id.to_hex(text), std::size_t{0});
}
