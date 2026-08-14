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
// The wire contract lives in bus.toml — `alloy gen` renders it into
// <alloy/bus_messages.hpp> (structs + WireBinding-shaped bindings, the
// exact shape phases F1/F2 wrote by hand, so the frames on the wire are
// byte-identical to the hand-written era; this example's Renode leg is the
// witness — it passed unchanged across the migration).
#include <alloy/board.hpp>
#include <alloy/bus_messages.hpp>
#include <alloy/time.hpp>
#include <bus.hpp>

#include <cstdint>

namespace bus = alloy::lib::bus;

namespace {

// The link and its routes: ping crosses inbound, pong crosses outbound —
// the SAME declaration covers both directions of each message. Static, in
// declaration order (routes after the link they hang on).
//
// Named `uplink`, not `link`: at namespace scope on a target whose libc
// headers declare POSIX link(2) — the Xtensa/ESP32 toolchain does — a
// namespace-scope `link` competes with ::link on equal footing and every
// use is ambiguous. A file-local name that collides with a libc function
// is a portability trap, and this example compiles for every board.
bus::bridge<512> uplink;
bus::bridge_route<messages::ping_wire> ping_route{uplink};
bus::bridge_route<messages::pong_wire> pong_route{uplink};

// The service's inbox. It subscribes to the TOPIC — whether a ping came
// from this image or across the wire is invisible here, which is the point.
bus::subscriber<messages::ping, 4> pings;

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
            (void)uplink.on_bytes({&b, 1}, alloy::uptime_us());
        }

        // The service: consume pings FROM THE BUS, answer INTO THE BUS.
        messages::ping p{};
        while (pings.try_next(p)) {
            ++served;
            (void)bus::publish(messages::pong{p.token, served});
        }

        // Drain queued frames to the wire.
        for (;;) {
            const auto frame = uplink.tx_take(staging);
            if (frame.empty()) {
                break;
            }
            for (const std::uint8_t out : frame) {
                uart.write(out);
            }
        }
    }
}
