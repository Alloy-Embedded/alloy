// RS-485 DE pin decorator tests.
// Verifies that DE is asserted before the first byte and de-asserted after flush.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "alloy/modbus/transport/loopback_stream.hpp"
#include "alloy/modbus/transport/rs485_de.hpp"

using namespace alloy::modbus;

namespace {

constexpr std::byte b(std::uint8_t v) { return std::byte{v}; }

// Minimal fake DE pin that records state transitions.
struct FakeDePin {
    bool state{false};
    int  high_calls{0};
    int  low_calls{0};

    void set_high() noexcept { state = true;  ++high_calls; }
    void set_low()  noexcept { state = false; ++low_calls;  }
};

static_assert(DePin<FakeDePin>);

}  // namespace

TEST_CASE("Rs485DeStream: DE is low initially, high after write, low after flush") {
    LoopbackPair<> pair;
    auto ep = pair.a();
    FakeDePin de;
    Rs485DeStream rs485{ep, de};

    CHECK(!de.state);
    CHECK(de.high_calls == 0);
    CHECK(de.low_calls  == 0);

    const std::array<std::byte, 4> data{b(0x01), b(0x02), b(0x03), b(0x04)};
    REQUIRE(rs485.write(data).is_ok());

    CHECK(de.state);
    CHECK(de.high_calls == 1);
    CHECK(de.low_calls  == 0);

    REQUIRE(rs485.flush(0u).is_ok());

    CHECK(!de.state);
    CHECK(de.high_calls == 1);
    CHECK(de.low_calls  == 1);
}

TEST_CASE("Rs485DeStream: DE is re-asserted on each write call") {
    LoopbackPair<> pair;
    auto ep = pair.a();
    FakeDePin de;
    Rs485DeStream rs485{ep, de};

    const std::array<std::byte, 2> frame{b(0x01), b(0x02)};

    REQUIRE(rs485.write(frame).is_ok());
    REQUIRE(rs485.flush(0u).is_ok());
    REQUIRE(rs485.write(frame).is_ok());
    REQUIRE(rs485.flush(0u).is_ok());

    CHECK(de.high_calls == 2);
    CHECK(de.low_calls  == 2);
    CHECK(!de.state);
}

TEST_CASE("Rs485DeStream: read passes through, DE unchanged") {
    LoopbackPair<> pair;
    auto tx = pair.a();
    auto rx_ep = pair.b();
    FakeDePin de;
    Rs485DeStream rx{rx_ep, de};

    const std::array<std::byte, 3> out{b(0xAA), b(0xBB), b(0xCC)};
    REQUIRE(tx.write(out).is_ok());

    std::array<std::byte, 8> buf{};
    const auto n = rx.read(buf, 0u);
    REQUIRE(n.is_ok());
    CHECK(n.unwrap() == 3u);
    CHECK(buf[0] == b(0xAA));
    CHECK(buf[2] == b(0xCC));

    // DE should remain untouched during read
    CHECK(de.high_calls == 0);
    CHECK(de.low_calls  == 0);
}

TEST_CASE("Rs485DeStream: flush lowers DE even if inner flush returns ok") {
    LoopbackPair<> pair;
    auto ep = pair.a();
    FakeDePin de;
    Rs485DeStream rs485{ep, de};

    const std::array<std::byte, 1> frame{b(0xFF)};
    REQUIRE(rs485.write(frame).is_ok());
    CHECK(de.state);

    // flush on loopback always succeeds; DE must still be lowered
    const auto res = rs485.flush(1000u);
    CHECK(res.is_ok());
    CHECK(!de.state);
}

TEST_CASE("Rs485DeStream: wait_idle passes through, DE unchanged") {
    LoopbackPair<> pair;
    auto ep = pair.a();
    FakeDePin de;
    Rs485DeStream rs485{ep, de};

    const auto res = rs485.wait_idle(0u);
    CHECK(res.is_ok());
    CHECK(de.high_calls == 0);
    CHECK(de.low_calls  == 0);
}

TEST_CASE("Rs485DeStream: satisfies ByteStream concept") {
    static_assert(ByteStream<Rs485DeStream<LoopbackEndpoint<512u>, FakeDePin>>);
}
