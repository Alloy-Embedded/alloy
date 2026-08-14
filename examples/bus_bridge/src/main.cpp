// Bus bridge demo — the local pub/sub extended over the debug uart. The
// robot on the other end (an INDEPENDENT frame implementation in monitor
// Python) sends ping datagrams; they cross the bridge, are republished on
// the LOCAL bus, a subscriber-service answers by PUBLISHING a pong — and
// the pong leaves through the same bridge because a route forwards it.
// Nothing in the service knows a uart exists: it consumes ping, publishes
// pong. That is the demo — the message round-trips THROUGH the bus, so a
// firmware that hardcoded an echo could not produce the incrementing
// `count` the robot asserts.
//
// The SAME file compiles for every alloy board with a debug_uart role:
// plain ByteStream polling, zero preprocessor conditionals, no DMA claim —
// deliberately the simplest transport shape, so this example is the
// portable floor. (The DMA-ring feeder and the co_await tx task are the
// lib's tests' and a later example's business.)
//
// Framing needs NO clock: delimitation is length-driven (see bus/wire.hpp);
// uptime_us feeds only the stall-abandon housekeeping. Baud is 115200 —
// unlike modbus RTU there is no t3.5 to protect, so speed is free.
//
// The wire bindings below are hand-written in the exact shape the bus.toml
// generator (phase F3) will emit; when that lands, these structs move to a
// generated header and the frames stay byte-identical.
#include <alloy/board.hpp>
#include <alloy/time.hpp>
#include <bus.hpp>

#include <cstdint>

namespace bus = alloy::lib::bus;

namespace {

struct ping {
    std::uint32_t token;
};

struct pong {
    std::uint32_t token;
    std::uint32_t count;
};

struct ping_wire {
    using message = ping;
    static constexpr std::uint16_t id = 0x0301;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 4;
    static void encode(const ping& m, std::uint8_t* out) noexcept {
        alloy::byteorder::store_le32(&out[0], m.token);
    }
    static ping decode(const std::uint8_t* in) noexcept {
        return {alloy::byteorder::load_le32(&in[0])};
    }
};

struct pong_wire {
    using message = pong;
    static constexpr std::uint16_t id = 0x0302;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 8;
    static void encode(const pong& m, std::uint8_t* out) noexcept {
        alloy::byteorder::store_le32(&out[0], m.token);
        alloy::byteorder::store_le32(&out[4], m.count);
    }
    static pong decode(const std::uint8_t* in) noexcept {
        return {alloy::byteorder::load_le32(&in[0]), alloy::byteorder::load_le32(&in[4])};
    }
};

// The link and its routes: ping crosses inbound, pong crosses outbound —
// the SAME declaration covers both directions of each message. Static, in
// declaration order (routes after the link they hang on).
bus::bridge<512> link;
bus::bridge_route<ping_wire> ping_route{link};
bus::bridge_route<pong_wire> pong_route{link};

// The service's inbox. It subscribes to the TOPIC — whether a ping came
// from this image or across the wire is invisible here, which is the point.
bus::subscriber<ping, 4> pings;

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = 115'200});
    uart.write("alloy bus_bridge ready\r\n");

    std::uint32_t served = 0;
    std::uint8_t staging[bus::wire_max_frame];
    for (;;) {
        // Feed the link whatever the uart has (byte-at-a-time ByteStream
        // floor; a DMA board would hand readable() spans instead).
        std::uint8_t b = 0;
        while (uart.read(b)) {
            (void)link.on_bytes({&b, 1}, alloy::uptime_us());
        }

        // The service: consume pings FROM THE BUS, answer INTO THE BUS.
        ping p{};
        while (pings.try_next(p)) {
            ++served;
            (void)bus::publish(pong{p.token, served});
        }

        // Drain queued frames to the wire.
        for (;;) {
            const auto frame = link.tx_take(staging);
            if (frame.empty()) {
                break;
            }
            for (const std::uint8_t out : frame) {
                uart.write(out);
            }
        }
    }
}
