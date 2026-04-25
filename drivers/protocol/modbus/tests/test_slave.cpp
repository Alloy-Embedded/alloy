// Modbus RTU slave tests — all FCs over LoopbackPair.
// Master sends a properly framed RTU request; slave.poll() processes it
// and writes the response back; master decodes and checks byte-for-byte.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/pdu.hpp"
#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/rtu_frame.hpp"
#include "alloy/modbus/slave.hpp"
#include "alloy/modbus/transport/loopback_stream.hpp"
#include "alloy/modbus/var.hpp"

using namespace alloy::modbus;

// ============================================================================
// Test fixture helpers
// ============================================================================

namespace {

constexpr std::uint8_t kSlaveId = 0x01u;
constexpr std::byte b(std::uint8_t v) { return std::byte{v}; }

// Build a complete RTU frame around a raw PDU byte list.
template <std::size_t N>
std::array<std::byte, N + 3u> make_rtu_frame(
    std::uint8_t slave_id,
    const std::array<std::byte, N>& pdu) {
    std::array<std::byte, N + 3u> frame{};
    frame[0] = std::byte{slave_id};
    for (std::size_t i = 0u; i < N; ++i) frame[i + 1u] = pdu[i];
    const auto crc = crc16(std::span<const std::byte>{frame.data(), 1u + N});
    frame[1u + N] = std::byte{static_cast<std::uint8_t>(crc & 0xFFu)};
    frame[2u + N] = std::byte{static_cast<std::uint8_t>(crc >> 8u)};
    return frame;
}

// Transact: master writes a PDU as an RTU frame, slave polls, master reads back.
// Returns the decoded response PDU bytes (in a fixed buffer; size in out_len).
struct Transact {
    template <std::size_t PduN, ByteStream MasterStream,
              ByteStream SlaveStream, std::size_t RegN>
    static std::pair<std::array<std::byte, 256>, std::size_t> run(
        MasterStream& master, SlaveStream& slave_stream,
        Slave<SlaveStream, RegN>& slave,
        const std::array<std::byte, PduN>& req_pdu) {
        // Encode and send request RTU frame from master
        std::array<std::byte, 256u> req_frame{};
        const auto enc = encode_rtu_frame(req_frame, kSlaveId, req_pdu);
        REQUIRE(enc.is_ok());
        REQUIRE(master.write(
            std::span<const std::byte>{req_frame.data(), enc.unwrap()}).is_ok());

        // Slave processes one frame
        const auto poll_res = slave.poll(0u);
        REQUIRE(poll_res.is_ok());
        REQUIRE(poll_res.unwrap() == true);

        // Master reads the response RTU frame
        std::array<std::byte, 256u> rsp_buf{};
        const auto n = master.read(rsp_buf, 0u);
        REQUIRE(n.is_ok());
        REQUIRE(n.unwrap() >= 4u);  // min: 1 id + 1+ pdu + 2 crc

        // Decode the response
        const auto rsp_frame = decode_rtu_frame(
            std::span<const std::byte>{rsp_buf.data(), n.unwrap()});
        REQUIRE(rsp_frame.is_ok());
        CHECK(rsp_frame.unwrap().slave_id == kSlaveId);

        std::array<std::byte, 256u> pdu_out{};
        const auto pdu_span = rsp_frame.unwrap().pdu;
        std::copy(pdu_span.begin(), pdu_span.end(), pdu_out.begin());
        return {pdu_out, pdu_span.size()};
    }
};

}  // namespace

// ============================================================================
// Test variables (shared across test cases via manual construction per case)
// ============================================================================

// ============================================================================
// FC 0x03 — Read Holding Registers
// ============================================================================

TEST_CASE("Slave FC03: read single uint16_t register") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t status = 0xABCDu;
    constexpr Var<std::uint16_t> status_var{
        .address = 100u, .access = Access::ReadWrite, .name = "status"};

    auto reg = Registry<1>{{bind(status_var, status)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC03 request: addr=100, qty=1
    std::array<std::byte, 5u> req{
        b(0x03), b(0x00), b(0x64), b(0x00), b(0x01)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // Response: FC=0x03, byte_count=2, hi=0xAB, lo=0xCD
    CHECK(rsp_len == 4u);
    CHECK(rsp[0] == b(0x03));
    CHECK(rsp[1] == b(0x02));  // byte count
    CHECK(rsp[2] == b(0xAB));
    CHECK(rsp[3] == b(0xCD));
}

TEST_CASE("Slave FC03: read float (two registers)") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    float temperature = 25.0f;  // 0x41C80000
    constexpr Var<float> temp_var{
        .address = 200u, .access = Access::ReadOnly, .name = "temp",
        .word_order = WordOrder::ABCD};

    auto reg = Registry<1>{{bind(temp_var, temperature)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC03 request: addr=200, qty=2
    std::array<std::byte, 5u> req{
        b(0x03), b(0x00), b(0xC8), b(0x00), b(0x02)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp_len == 6u);
    CHECK(rsp[0] == b(0x03));
    CHECK(rsp[1] == b(0x04));  // byte count = 4
    // 25.0f = 0x41C80000 → hi word=0x41C8, lo word=0x0000
    CHECK(rsp[2] == b(0x41));
    CHECK(rsp[3] == b(0xC8));
    CHECK(rsp[4] == b(0x00));
    CHECK(rsp[5] == b(0x00));
}

TEST_CASE("Slave FC03: exception on unmapped address") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t val = 0u;
    constexpr Var<std::uint16_t> v{
        .address = 0u, .access = Access::ReadWrite, .name = "x"};
    auto reg = Registry<1>{{bind(v, val)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // Request address 999 — not mapped
    std::array<std::byte, 5u> req{
        b(0x03), b(0x03), b(0xE7), b(0x00), b(0x01)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // Exception response: FC=0x83, code=0x02 (IllegalDataAddress)
    CHECK(rsp_len == 2u);
    CHECK(rsp[0] == b(0x83));
    CHECK(rsp[1] == b(0x02));
}

// ============================================================================
// FC 0x04 — Read Input Registers (read-only vars)
// ============================================================================

TEST_CASE("Slave FC04: read read-only register") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t sensor = 0x1234u;
    constexpr Var<std::uint16_t> s{
        .address = 50u, .access = Access::ReadOnly, .name = "sensor"};
    auto reg = Registry<1>{{bind(s, sensor)}};
    Slave slave{slave_ep, kSlaveId, reg};

    std::array<std::byte, 5u> req{
        b(0x04), b(0x00), b(0x32), b(0x00), b(0x01)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x04));
    CHECK(rsp[2] == b(0x12));
    CHECK(rsp[3] == b(0x34));
}

// ============================================================================
// FC 0x06 — Write Single Register
// ============================================================================

TEST_CASE("Slave FC06: write single register updates value") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t ctrl = 0u;
    constexpr Var<std::uint16_t> cv{
        .address = 10u, .access = Access::ReadWrite, .name = "ctrl"};
    auto reg = Registry<1>{{bind(cv, ctrl)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC06 request: addr=10, value=0x0042
    std::array<std::byte, 5u> req{
        b(0x06), b(0x00), b(0x0A), b(0x00), b(0x42)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // Echo response: same addr and value
    CHECK(rsp[0] == b(0x06));
    CHECK(rsp[1] == b(0x00));
    CHECK(rsp[2] == b(0x0A));
    CHECK(rsp[3] == b(0x00));
    CHECK(rsp[4] == b(0x42));

    // Value actually updated
    CHECK(ctrl == 0x0042u);
}

TEST_CASE("Slave FC06: exception on read-only register") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t ro_val = 0u;
    constexpr Var<std::uint16_t> ro{
        .address = 20u, .access = Access::ReadOnly, .name = "ro"};
    auto reg = Registry<1>{{bind(ro, ro_val)}};
    Slave slave{slave_ep, kSlaveId, reg};

    std::array<std::byte, 5u> req{
        b(0x06), b(0x00), b(0x14), b(0x12), b(0x34)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x86));  // FC | 0x80
    CHECK(rsp[1] == b(0x02));  // IllegalDataAddress
}

// ============================================================================
// FC 0x10 — Write Multiple Registers
// ============================================================================

TEST_CASE("Slave FC10: write two consecutive uint16_t registers") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t a_val = 0u;
    std::uint16_t b_val = 0u;
    constexpr Var<std::uint16_t> va{
        .address = 30u, .access = Access::ReadWrite, .name = "a"};
    constexpr Var<std::uint16_t> vb{
        .address = 31u, .access = Access::ReadWrite, .name = "b"};
    auto reg = Registry<2>{{bind(va, a_val), bind(vb, b_val)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC10: start=30, qty=2, byte_count=4, data: 0x0011 0x0022
    std::array<std::byte, 10u> req{
        b(0x10), b(0x00), b(0x1E), b(0x00), b(0x02), b(0x04),
        b(0x00), b(0x11), b(0x00), b(0x22)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // Response: FC=0x10, addr=30, qty=2
    CHECK(rsp[0] == b(0x10));
    CHECK(rsp[3] == b(0x00));
    CHECK(rsp[4] == b(0x02));

    CHECK(a_val == 0x0011u);
    CHECK(b_val == 0x0022u);
}

TEST_CASE("Slave FC10: write float (two registers)") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    float setpoint = 0.0f;
    constexpr Var<float> sp{
        .address = 40u, .access = Access::ReadWrite, .name = "setpoint",
        .word_order = WordOrder::ABCD};
    auto reg = Registry<1>{{bind(sp, setpoint)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // Encode 25.0f = 0x41C80000 → hi=0x41C8, lo=0x0000
    // FC10: start=40, qty=2, byte_count=4, data: 41 C8 00 00
    std::array<std::byte, 10u> req{
        b(0x10), b(0x00), b(0x28), b(0x00), b(0x02), b(0x04),
        b(0x41), b(0xC8), b(0x00), b(0x00)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x10));
    CHECK(setpoint == 25.0f);
}

// ============================================================================
// FC 0x17 — Read/Write Multiple Registers
// ============================================================================

TEST_CASE("Slave FC17: write then read in one transaction") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t x = 0u;
    std::uint16_t y = 0xFFFFu;
    constexpr Var<std::uint16_t> vx{
        .address = 0u, .access = Access::ReadWrite, .name = "x"};
    constexpr Var<std::uint16_t> vy{
        .address = 1u, .access = Access::ReadWrite, .name = "y"};
    auto reg = Registry<2>{{bind(vx, x), bind(vy, y)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC17: read_addr=1, read_qty=1, write_addr=0, write_qty=1,
    //       byte_count=2, write_data: 0x1234
    // PDU = 1+2+2+2+2+1+2 = 12 bytes
    std::array<std::byte, 12u> req{
        b(0x17),
        b(0x00), b(0x01),  // read addr = 1
        b(0x00), b(0x01),  // read qty = 1
        b(0x00), b(0x00),  // write addr = 0
        b(0x00), b(0x01),  // write qty = 1
        b(0x02),           // byte count = 2
        b(0x12), b(0x34),  // write data = 0x1234
    };
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // Write applied: x should be 0x1234
    CHECK(x == 0x1234u);

    // Read response: FC=0x17, byte_count=2, then reg[1]=y=0xFFFF
    CHECK(rsp[0] == b(0x17));
    CHECK(rsp[1] == b(0x02));
    CHECK(rsp[2] == b(0xFF));
    CHECK(rsp[3] == b(0xFF));
}

// ============================================================================
// FC 0x01 — Read Coils
// ============================================================================

TEST_CASE("Slave FC01: read bool coils") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    bool c0 = true;
    bool c1 = false;
    bool c2 = true;
    constexpr Var<bool> vc0{.address = 0u, .access = Access::ReadWrite, .name = "c0"};
    constexpr Var<bool> vc1{.address = 1u, .access = Access::ReadWrite, .name = "c1"};
    constexpr Var<bool> vc2{.address = 2u, .access = Access::ReadWrite, .name = "c2"};
    auto reg = Registry<3>{{bind(vc0, c0), bind(vc1, c1), bind(vc2, c2)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC01: addr=0, qty=3
    std::array<std::byte, 5u> req{
        b(0x01), b(0x00), b(0x00), b(0x00), b(0x03)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    // c0=1, c1=0, c2=1 → packed = 0b00000101 = 0x05
    CHECK(rsp[0] == b(0x01));
    CHECK(rsp[1] == b(0x01));  // byte count = 1
    CHECK(rsp[2] == b(0x05));  // bits: c0=bit0=1, c1=bit1=0, c2=bit2=1
}

// ============================================================================
// FC 0x05 — Write Single Coil
// ============================================================================

TEST_CASE("Slave FC05: write single coil ON") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    bool flag = false;
    constexpr Var<bool> vf{
        .address = 5u, .access = Access::ReadWrite, .name = "flag"};
    auto reg = Registry<1>{{bind(vf, flag)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC05: addr=5, value=0xFF00 (ON)
    std::array<std::byte, 5u> req{
        b(0x05), b(0x00), b(0x05), b(0xFF), b(0x00)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x05));
    CHECK(flag == true);
}

TEST_CASE("Slave FC05: write single coil OFF") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    bool flag = true;
    constexpr Var<bool> vf{
        .address = 5u, .access = Access::ReadWrite, .name = "flag"};
    auto reg = Registry<1>{{bind(vf, flag)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC05: addr=5, value=0x0000 (OFF)
    std::array<std::byte, 5u> req{
        b(0x05), b(0x00), b(0x05), b(0x00), b(0x00)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x05));
    CHECK(flag == false);
}

// ============================================================================
// FC 0x0F — Write Multiple Coils
// ============================================================================

TEST_CASE("Slave FC0F: write multiple coils") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    bool c0 = false, c1 = false, c2 = false, c3 = false;
    constexpr Var<bool> vc0{.address = 10u, .access = Access::ReadWrite, .name = "c0"};
    constexpr Var<bool> vc1{.address = 11u, .access = Access::ReadWrite, .name = "c1"};
    constexpr Var<bool> vc2{.address = 12u, .access = Access::ReadWrite, .name = "c2"};
    constexpr Var<bool> vc3{.address = 13u, .access = Access::ReadWrite, .name = "c3"};
    auto reg = Registry<4>{{bind(vc0, c0), bind(vc1, c1), bind(vc2, c2), bind(vc3, c3)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC0F PDU: [FC][addr_hi][addr_lo][qty_hi][qty_lo][byte_count][data...]
    // = 1+2+2+1+1 = 7 bytes; data=0b00001101 (c0=bit0=1, c1=bit1=0, c2=bit2=1, c3=bit3=1)
    std::array<std::byte, 7u> req{
        b(0x0F), b(0x00), b(0x0A), b(0x00), b(0x04), b(0x01), b(0x0D)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp[0] == b(0x0F));
    CHECK(c0 == true);
    CHECK(c1 == false);
    CHECK(c2 == true);
    CHECK(c3 == true);
}

// ============================================================================
// Frame not for this slave — no response
// ============================================================================

TEST_CASE("Slave: ignores frame for different slave ID") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t val = 0u;
    constexpr Var<std::uint16_t> v{
        .address = 0u, .access = Access::ReadWrite, .name = "v"};
    auto reg = Registry<1>{{bind(v, val)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // Send a valid frame for slave ID 0x02 (not our slave 0x01)
    std::array<std::byte, 5u> pdu{
        b(0x03), b(0x00), b(0x00), b(0x00), b(0x01)};
    std::array<std::byte, 8u> frame{};
    frame[0] = b(0x02);  // wrong slave id
    for (std::size_t i = 0; i < 5u; ++i) frame[i + 1u] = pdu[i];
    const auto crc = crc16(std::span<const std::byte>{frame.data(), 6u});
    frame[6] = std::byte{static_cast<std::uint8_t>(crc & 0xFF)};
    frame[7] = std::byte{static_cast<std::uint8_t>(crc >> 8u)};

    REQUIRE(master.write(frame).is_ok());
    const auto poll = slave.poll(0u);
    REQUIRE(poll.is_ok());
    CHECK(poll.unwrap() == false);  // not processed

    // No response in master's RX buffer
    std::array<std::byte, 8u> rx{};
    CHECK(master.read(rx, 0u).unwrap() == 0u);
}

// ============================================================================
// Slave FC03: multiple vars in one read span
// ============================================================================

TEST_CASE("Slave FC03: read across two adjacent uint16_t vars") {
    LoopbackPair<> pair;
    auto master = pair.a();
    auto slave_ep = pair.b();

    std::uint16_t v0 = 0x0011u;
    std::uint16_t v1 = 0x0022u;
    constexpr Var<std::uint16_t> var0{
        .address = 0u, .access = Access::ReadOnly, .name = "v0"};
    constexpr Var<std::uint16_t> var1{
        .address = 1u, .access = Access::ReadOnly, .name = "v1"};
    auto reg = Registry<2>{{bind(var0, v0), bind(var1, v1)}};
    Slave slave{slave_ep, kSlaveId, reg};

    // FC03: read 2 registers starting at 0
    std::array<std::byte, 5u> req{
        b(0x03), b(0x00), b(0x00), b(0x00), b(0x02)};
    const auto [rsp, rsp_len] = Transact::run(master, slave_ep, slave, req);

    CHECK(rsp_len == 6u);
    CHECK(rsp[2] == b(0x00));
    CHECK(rsp[3] == b(0x11));  // v0 hi, lo
    CHECK(rsp[4] == b(0x00));
    CHECK(rsp[5] == b(0x22));  // v1 hi, lo
}
