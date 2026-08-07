// The two server extension points, exercised with synthetic frames: the
// UserDispatch hook (vendor function codes the eight-FC core does not speak)
// and the rich write verdict (a DataModel that distinguishes WHY it refused —
// wire 0x02 for a missing address vs 0x03 for an illegal value, which a bool
// physically cannot express). Every wire assertion is byte-exact through the
// full rtu_server stack.

#include "modbus/server.hpp"

#include <cstdint>
#include <span>

#include "alloy_test.hpp"
#include "modbus/crc.hpp"
#include "modbus/data_model.hpp"
#include "testkit/mock_wire.hpp"

namespace {
using namespace alloy::lib::modbus;
using alloy::ok;
using alloy::Result;
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

// A model whose holding writes carry the RICH verdict: addresses 0..15 exist,
// values are clamped to <= 100 — so a bad address earns 0x02 and a bad value
// on a GOOD address earns 0x03. Coils stay bool: the two verdict shapes must
// coexist in one model.
struct rich_model {
    std::uint16_t regs[16]{};
    bool coils[8]{};

    [[nodiscard]] bool read_coil(std::uint16_t a, bool& v) const {
        if (a >= 8) { return false; }
        v = coils[a];
        return true;
    }
    [[nodiscard]] bool write_coil(std::uint16_t a, bool v) {
        if (a >= 8) { return false; }
        coils[a] = v;
        return true;
    }
    [[nodiscard]] bool read_discrete_input(std::uint16_t, bool&) const { return false; }
    [[nodiscard]] bool read_holding(std::uint16_t a, std::uint16_t& v) const {
        if (a >= 16) { return false; }
        v = regs[a];
        return true;
    }
    [[nodiscard]] Result<void, modbus_error> write_holding(std::uint16_t a,
                                                           std::uint16_t v) {
        if (a >= 16) {
            return modbus_error::exception_illegal_data_address;
        }
        if (v > 100u) {
            return modbus_error::exception_illegal_data_value;
        }
        regs[a] = v;
        return ok<modbus_error>();
    }
    [[nodiscard]] bool read_input(std::uint16_t, std::uint16_t&) const { return false; }
};
static_assert(DataModel<rich_model>);

// A hook covering the contract's four outcomes, keyed on the vendor FC.
struct test_dispatch {
    std::uint32_t broadcast_calls = 0;
    [[nodiscard]] Result<std::size_t, modbus_error> operator()(
        std::span<const std::uint8_t> req, std::span<std::uint8_t> resp,
        bool broadcast) {
        if (broadcast) {
            ++broadcast_calls;  // executed; the server owes the bus silence
            return std::size_t{0};
        }
        switch (req[0]) {
        case 0x41:  // answered: echo the fc + a fixed payload
            resp[0] = 0x41;
            resp[1] = 0xCA;
            resp[2] = 0xFE;
            return std::size_t{3};
        case 0x42:  // deliberate silence
            return std::size_t{0};
        case 0x43:  // wire exception passes through untouched
            return modbus_error::exception_server_busy;
        case 0x44:  // a LOCAL error must surface as 0x04, never a wire lie
            return modbus_error::timeout;
        default:  // not mine: the server refuses with 0x01 as if no hook existed
            return modbus_error::unexpected_function;
        }
    }
};

using server =
    rtu_server<mock_serial, rich_model, 8, stepping_clock,
               alloy::gpio::null_output, test_dispatch>;
constexpr server_config kCfg{.unit = 17, .baud = 19'200};

void queue_request(mock_serial& wire, std::span<const std::uint8_t> body) {
    std::uint8_t adu[64];
    for (std::size_t i = 0; i < body.size(); ++i) {
        adu[i] = body[i];
    }
    const std::size_t n = append_crc({adu, sizeof adu}, body.size());
    wire.queue_rx({adu, n});
}

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

ALLOY_TEST(modbus_dispatch_hook_answers_a_vendor_fc_byte_exactly) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    // Vendor FC 0x41 has no length rule: silence-close, then the hook speaks.
    const std::uint8_t req[4] = {0x11, 0x41, 0x00, 0x01};
    queue_request(wire, req);
    ALLOY_CHECK(!srv.poll());
    vc.advance_us(rtu_times_for(19'200).t3_5_us);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t resp[4] = {0x11, 0x41, 0xCA, 0xFE};
    ALLOY_CHECK(tx_is(wire, resp));
}

ALLOY_TEST(modbus_dispatch_hook_outcomes_silence_exception_and_local_error) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    server srv{wire, model, kCfg, {&vc, 100}};
    const auto t35 = rtu_times_for(19'200).t3_5_us;

    // 0x42: the hook handled it and chose silence — not one byte.
    const std::uint8_t rq_silent[3] = {0x11, 0x42, 0x00};
    queue_request(wire, rq_silent);
    (void)srv.poll();
    vc.advance_us(t35);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(wire.tx_len, 0u);

    // 0x43: a wire exception passes through with the hook's own code.
    wire.reset();
    const std::uint8_t rq_busy[3] = {0x11, 0x43, 0x00};
    queue_request(wire, rq_busy);
    (void)srv.poll();
    vc.advance_us(t35);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t busy[3] = {0x11, 0xC3, 0x06};
    ALLOY_CHECK(tx_is(wire, busy));

    // 0x44: a LOCAL error surfaces as 0x04 (server failure), never a wire lie.
    wire.reset();
    const std::uint8_t rq_local[3] = {0x11, 0x44, 0x00};
    queue_request(wire, rq_local);
    (void)srv.poll();
    vc.advance_us(t35);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t fail[3] = {0x11, 0xC4, 0x04};
    ALLOY_CHECK(tx_is(wire, fail));
}

ALLOY_TEST(modbus_dispatch_not_mine_falls_through_to_illegal_function) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    const std::uint8_t req[3] = {0x11, 0x5A, 0x00};  // hook says unexpected_function
    queue_request(wire, req);
    (void)srv.poll();
    vc.advance_us(rtu_times_for(19'200).t3_5_us);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc[3] = {0x11, 0xDA, 0x01};
    ALLOY_CHECK(tx_is(wire, exc));
}

ALLOY_TEST(modbus_dispatch_broadcast_reaches_the_hook_but_never_the_bus) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    test_dispatch hook;
    server srv{wire, model, kCfg, {&vc, 100}, {}, hook};

    const std::uint8_t req[3] = {0x00, 0x41, 0x00};  // unit 0
    queue_request(wire, req);
    (void)srv.poll();
    vc.advance_us(rtu_times_for(19'200).t3_5_us);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK_EQ(wire.tx_len, 0u);  // executed (hook counts it), bus untouched
}

ALLOY_TEST(modbus_rich_model_distinguishes_address_02_from_value_03) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    // FC06 to a good address with an illegal value: 0x03 — the code a bool
    // model can never produce.
    const std::uint8_t bad_value[6] = {0x11, 0x06, 0x00, 0x01, 0x00, 0x65};  // 101
    queue_request(wire, bad_value);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc3[3] = {0x11, 0x86, 0x03};
    ALLOY_CHECK(tx_is(wire, exc3));
    ALLOY_CHECK_EQ(model.regs[1], 0u);  // refused, nothing applied

    // Same FC to a missing address: 0x02, as ever.
    wire.reset();
    const std::uint8_t bad_addr[6] = {0x11, 0x06, 0x00, 0x63, 0x00, 0x01};  // @99
    queue_request(wire, bad_addr);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc2[3] = {0x11, 0x86, 0x02};
    ALLOY_CHECK(tx_is(wire, exc2));

    // And a legal write still echoes; bool coils coexist in the same model.
    wire.reset();
    const std::uint8_t good[6] = {0x11, 0x06, 0x00, 0x01, 0x00, 0x64};  // 100
    queue_request(wire, good);
    ALLOY_CHECK(srv.poll());
    ALLOY_CHECK(tx_is(wire, good));
    ALLOY_CHECK_EQ(model.regs[1], 100u);
}

ALLOY_TEST(modbus_rich_model_fc16_first_bad_value_stops_with_03) {
    mock_serial wire;
    virtual_clock vc;
    rich_model model;
    server srv{wire, model, kCfg, {&vc, 100}};

    // Two registers: first legal, second over the clamp — sequential apply
    // stops at the offender and answers ITS code.
    const std::uint8_t req[11] = {0x11, 0x10, 0x00, 0x00, 0x00, 0x02,
                                  0x04, 0x00, 0x64, 0x00, 0x65};
    queue_request(wire, req);
    ALLOY_CHECK(srv.poll());
    const std::uint8_t exc3[3] = {0x11, 0x90, 0x03};
    ALLOY_CHECK(tx_is(wire, exc3));
    ALLOY_CHECK_EQ(model.regs[0], 100u);  // sequential: the first stuck
}
