// rtu_client: byte-exact TX against the plan's golden frame, the correlation
// checks the reference C library never performed (wrong unit, wrong function,
// echo mismatch), deterministic timeout via an injected stepping clock, the
// RS-485 DE envelope around the TX burst, and the stale-late-reply drain —
// RTU has no transaction id, so hygiene IS the other half of correlation.

#include "modbus/client.hpp"

#include <cstdint>
#include <span>

#include "alloy_test.hpp"
#include "modbus/crc.hpp"
#include "testkit/mock_wire.hpp"

namespace {
using namespace alloy::lib::modbus;
using alloy::testkit::mock_serial;
using alloy::testkit::virtual_clock;
using namespace std::chrono_literals;

// Every now_us() call advances time — the poll loop inside the client then
// walks toward its own deadline instead of spinning a frozen clock forever.
struct stepping_clock {
    virtual_clock* clk;
    std::uint32_t step_us;
    [[nodiscard]] std::uint32_t now_us() const {
        clk->advance_us(step_us);
        return clk->now_us;
    }
};

// Records WHERE in the TX stream the DE pin moved: proof the enable wraps
// the whole burst, not just most of it. The client holds De BY VALUE, so the
// recorder writes through pointers to slots the test still owns.
struct de_recorder {
    mock_serial* wire;
    std::size_t* high_at;
    std::size_t* low_at;
    void set_high() const { *high_at = wire->tx_len; }
    void set_low() const { *low_at = wire->tx_len; }
    void toggle() const {}
};
static_assert(alloy::OutputPin<de_recorder>);

// Response ADU helper: body + CRC queued as a half-duplex PEER would send it
// — readable only after the client transmits its request, so the client's
// pre-send hygiene drain (which kills stale bytes) leaves it alone.
void queue_response(mock_serial& wire, std::span<const std::uint8_t> body) {
    std::uint8_t adu[64];
    for (std::size_t i = 0; i < body.size(); ++i) {
        adu[i] = body[i];
    }
    const std::size_t n = append_crc({adu, sizeof adu}, body.size());
    wire.respond_after_tx = true;
    wire.queue_rx({adu, n});
}

using client = rtu_client<mock_serial, 8, stepping_clock>;
constexpr client::config kCfg{.baud = 19'200, .response_timeout = 500ms};
}  // namespace

ALLOY_TEST(modbus_client_read_holding_emits_the_golden_frame_and_parses) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t resp[7] = {0x11, 0x03, 0x04, 0x00, 0x2A, 0x01, 0x02};
    queue_response(wire, resp);

    std::uint16_t regs[2] = {};
    const auto r = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(r.has_value());
    ALLOY_CHECK_EQ(*r, 2u);
    ALLOY_CHECK_EQ(regs[0], 0x002Au);
    ALLOY_CHECK_EQ(regs[1], 0x0102u);

    // The plan's byte-exact claim: 11 03 00 00 00 02 C6 9B on the wire.
    const std::uint8_t expect[8] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC6, 0x9B};
    ALLOY_CHECK_EQ(wire.tx_len, 8u);
    bool same = true;
    for (std::size_t i = 0; i < 8; ++i) {
        same = same && wire.tx[i] == expect[i];
    }
    ALLOY_CHECK(same);
}

ALLOY_TEST(modbus_client_response_from_another_unit_is_refused) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t resp[5] = {0x12, 0x03, 0x02, 0x00, 0x2A};  // unit 18
    queue_response(wire, resp);

    std::uint16_t regs[1] = {};
    const auto r = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::unexpected_unit);
}

ALLOY_TEST(modbus_client_wrong_function_answering_is_refused) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t resp[5] = {0x11, 0x04, 0x02, 0x00, 0x2A};  // FC04 to a FC03 ask
    queue_response(wire, resp);

    std::uint16_t regs[1] = {};
    const auto r = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::unexpected_function);
}

ALLOY_TEST(modbus_client_wire_exception_arrives_with_its_code) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t resp[3] = {0x11, 0x83, 0x02};  // illegal data address
    queue_response(wire, resp);

    std::uint16_t regs[1] = {};
    const auto r = bus.read_holding(17, 0x7000, regs);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(is_exception(r.error()));
    ALLOY_CHECK_EQ(exception_code(r.error()), 0x02u);
}

ALLOY_TEST(modbus_client_silent_bus_times_out_and_the_client_is_reusable) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 1'000}};  // 1 ms per poll → ~500 polls to deadline

    std::uint16_t regs[1] = {};
    const auto first = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(!first);
    ALLOY_CHECK(first.error() == modbus_error::timeout);

    // Second transaction on the same object succeeds — no wedged state.
    const std::uint8_t resp[5] = {0x11, 0x03, 0x02, 0x00, 0x2A};
    queue_response(wire, resp);
    const auto second = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(second.has_value());
    ALLOY_CHECK_EQ(regs[0], 0x002Au);
}

ALLOY_TEST(modbus_client_de_pin_wraps_the_entire_tx_burst) {
    mock_serial wire;
    virtual_clock vc;
    std::size_t high_at = static_cast<std::size_t>(-1);
    std::size_t low_at = static_cast<std::size_t>(-1);
    rtu_client<mock_serial, 8, stepping_clock, de_recorder> bus{
        wire, kCfg, {&vc, 1'000}, de_recorder{&wire, &high_at, &low_at}};

    std::uint16_t regs[1] = {};
    (void)bus.read_holding(17, 0, regs);  // times out; the TX side is what matters

    // High BEFORE the first byte left, low only after ALL 8 bytes did — a DE
    // that drops early truncates the CRC on a real transceiver, and the
    // failure would read as a CRC bug on the PEER'S side.
    ALLOY_CHECK_EQ(high_at, 0u);
    ALLOY_CHECK_EQ(low_at, 8u);
    ALLOY_CHECK_EQ(wire.tx_len, 8u);
}

ALLOY_TEST(modbus_client_stale_late_reply_cannot_answer_the_next_request) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 1'000}};

    // A late FC06 echo from some earlier, timed-out transaction sits in the
    // RX path — queued WITHOUT respond_after_tx, i.e. already on the line
    // before this request exists. Without the drain it would be parsed as
    // THIS FC03's response and fail as unexpected_function; with it, the
    // line is simply quiet.
    std::uint8_t late[7] = {0x11, 0x06, 0x00, 0x01, 0x00, 0, 0};
    const std::size_t n = append_crc(late, 5);
    wire.queue_rx({late, n});

    std::uint16_t regs[1] = {};
    const auto r = bus.read_holding(17, 0, regs);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::timeout);  // NOT unexpected_function
}

ALLOY_TEST(modbus_client_write_register_verifies_the_echo) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t echo[6] = {0x11, 0x06, 0x00, 0x01, 0x00, 0x30};
    queue_response(wire, echo);
    ALLOY_CHECK(bus.write_register(17, 0x0001, 0x0030).has_value());

    // An echo whose value differs is a malformed transaction, not a success.
    wire.reset();
    const std::uint8_t wrong[6] = {0x11, 0x06, 0x00, 0x01, 0x00, 0x31};
    queue_response(wire, wrong);
    const auto r = bus.write_register(17, 0x0001, 0x0030);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::malformed);
}

ALLOY_TEST(modbus_client_broadcast_write_returns_without_waiting) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    // Nothing queued, nothing to answer: unit 0 must return ok immediately —
    // and a broadcast READ is meaningless, refused before it reaches a wire.
    ALLOY_CHECK(bus.write_register(0, 0x0001, 0x1234).has_value());
    ALLOY_CHECK_EQ(wire.tx_len, 8u);

    std::uint16_t regs[1] = {};
    const auto r = bus.read_holding(0, 0, regs);
    ALLOY_CHECK(!r);
    ALLOY_CHECK(r.error() == modbus_error::malformed);
}

ALLOY_TEST(modbus_client_write_registers_round_trips_fc16) {
    mock_serial wire;
    virtual_clock vc;
    client bus{wire, kCfg, {&vc, 100}};

    const std::uint8_t ack[6] = {0x11, 0x10, 0x00, 0x10, 0x00, 0x02};
    queue_response(wire, ack);

    const std::uint16_t values[2] = {0xBEEF, 0xCAFE};
    ALLOY_CHECK(bus.write_registers(17, 0x0010, values).has_value());

    // TX: 11 10 00 10 00 02 04 BE EF CA FE + CRC — check the shape.
    ALLOY_CHECK_EQ(wire.tx_len, 13u);
    ALLOY_CHECK_EQ(wire.tx[1], 0x10u);
    ALLOY_CHECK_EQ(wire.tx[6], 0x04u);   // byte count
    ALLOY_CHECK_EQ(wire.tx[7], 0xBEu);   // big-endian data
}
