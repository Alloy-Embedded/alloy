// Variable registry and word-order codec tests.
// Covers all four word orders for every multi-register type; single-register
// types; bool encoding; and registry address lookup.

#include <array>
#include <cstdint>
#include <limits>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/var.hpp"

using namespace alloy::modbus;

// ============================================================================
// Helpers
// ============================================================================

namespace {

template <typename T, std::size_t N>
constexpr T rt(T value, WordOrder order) {
    const auto words = encode_words<T>(value, order);
    return decode_words<T>(std::span<const std::uint16_t, N>{words.data(), N}, order);
}

}  // namespace

// ============================================================================
// Single-register types (bool, int16_t, uint16_t)
// ============================================================================

TEST_CASE("encode_words<bool>: true → 0x0001, false → 0x0000") {
    CHECK(encode_words<bool>(true)[0]  == 0x0001u);
    CHECK(encode_words<bool>(false)[0] == 0x0000u);
}

TEST_CASE("decode_words<bool>: zero → false, non-zero → true") {
    const std::array<std::uint16_t, 1> t{0x0001u};
    const std::array<std::uint16_t, 1> f{0x0000u};
    const std::array<std::uint16_t, 1> nz{0xFFFFu};
    CHECK(decode_words<bool>(t, WordOrder::ABCD)  == true);
    CHECK(decode_words<bool>(f, WordOrder::ABCD)  == false);
    CHECK(decode_words<bool>(nz, WordOrder::ABCD) == true);
}

TEST_CASE("encode/decode round-trip: int16_t") {
    CHECK(rt<std::int16_t, 1>(-1,      WordOrder::ABCD) == -1);
    CHECK(rt<std::int16_t, 1>(-32768,  WordOrder::ABCD) == -32768);
    CHECK(rt<std::int16_t, 1>(32767,   WordOrder::ABCD) == 32767);
    CHECK(rt<std::int16_t, 1>(0,       WordOrder::ABCD) == 0);
}

TEST_CASE("encode/decode round-trip: uint16_t") {
    CHECK(rt<std::uint16_t, 1>(0u,      WordOrder::ABCD) == 0u);
    CHECK(rt<std::uint16_t, 1>(0xFFFFu, WordOrder::ABCD) == 0xFFFFu);
    CHECK(rt<std::uint16_t, 1>(0xABCDu, WordOrder::ABCD) == 0xABCDu);
}

// ============================================================================
// Two-register types (int32_t, uint32_t, float) — all four word orders
// ============================================================================

TEST_CASE("encode_words<uint32_t>: ABCD places MSW in reg[0]") {
    // 0xAABBCCDD → reg[0]=0xAABB, reg[1]=0xCCDD
    const auto w = encode_words<std::uint32_t>(0xAABBCCDDu, WordOrder::ABCD);
    CHECK(w[0] == 0xAABBu);
    CHECK(w[1] == 0xCCDDu);
}

TEST_CASE("encode_words<uint32_t>: CDAB places LSW in reg[0]") {
    const auto w = encode_words<std::uint32_t>(0xAABBCCDDu, WordOrder::CDAB);
    CHECK(w[0] == 0xCCDDu);
    CHECK(w[1] == 0xAABBu);
}

TEST_CASE("encode_words<uint32_t>: BADC byte-swaps each word (MSW first)") {
    // hi=0xAABB → byte_swap → 0xBBAA; lo=0xCCDD → 0xDDCC
    const auto w = encode_words<std::uint32_t>(0xAABBCCDDu, WordOrder::BADC);
    CHECK(w[0] == 0xBBAAu);
    CHECK(w[1] == 0xDDCCu);
}

TEST_CASE("encode_words<uint32_t>: DCBA fully little-endian") {
    // lo=0xCCDD byte_swap → 0xDDCC first; hi=0xAABB byte_swap → 0xBBAA second
    const auto w = encode_words<std::uint32_t>(0xAABBCCDDu, WordOrder::DCBA);
    CHECK(w[0] == 0xDDCCu);
    CHECK(w[1] == 0xBBAAu);
}

TEST_CASE("round-trip uint32_t: all four word orders") {
    constexpr std::uint32_t val = 0xDEADBEEFu;
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<std::uint32_t, 2>(val, order) == val);
    }
}

TEST_CASE("round-trip int32_t: all four word orders") {
    constexpr std::int32_t val = -123456789;
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<std::int32_t, 2>(val, order) == val);
    }
}

TEST_CASE("round-trip float: all four word orders") {
    constexpr float val = 3.14159265f;
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<float, 2>(val, order) == val);
    }
}

TEST_CASE("round-trip float: edge cases") {
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<float, 2>(0.0f,                             order) == 0.0f);
        CHECK(rt<float, 2>(-1.0f,                            order) == -1.0f);
        CHECK(rt<float, 2>(std::numeric_limits<float>::max(), order) ==
              std::numeric_limits<float>::max());
    }
}

// ============================================================================
// Four-register type (double) — all four word orders
// ============================================================================

TEST_CASE("round-trip double: all four word orders") {
    constexpr double val = 2.718281828459045;
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<double, 4>(val, order) == val);
    }
}

TEST_CASE("round-trip double: edge cases") {
    for (const auto order : {WordOrder::ABCD, WordOrder::CDAB,
                              WordOrder::BADC, WordOrder::DCBA}) {
        CHECK(rt<double, 4>(0.0,                              order) == 0.0);
        CHECK(rt<double, 4>(-1.0,                             order) == -1.0);
        CHECK(rt<double, 4>(std::numeric_limits<double>::max(), order) ==
              std::numeric_limits<double>::max());
    }
}

TEST_CASE("encode_words<double>: ABCD places highest word in reg[0]") {
    // 1.0 IEEE 754: 0x3FF0_0000_0000_0000
    //   → a=0x3FF0, b=0x0000, c=0x0000, d=0x0000
    const auto w = encode_words<double>(1.0, WordOrder::ABCD);
    CHECK(w[0] == 0x3FF0u);
    CHECK(w[1] == 0x0000u);
    CHECK(w[2] == 0x0000u);
    CHECK(w[3] == 0x0000u);
}

TEST_CASE("encode_words<double>: CDAB swaps word pairs") {
    // 1.0 → CDAB: reg[0]=c=0x0000, reg[1]=d=0x0000, reg[2]=a=0x3FF0, reg[3]=b=0x0000
    const auto w = encode_words<double>(1.0, WordOrder::CDAB);
    CHECK(w[0] == 0x0000u);
    CHECK(w[1] == 0x0000u);
    CHECK(w[2] == 0x3FF0u);
    CHECK(w[3] == 0x0000u);
}

// ============================================================================
// Var<T> descriptor and encode/decode helpers
// ============================================================================

TEST_CASE("Var<uint16_t>: encode/decode round-trip") {
    constexpr Var<std::uint16_t> v{
        .address    = 100u,
        .access     = Access::ReadWrite,
        .name       = "sensor",
        .word_order = WordOrder::ABCD,
    };
    CHECK(v.kRegCount == 1u);
    const auto w = v.encode(0x1234u);
    CHECK(v.decode(w) == 0x1234u);
}

TEST_CASE("Var<float>: encode/decode round-trip CDAB") {
    constexpr Var<float> v{
        .address    = 200u,
        .access     = Access::ReadOnly,
        .name       = "temperature",
        .word_order = WordOrder::CDAB,
    };
    CHECK(v.kRegCount == 2u);
    const auto w = v.encode(25.5f);
    CHECK(v.decode(w) == 25.5f);
}

// ============================================================================
// Registry: make_registry + address lookup
// ============================================================================

TEST_CASE("Registry: make_registry builds correct descriptors") {
    constexpr Var<std::uint16_t> v0{.address = 0u,  .access = Access::ReadOnly,  .name = "a"};
    constexpr Var<std::int32_t>  v1{.address = 2u,  .access = Access::ReadWrite, .name = "b"};
    constexpr Var<float>         v2{.address = 10u, .access = Access::ReadOnly,  .name = "c"};

    constexpr auto reg = make_registry(v0, v1, v2);
    CHECK(reg.size() == 3u);
    CHECK(reg[0].address   == 0u);
    CHECK(reg[0].reg_count == 1u);
    CHECK(reg[1].address   == 2u);
    CHECK(reg[1].reg_count == 2u);
    CHECK(reg[2].address   == 10u);
    CHECK(reg[2].reg_count == 2u);
}

TEST_CASE("Registry::find: exact address match") {
    constexpr auto reg = make_registry(
        Var<std::uint16_t>{.address = 5u,  .access = Access::ReadWrite, .name = "x"},
        Var<float>        {.address = 10u, .access = Access::ReadOnly,  .name = "y"});

    const auto* d = reg.find(5u);
    REQUIRE(d != nullptr);
    CHECK(d->address == 5u);
    CHECK(d->name    == "x");
}

TEST_CASE("Registry::find: second register of a multi-register var") {
    constexpr auto reg = make_registry(
        Var<float>{.address = 20u, .access = Access::ReadOnly, .name = "temp"});

    // float occupies registers 20 and 21
    CHECK(reg.find(20u) != nullptr);
    CHECK(reg.find(21u) != nullptr);
    CHECK(reg.find(22u) == nullptr);
}

TEST_CASE("Registry::find: address not in any var returns nullptr") {
    constexpr auto reg = make_registry(
        Var<std::uint16_t>{.address = 0u, .access = Access::ReadOnly, .name = "v"});

    CHECK(reg.find(1u) == nullptr);
    CHECK(reg.find(100u) == nullptr);
}

TEST_CASE("Registry: iteration covers all descriptors") {
    constexpr auto reg = make_registry(
        Var<std::uint16_t>{.address = 1u,  .access = Access::ReadOnly,  .name = "a"},
        Var<std::uint16_t>{.address = 2u,  .access = Access::WriteOnly, .name = "b"},
        Var<std::uint16_t>{.address = 3u,  .access = Access::ReadWrite, .name = "c"});

    std::size_t count = 0u;
    for (const auto& d : reg) {
        (void)d;
        ++count;
    }
    CHECK(count == 3u);
}

TEST_CASE("var_reg_count: returns correct counts for all types") {
    static_assert(var_reg_count<bool>()           == 1u);
    static_assert(var_reg_count<std::int16_t>()   == 1u);
    static_assert(var_reg_count<std::uint16_t>()  == 1u);
    static_assert(var_reg_count<std::int32_t>()   == 2u);
    static_assert(var_reg_count<std::uint32_t>()  == 2u);
    static_assert(var_reg_count<float>()          == 2u);
    static_assert(var_reg_count<double>()         == 4u);
}
