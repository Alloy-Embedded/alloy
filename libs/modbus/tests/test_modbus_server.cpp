// rtu_server: every function code dispatched against a four-space model with
// byte-exact responses, the spec silences (wrong unit, CRC corruption,
// broadcast) asserted as ABSENCE of bytes — the hard half of a server's
// contract — and the wire exceptions (0x01/0x02/0x03) each earned by the
// exact refusal that produces them.

#include "modbus/server.hpp"

#include <chrono>
#include <cstdint>
#include <span>

#include "alloy_test.hpp"
#include "modbus/client.hpp"  // the loop-back test drives both halves
#include "modbus/crc.hpp"
#include "modbus/data_model.hpp"
#include "testkit/mock_wire.hpp"

namespace {
using namespace alloy::lib::modbus;
using alloy::testkit::mock_serial;
using alloy::testkit::virtual_clock;

struct stepping_clock {
    virtual_clock* clk;
    std::uint32_t step_us;
    [[nodiscard]] std::uint32_t now_us() const {
        clk->advance_us(step_us);
        return clk->now_us;
    }
};

// Four real spaces, 16 addresses each, seeded with distinct patterns so a
// space mix-up (coils answering a discrete-input read) changes the bytes.
struct test_model {
    bool coils[16]{};
    bool discrete[16]{true, false, true, true};       // 0b1101 at the bottom
    std::uint16_t holding[16]{0x1111, 0x2222};
    std::uint16_t input[16]{0xAA01, 0xAA02};

    [[nodiscard]] bool read_coil(std::uint16_t a, bool& v) const {
        if (a >= 16) { return false; }
        v = coils[a];
        return true;
    }
    [[nodiscard]] bool write_coil(std::uint16_t a, bool v) {
        if (a >= 16) { return false; }
        coils[a] = v;
        return true;
    }
    [[nodiscard]] bool read_discrete_input(std::uint16_t a, bool& v) const {
        if (a >= 16) { return false; }
        v = discrete[a];
        return true;
    }
    [[nodiscard]] bool read_holding(std::uint16_t a, std::uint16_t& v) const {
        if (a >= 16) { return false; }
        v = holding[a];
        return true;
    }
    [[nodiscard]] bool write_holding(std::uint16_t a, std::uint16_t v) {
        if (a >= 16) { return false; }
        holding[a] = v;
        return true;
    }
    [[nodiscard]] bool read_input(std::uint16_t a, std::uint16_t& v) const {
        if (a >= 16) { return false; }
        v = input[a];
        return true;
    }
};
static_assert(DataModel<test_model>);

using server = rtu_server<mock_serial, test_model, 8, stepping_clock>;
constexpr server::config kCfg{.unit = 17, .baud = 19'200};

// Queue a request ADU (body + CRC) as bytes already on the line — a server
// reads first, so no respond_after_tx gating.
void queue_request(mock_serial& wire, std::span<const std::uint8_t> body) {
    std::uint8_t adu[64];
    for (std::size_t i = 0; i < body.size(); ++i) {
        adu[i] = body[i];
    }
    const std::size_t n = append_crc({adu, sizeof adu}, body.size());
    wire.queue_rx({adu, n});
}

// Compare wire.tx against an expected body + computed CRC, byte for byte.
[[nodiscard]] bool tx_is(const mock_serial& wire, std::span<const std::uint8_t> body) {
    std::uint8_t expect[64];
    for (std::size_t i = 0; i < body.size(); ++i) {
        expect[i] = body[i];
    }
    const std::size_t n = append_crc({expect, sizeof expect}, body.size());
    if (wire.tx_len != n) {
        return false;
    }
    for (std::size_t i = 0; i < n; ++i) {
        if (wire.tx[i] != expect[i]) {
            return false;
        }
    }
    return true;
}
}  // namespace

ALLOY_TEST(modbus_server_fc03_answers_the_spec_shape_byte_for_byte) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x02};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());

    const std::uint8_t resp[7] = {0x11, 0x03, 0x04, 0x11, 0x11, 0x22, 0x22};
    ALLOY_CHECK(tx_is(wire, resp));
}

ALLOY_TEST(modbus_server_fc04_reads_the_input_space_not_holding) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x11, 0x04, 0x00, 0x00, 0x00, 0x02};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());

    const std::uint8_t resp[7] = {0x11, 0x04, 0x04, 0xAA, 0x01, 0xAA, 0x02};
    ALLOY_CHECK(tx_is(wire, resp));  // 0x1111 here would be a space mix-up
}

ALLOY_TEST(modbus_server_fc01_fc02_pack_bits_lsb_first) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    model.coils[0] = model.coils[2] = model.coils[7] = true;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t rc[6] = {0x11, 0x01, 0x00, 0x00, 0x00, 0x08};
    queue_request(wire, rc);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t coils_resp[4] = {0x11, 0x01, 0x01, 0b1000'0101};
    ALLOY_CHECK(tx_is(wire, coils_resp));

    wire.reset();
    const std::uint8_t rd[6] = {0x11, 0x02, 0x00, 0x00, 0x00, 0x04};
    queue_request(wire, rd);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t disc_resp[4] = {0x11, 0x02, 0x01, 0b0000'1101};
    ALLOY_CHECK(tx_is(wire, disc_resp));
}

ALLOY_TEST(modbus_server_fc06_writes_and_echoes_fc05_validates_the_value) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x11, 0x06, 0x00, 0x03, 0xBE, 0xEF};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(model.holding[3], 0xBEEFu);
    ALLOY_CHECK(tx_is(wire, req));  // the response IS the request echoed

    // FC05 with a value that is neither 0x0000 nor 0xFF00: exception 0x03.
    wire.reset();
    const std::uint8_t bad[6] = {0x11, 0x05, 0x00, 0x01, 0x12, 0x34};
    queue_request(wire, bad);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc[3] = {0x11, 0x85, 0x03};
    ALLOY_CHECK(tx_is(wire, exc));
    ALLOY_CHECK(!model.coils[1]);  // and nothing was applied
}

ALLOY_TEST(modbus_server_fc10_fc0f_write_blocks_and_ack_addr_count) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t wr[11] = {0x11, 0x10, 0x00, 0x04, 0x00, 0x02,
                                 0x04, 0xCA, 0xFE, 0xD0, 0x0D};
    queue_request(wire, wr);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(model.holding[4], 0xCAFEu);
    ALLOY_CHECK_EQ(model.holding[5], 0xD00Du);
    const std::uint8_t ack[6] = {0x11, 0x10, 0x00, 0x04, 0x00, 0x02};
    ALLOY_CHECK(tx_is(wire, ack));

    wire.reset();
    const std::uint8_t wc[8] = {0x11, 0x0F, 0x00, 0x00, 0x00, 0x09,
                                0x02, 0b0000'0101};  // 9 coils: 0 and 2 on
    // second byte of packed data:
    std::uint8_t wc_full[9];
    for (std::size_t i = 0; i < 8; ++i) { wc_full[i] = wc[i]; }
    wc_full[8] = 0b0000'0001;  // coil 8 on
    queue_request(wire, wc_full);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK(model.coils[0]);
    ALLOY_CHECK(!model.coils[1]);
    ALLOY_CHECK(model.coils[2]);
    ALLOY_CHECK(model.coils[8]);
    const std::uint8_t cack[6] = {0x11, 0x0F, 0x00, 0x00, 0x00, 0x09};
    ALLOY_CHECK(tx_is(wire, cack));
}

ALLOY_TEST(modbus_server_out_of_range_address_earns_exception_02) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x11, 0x03, 0x27, 0x0F, 0x00, 0x02};  // @9999
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc[3] = {0x11, 0x83, 0x02};
    ALLOY_CHECK(tx_is(wire, exc));
}

ALLOY_TEST(modbus_server_count_beyond_the_budget_earns_exception_03) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    // 20 registers is legal per spec but beyond this server's MaxRegisters=8.
    const std::uint8_t req[6] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x14};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc[3] = {0x11, 0x83, 0x03};
    ALLOY_CHECK(tx_is(wire, exc));
}

ALLOY_TEST(modbus_server_unknown_function_earns_exception_01_via_silence_close) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    // Vendor FC 0x41: no length rule, so the frame closes only by t3.5 —
    // this walks the silence path end to end through the server.
    const std::uint8_t req[4] = {0x11, 0x41, 0xCA, 0xFE};
    queue_request(wire, req);
    ALLOY_CHECK(!srv.poll());          // assembled, still waiting on silence
    vc.advance_us(rtu_times_for(19'200).t3_5_us);
    ALLOY_CHECK(srv.poll());           // tick closes, dispatch refuses
    const std::uint8_t exc[3] = {0x11, 0xC1, 0x01};
    ALLOY_CHECK(tx_is(wire, exc));
}

ALLOY_TEST(modbus_server_other_units_conversations_get_total_silence) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x05, 0x03, 0x00, 0x00, 0x00, 0x01};  // unit 5
    queue_request(wire, req);
    ALLOY_CHECK(!srv.poll());
    ALLOY_CHECK_EQ(wire.tx_len, 0u);   // not one byte — absence is the contract
}

ALLOY_TEST(modbus_server_crc_corruption_gets_no_reaction) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    std::uint8_t adu[8] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x02, 0, 0};
    (void)append_crc(adu, 6);
    adu[3] ^= 0x08u;  // corrupt after the CRC was computed
    wire.queue_rx(adu);
    ALLOY_CHECK(!srv.poll());
    ALLOY_CHECK_EQ(wire.tx_len, 0u);
}

ALLOY_TEST(modbus_server_broadcast_write_executes_and_never_answers) {
    mock_serial wire;
    virtual_clock vc;
    test_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[6] = {0x00, 0x06, 0x00, 0x02, 0x77, 0x77};  // unit 0
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());               // dispatched...
    ALLOY_CHECK_EQ(model.holding[2], 0x7777u);  // ...and applied
    ALLOY_CHECK_EQ(wire.tx_len, 0u);       // ...but the bus stays untouched

    // Broadcast READ is meaningless: dropped whole, nothing executes.
    const std::uint8_t rd[6] = {0x00, 0x03, 0x00, 0x00, 0x00, 0x01};
    queue_request(wire, rd);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(wire.tx_len, 0u);
}

ALLOY_TEST(modbus_server_holding_bank_serves_fc03_and_refuses_other_spaces) {
    mock_serial wire;
    virtual_clock vc;
    holding_bank<64> bank;
    bank.regs[0] = 0x1234;
    rtu_server<mock_serial, holding_bank<64>, 8, stepping_clock> srv{
        wire, bank, {.unit = 17, .baud = 19'200}, {&vc, 100}};

    const std::uint8_t req[6] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x01};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t resp[5] = {0x11, 0x03, 0x02, 0x12, 0x34};
    ALLOY_CHECK(tx_is(wire, resp));

    // The bank holds NO coils: FC01 against it is exception 0x02, honestly.
    wire.reset();
    const std::uint8_t rc[6] = {0x11, 0x01, 0x00, 0x00, 0x00, 0x01};
    queue_request(wire, rc);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc[3] = {0x11, 0x81, 0x02};
    ALLOY_CHECK(tx_is(wire, exc));
}

ALLOY_TEST(modbus_server_a_full_client_server_transaction_closes_the_loop) {
    // The two halves of this library against each other: rtu_client speaks
    // into a wire whose RX is the server's TX and vice versa — one mock per
    // direction, bytes ferried between polls.
    mock_serial client_wire;
    mock_serial server_wire;
    virtual_clock vc;
    test_model model;
    server srv{server_wire, model, kCfg, {&vc, 50}};
    rtu_client<mock_serial, 8, stepping_clock> cli{
        client_wire, {.baud = 19'200, .response_timeout = std::chrono::microseconds{200'000}},
        {&vc, 50}};

    model.holding[0] = 0x5555;

    // Client emits the request into its wire; ferry to the server; server
    // answers into its wire; ferry back BEFORE the client's deadline. The
    // ferrying happens inside the client's blocking call via a scripted
    // pre-load: run the request TX first by a dry call... simpler: do the
    // ferry by hand around a manual request instead.
    std::uint8_t req_pdu[5];
    const auto n = build_read_request(function::read_holding_registers, 0, 1, req_pdu);
    ALLOY_CHECK(n.has_value());
    std::uint8_t adu[16] = {0x11};
    for (std::size_t i = 0; i < *n; ++i) { adu[1 + i] = req_pdu[i]; }
    const std::size_t adu_len = append_crc({adu, sizeof adu}, 1 + *n);
    server_wire.queue_rx({adu, adu_len});
    ALLOY_CHECK(srv.poll());

    // Server's TX is a valid response the CLIENT parses to the same value.
    client_wire.respond_after_tx = true;
    client_wire.queue_rx({server_wire.tx, server_wire.tx_len});
    std::uint16_t regs[1] = {};
    const auto r = cli.read_holding(17, 0, regs);
    ALLOY_CHECK(r.has_value());
    ALLOY_CHECK_EQ(regs[0], 0x5555u);
}
