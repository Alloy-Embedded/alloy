#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/discovery.hpp"
#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/rtu_frame.hpp"
#include "alloy/modbus/slave.hpp"
#include "alloy/modbus/transport/loopback_stream.hpp"
#include "alloy/modbus/var.hpp"

using namespace alloy::modbus;

// ============================================================================
// Test fixtures
// ============================================================================

static constexpr Var<std::uint16_t> kVarTemp{
    .address = 100u, .access = Access::ReadOnly, .name = "temp"};
static constexpr Var<float> kVarPressure{
    .address = 102u, .access = Access::ReadWrite, .name = "pressure"};
static constexpr Var<std::int32_t> kVarRaw{
    .address = 104u, .access = Access::WriteOnly, .name = "raw"};

static constexpr VarMeta kPressureMeta{
    .unit = "hPa", .desc = "barometric pressure",
    .range_min = 800.0f, .range_max = 1100.0f};

static std::uint16_t  g_temp{250u};
static float          g_pressure{1013.25f};
static std::int32_t   g_raw{0};

static auto make_registry() {
    return Registry<3u>{std::array<VarDescriptor, 3u>{
        bind(kVarTemp,     g_temp),
        bind(kVarPressure, g_pressure, &kPressureMeta),
        bind(kVarRaw,      g_raw),
    }};
}

// Send a raw discovery request PDU through a LoopbackPair and collect the slave
// response PDU (unwrapped from RTU). Returns the raw PDU bytes.
static std::vector<std::byte> probe(std::uint8_t sub_fn) {
    auto reg = make_registry();
    LoopbackPair<512u> pair{};
    auto master_ep = pair.a();
    auto slave_ep  = pair.b();

    Slave slave{slave_ep, 0x01u, reg};

    // Encode request: [0x65][sub_fn] wrapped in RTU
    std::array<std::byte, 2u> req_pdu{
        std::byte{kDiscoveryFcDefault}, std::byte{sub_fn}};
    std::array<std::byte, 256u> tx{};
    const auto frm = encode_rtu_frame(
        tx, 0x01u, std::span<const std::byte>{req_pdu.data(), 2u});
    REQUIRE(frm.is_ok());
    master_ep.write(std::span<const std::byte>{tx.data(), frm.unwrap()});

    // Slave processes it
    const auto poll_res = slave.poll(0u);
    REQUIRE(poll_res.is_ok());
    REQUIRE(poll_res.unwrap() == true);

    // Read RTU response from master's endpoint
    std::array<std::byte, 512u> rx{};
    const auto n = master_ep.read(rx, 0u);
    REQUIRE(n.is_ok());
    REQUIRE(n.unwrap() > 0u);

    const auto decoded = decode_rtu_frame(
        std::span<const std::byte>{rx.data(), n.unwrap()});
    REQUIRE(decoded.is_ok());

    const auto& frame = decoded.unwrap();
    return std::vector<std::byte>{frame.pdu.begin(), frame.pdu.end()};
}

// ============================================================================
// Encode/decode round-trip tests (no slave)
// ============================================================================

TEST_CASE("discovery: encode_thin header fields") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto res = encode_discovery_thin(buf, kDiscoveryFcDefault, reg);
    REQUIRE(res.is_ok());
    const std::size_t sz = res.unwrap();
    REQUIRE(sz >= 4u);
    // [FC=0x65][sub=0x01][count_hi=0x00][count_lo=0x03]
    CHECK(buf[0] == std::byte{0x65u});
    CHECK(buf[1] == std::byte{0x01u});
    CHECK(buf[2] == std::byte{0x00u});
    CHECK(buf[3] == std::byte{0x03u});
}

TEST_CASE("discovery: encode_rich header fields") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto res = encode_discovery_rich(buf, kDiscoveryFcDefault, reg);
    REQUIRE(res.is_ok());
    CHECK(buf[0] == std::byte{0x65u});
    CHECK(buf[1] == std::byte{0x02u});
    CHECK(buf[2] == std::byte{0x00u});
    CHECK(buf[3] == std::byte{0x03u});
}

TEST_CASE("discovery: thin round-trip — all three vars") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto enc = encode_discovery_thin(buf, kDiscoveryFcDefault, reg);
    REQUIRE(enc.is_ok());

    std::vector<DiscoveryEntry> entries;
    const auto dec = decode_discovery_response(
        std::span<const std::byte>{buf.data(), enc.unwrap()},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    REQUIRE(dec.is_ok());
    REQUIRE(dec.unwrap() == 3u);
    REQUIRE(entries.size() == 3u);

    CHECK(entries[0].address   == 100u);
    CHECK(entries[0].reg_count == 1u);
    CHECK(entries[0].type_tag  == VarType::Uint16);
    CHECK(entries[0].access    == Access::ReadOnly);
    CHECK(entries[0].name      == "temp");

    CHECK(entries[1].address   == 102u);
    CHECK(entries[1].reg_count == 2u);
    CHECK(entries[1].type_tag  == VarType::Float);
    CHECK(entries[1].access    == Access::ReadWrite);
    CHECK(entries[1].name      == "pressure");

    CHECK(entries[2].address   == 104u);
    CHECK(entries[2].reg_count == 2u);
    CHECK(entries[2].type_tag  == VarType::Int32);
    CHECK(entries[2].access    == Access::WriteOnly);
    CHECK(entries[2].name      == "raw");
}

TEST_CASE("discovery: thin round-trip — rich fields are zero/empty") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto enc = encode_discovery_thin(buf, kDiscoveryFcDefault, reg);
    REQUIRE(enc.is_ok());

    std::vector<DiscoveryEntry> entries;
    (void)decode_discovery_response(
        std::span<const std::byte>{buf.data(), enc.unwrap()},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    REQUIRE(entries.size() == 3u);
    for (const auto& e : entries) {
        CHECK(e.unit.empty());
        CHECK(e.desc.empty());
        CHECK(e.range_min == 0.0f);
        CHECK(e.range_max == 0.0f);
    }
}

TEST_CASE("discovery: rich round-trip — pressure meta present") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto enc = encode_discovery_rich(buf, kDiscoveryFcDefault, reg);
    REQUIRE(enc.is_ok());

    std::vector<DiscoveryEntry> entries;
    const auto dec = decode_discovery_response(
        std::span<const std::byte>{buf.data(), enc.unwrap()},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    REQUIRE(dec.is_ok());
    REQUIRE(entries.size() == 3u);

    // temp: no meta → empty strings, zero range
    CHECK(entries[0].unit.empty());
    CHECK(entries[0].desc.empty());
    CHECK(entries[0].range_min == 0.0f);
    CHECK(entries[0].range_max == 0.0f);

    // pressure: has VarMeta
    CHECK(entries[1].unit      == "hPa");
    CHECK(entries[1].desc      == "barometric pressure");
    CHECK(entries[1].range_min == 800.0f);
    CHECK(entries[1].range_max == 1100.0f);

    // raw: no meta
    CHECK(entries[2].unit.empty());
    CHECK(entries[2].desc.empty());
}

TEST_CASE("discovery: buffer too small returns BufferTooSmall") {
    auto reg = make_registry();
    std::array<std::byte, 2u> tiny{};
    const auto res = encode_discovery_thin(tiny, kDiscoveryFcDefault, reg);
    CHECK(res.is_err());
}

TEST_CASE("discovery: truncated PDU returns Truncated") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto enc = encode_discovery_thin(buf, kDiscoveryFcDefault, reg);
    REQUIRE(enc.is_ok());

    // Feed only the header (4 bytes) — entries are missing
    std::vector<DiscoveryEntry> entries;
    const auto dec = decode_discovery_response(
        std::span<const std::byte>{buf.data(), 4u},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    CHECK(dec.is_err());
}

TEST_CASE("discovery: alternate FC override") {
    auto reg = make_registry();
    std::array<std::byte, 512u> buf{};
    const auto enc = encode_discovery_thin(buf, 0x66u, reg);
    REQUIRE(enc.is_ok());
    CHECK(buf[0] == std::byte{0x66u});
    CHECK(buf[1] == std::byte{0x01u});
}

// ============================================================================
// Slave integration: loopback discovery probe
// ============================================================================

TEST_CASE("discovery: slave responds to thin probe over loopback") {
    const auto pdu = probe(kDiscoverySubThin);
    REQUIRE(pdu.size() >= 4u);
    CHECK(pdu[0] == std::byte{kDiscoveryFcDefault});
    CHECK(pdu[1] == std::byte{kDiscoverySubThin});
    CHECK(pdu[2] == std::byte{0x00u});
    CHECK(pdu[3] == std::byte{0x03u});

    std::vector<DiscoveryEntry> entries;
    const auto dec = decode_discovery_response(
        std::span<const std::byte>{pdu.data(), pdu.size()},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    REQUIRE(dec.is_ok());
    REQUIRE(entries.size() == 3u);

    CHECK(entries[0].name == "temp");
    CHECK(entries[1].name == "pressure");
    CHECK(entries[2].name == "raw");
}

TEST_CASE("discovery: slave responds to rich probe over loopback") {
    const auto pdu = probe(kDiscoverySubRich);
    REQUIRE(pdu.size() >= 4u);
    CHECK(pdu[1] == std::byte{kDiscoverySubRich});

    std::vector<DiscoveryEntry> entries;
    const auto dec = decode_discovery_response(
        std::span<const std::byte>{pdu.data(), pdu.size()},
        [&](const DiscoveryEntry& e) { entries.push_back(e); });
    REQUIRE(dec.is_ok());
    REQUIRE(entries.size() == 3u);

    CHECK(entries[1].unit      == "hPa");
    CHECK(entries[1].range_min == 800.0f);
    CHECK(entries[1].range_max == 1100.0f);
}

TEST_CASE("discovery: slave with overridden FC 0x66 responds correctly") {
    auto reg = make_registry();
    LoopbackPair<512u> pair{};
    auto master_ep = pair.a();
    auto slave_ep  = pair.b();

    Slave slave{slave_ep, 0x01u, reg, 0x66u};

    std::array<std::byte, 2u> req_pdu{std::byte{0x66u}, std::byte{0x01u}};
    std::array<std::byte, 256u> tx{};
    const auto frm = encode_rtu_frame(
        tx, 0x01u, std::span<const std::byte>{req_pdu.data(), 2u});
    REQUIRE(frm.is_ok());
    master_ep.write(std::span<const std::byte>{tx.data(), frm.unwrap()});

    const auto poll_res = slave.poll(0u);
    REQUIRE(poll_res.is_ok());
    REQUIRE(poll_res.unwrap() == true);

    std::array<std::byte, 512u> rx{};
    const auto n = master_ep.read(rx, 0u);
    REQUIRE(n.is_ok());
    const auto decoded = decode_rtu_frame(
        std::span<const std::byte>{rx.data(), n.unwrap()});
    REQUIRE(decoded.is_ok());
    CHECK(decoded.unwrap().pdu[0] == std::byte{0x66u});
    CHECK(decoded.unwrap().pdu[1] == std::byte{0x01u});
}

TEST_CASE("discovery: default FC 0x65 ignored when slave uses 0x66") {
    auto reg = make_registry();
    LoopbackPair<512u> pair{};
    auto master_ep = pair.a();
    auto slave_ep  = pair.b();

    Slave slave{slave_ep, 0x01u, reg, 0x66u};

    // Send 0x65 — should reply with IllegalFunction exception
    std::array<std::byte, 2u> req_pdu{std::byte{0x65u}, std::byte{0x01u}};
    std::array<std::byte, 256u> tx{};
    const auto frm = encode_rtu_frame(
        tx, 0x01u, std::span<const std::byte>{req_pdu.data(), 2u});
    REQUIRE(frm.is_ok());
    master_ep.write(std::span<const std::byte>{tx.data(), frm.unwrap()});

    const auto poll_res = slave.poll(0u);
    REQUIRE(poll_res.is_ok());
    REQUIRE(poll_res.unwrap() == true);

    std::array<std::byte, 512u> rx{};
    const auto n = master_ep.read(rx, 0u);
    REQUIRE(n.is_ok());
    const auto decoded = decode_rtu_frame(
        std::span<const std::byte>{rx.data(), n.unwrap()});
    REQUIRE(decoded.is_ok());
    // Exception response: FC | 0x80 = 0xE5
    CHECK(decoded.unwrap().pdu[0] == std::byte{0xE5u});
}
