// Host tests for the per-device factory identity record
// (alloy/provision/identity.hpp).
//
// The claim worth testing is CROSS-LANGUAGE AGREEMENT: the record is written by
// a Python host verb (`alloy provision write`) and parsed by C++ on the device,
// and the two never run in the same process, so nothing catches a drift between
// them except a shared golden. kGolden below is 64 bytes produced by the Python
// encoder; tools/alloy/tests/test_provision.py pins the SAME literal from the
// other side. Change either encoder and exactly one suite goes red, which is the
// point — a silent disagreement would put a serial number nobody can read into a
// production batch.
//
// The rest is refusals: erased flash, a wrong magic, a repaired-looking record
// whose CRC does not cover the change, and a serial that fills all 16 bytes with
// no room for a terminator.
#include <cstdint>
#include <cstring>
#include <span>

#include "alloy/provision.hpp"
#include "alloy_test.hpp"

namespace {

using namespace alloy::provision;

// Produced by: alloy provision write --serial ALY-0001-A7
//              --mac 02:1a:2b:3c:4d:5e --hw-rev 3 --batch 42 -o id.bin
constexpr std::uint8_t kGolden[64] = {
    0x41, 0x50, 0x52, 0x56, 0x01, 0x00, 0x40, 0x00,  // "APRV", version 1, size 64
    0x41, 0x4c, 0x59, 0x2d, 0x30, 0x30, 0x30, 0x31,  // "ALY-0001"
    0x2d, 0x41, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00,  // "-A7" + NUL padding
    0x02, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x03, 0x00,  // mac, hw_rev = 3
    0x2a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // batch = 42, reserved…
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x86, 0x13, 0xdf,  // crc32
};

identity sample() {
    identity id;
    std::memcpy(id.serial, "ALY-0001-A7", 11);
    const std::uint8_t mac[6] = {0x02, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e};
    std::memcpy(id.mac, mac, sizeof(mac));
    id.hw_revision = 3;
    id.batch = 42;
    return id;
}

}  // namespace

// ── cross-language agreement ────────────────────────────────────────────────

ALLOY_TEST(provision_parses_the_record_the_host_verb_writes) {
    const auto id = identity::parse({kGolden, sizeof(kGolden)});
    ALLOY_CHECK(static_cast<bool>(id));
    ALLOY_CHECK(id->serial_view() == std::string_view{"ALY-0001-A7"});
    ALLOY_CHECK_EQ(id->mac[0], 0x02u);
    ALLOY_CHECK_EQ(id->mac[5], 0x5eu);
    ALLOY_CHECK_EQ(id->hw_revision, 3u);
    ALLOY_CHECK_EQ(id->batch, 42u);
}

ALLOY_TEST(provision_serializes_byte_for_byte_like_the_host_verb) {
    // The device's own encoder must produce exactly what Python produced —
    // including the CRC, which is the field most likely to drift silently.
    std::uint8_t out[identity::byte_size]{};
    sample().serialize(out);
    ALLOY_CHECK_EQ(std::memcmp(out, kGolden, sizeof(kGolden)), 0);
}

ALLOY_TEST(provision_round_trips_through_its_own_encoder) {
    std::uint8_t out[identity::byte_size]{};
    sample().serialize(out);
    const auto back = identity::parse({out, sizeof(out)});
    ALLOY_CHECK(static_cast<bool>(back));
    ALLOY_CHECK(back->serial_view() == std::string_view{"ALY-0001-A7"});
    ALLOY_CHECK_EQ(back->batch, 42u);
}

// ── the states a production line actually hits ──────────────────────────────

ALLOY_TEST(provision_erased_flash_reads_as_blank_not_as_corrupt) {
    // A board that was never provisioned and a board whose record went bad are
    // different faults — one is a missed line step, the other is a programmer
    // or a flash problem. The firmware must not conflate them.
    std::uint8_t erased[identity::byte_size];
    std::memset(erased, 0xFF, sizeof(erased));
    const auto id = identity::parse({erased, sizeof(erased)});
    ALLOY_CHECK(!id);
    ALLOY_CHECK(id.error() == prov_error::blank);
}

ALLOY_TEST(provision_foreign_bytes_are_bad_magic_not_blank) {
    std::uint8_t junk[identity::byte_size]{};  // all zeros: not 0xFF, not "APRV"
    const auto id = identity::parse({junk, sizeof(junk)});
    ALLOY_CHECK(!id);
    ALLOY_CHECK(id.error() == prov_error::bad_magic);
}

ALLOY_TEST(provision_a_flipped_serial_byte_fails_the_crc) {
    // A torn write or a partially erased page changes bytes without changing
    // the CRC field. If the CRC did not cover the serial, this would parse and
    // the device would report a serial number that is not its own.
    std::uint8_t raw[identity::byte_size];
    std::memcpy(raw, kGolden, sizeof(raw));
    raw[8] ^= 0x01;  // 'A' -> '@' in the serial
    const auto id = identity::parse({raw, sizeof(raw)});
    ALLOY_CHECK(!id);
    ALLOY_CHECK(id.error() == prov_error::bad_crc);
}

ALLOY_TEST(provision_reserved_bytes_are_inside_the_crc) {
    // Byte 40 is in the reserved block. It is covered on purpose: reserved
    // bytes a v2 will use must not be a place an attacker or a bad programmer
    // can scribble without the record noticing.
    std::uint8_t raw[identity::byte_size];
    std::memcpy(raw, kGolden, sizeof(raw));
    raw[40] ^= 0xFF;
    const auto id = identity::parse({raw, sizeof(raw)});
    ALLOY_CHECK(!id);
    ALLOY_CHECK(id.error() == prov_error::bad_crc);
}

ALLOY_TEST(provision_a_newer_format_version_is_refused_not_guessed) {
    identity id = sample();
    std::uint8_t raw[identity::byte_size]{};
    id.serialize(raw);
    raw[4] = 2;  // format_version = 2
    // Repair the CRC so ONLY the version can reject it — otherwise this test
    // would pass for the wrong reason.
    const std::uint32_t crc = alloy::ota::crc::crc32_of({raw, 60u});
    raw[60] = static_cast<std::uint8_t>(crc & 0xFF);
    raw[61] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
    raw[62] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
    raw[63] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
    const auto parsed = identity::parse({raw, sizeof(raw)});
    ALLOY_CHECK(!parsed);
    ALLOY_CHECK(parsed.error() == prov_error::unsupported_version);
}

ALLOY_TEST(provision_a_short_read_is_truncated_not_a_parse) {
    const auto id = identity::parse({kGolden, 32u});
    ALLOY_CHECK(!id);
    ALLOY_CHECK(id.error() == prov_error::truncated);
}

// ── the 16-byte serial edge ────────────────────────────────────────────────

ALLOY_TEST(provision_a_full_16_byte_serial_has_no_terminator) {
    // This is why serial_view() exists rather than a const char*. A serial that
    // fills the field is legal, and reading it as a C string would run off the
    // end into the MAC.
    identity id;
    std::memcpy(id.serial, "0123456789ABCDEF", 16);
    const std::uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    std::memcpy(id.mac, mac, sizeof(mac));
    std::uint8_t raw[identity::byte_size]{};
    id.serialize(raw);
    const auto back = identity::parse({raw, sizeof(raw)});
    ALLOY_CHECK(static_cast<bool>(back));
    ALLOY_CHECK_EQ(back->serial_view().size(), 16u);
    ALLOY_CHECK(back->serial_view() == std::string_view{"0123456789ABCDEF"});
    ALLOY_CHECK_EQ(back->mac[0], 0xAAu);  // the MAC is still the MAC
}

ALLOY_TEST(provision_read_parses_a_memory_mapped_page) {
    // alloy::provision::read() is what firmware calls; point it at a buffer
    // standing in for memory-mapped flash at provision_base.
    alignas(std::uint64_t) static std::uint8_t page[128];
    std::memset(page, 0xFF, sizeof(page));
    std::memcpy(page, kGolden, sizeof(kGolden));
    const auto id = read(reinterpret_cast<std::uintptr_t>(page));
    ALLOY_CHECK(static_cast<bool>(id));
    ALLOY_CHECK(id->serial_view() == std::string_view{"ALY-0001-A7"});
}
