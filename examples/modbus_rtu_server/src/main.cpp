// Portable Modbus RTU server (slave) — the dma-streams anchor 2.2
// (docs/design/dma-streams.md §2.2). The SAME file compiles for every alloy
// board with a debug_uart role; zero preprocessor conditionals. Unit 17,
// 9600 baud — chosen deliberately: t3.5 at 9600 is 4011 µs, several SysTick
// periods, so under emulation the frame timing does not lean on the
// emulator's sub-millisecond fidelity (the robot leg states the same).
//
// On a board whose board.json assigns `debug_uart.rx` a DMA channel, RX is
// the anchor shape: bytes land in a caller-owned ring BY DMA — no interrupt
// runs per byte and the CPU never reads RDR — the loop sleeps between
// events, and each wake hands everything new to the server's on_bytes()
// whole (`readable()`/`consume()`, the byte-stream discipline). The one
// printed marker line, "modbus: dma ring rx", exists for the emulation leg:
// only THIS path prints it, so a silent fold back to polled RX cannot pass
// a leg that asserts it.
//
// On every other board the same server runs the polled shape (poll() reads
// the uart directly) — the requires-probe on rx_ring() folds the ring path
// away at compile time, exactly like the design's §1 gate promises.
//
// Wake honesty: rx_ring() arms the UART IDLE interrupt as the frame-gap
// wake (first quiet bit time after a byte). Renode's pinned USART model does
// not implement IDLE at all (measured — emit/renode.py states it), so under
// emulation the wakes that drain the ring are SysTick's — any interrupt
// wakes sleep_until_event(), and the t3.5 framing above never depended on
// sub-millisecond wakeup latency in the first place. What the emulation leg
// therefore witnesses is the transport: frames received by DMA ring with no
// per-byte ISR. The IDLE wake itself is silicon-only until then.
//
// The register bank is plain user memory behind the DataModel concept — the
// application owns it and can update it between polls; here registers 0 and
// 1 carry known constants the emulation robot (playing the master with an
// independent CRC implementation) reads back and asserts.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>
#include <alloy/time.hpp>
#include <modbus.hpp>

#include <cstdint>
#include <span>

namespace mb = alloy::lib::modbus;
using namespace alloy::literals;

namespace {
// 256 bytes = the RTU spec's largest ADU; static storage is the ring APIs'
// natural spelling (design §4 — the buffer must outlive the stream).
alloy::dma::ring_storage<std::uint8_t, 256> rxbuf;
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = 9'600});

    mb::holding_bank<64> bank;
    bank.regs[0] = 0x1234;
    bank.regs[1] = 0xABCD;

    mb::rtu_server<decltype(uart), mb::holding_bank<64>, 8> server{
        uart, bank, {.unit = 17, .baud = 9'600}};

    uart.write("alloy modbus_rtu_server ready\r\n");

    // Templated lambda so the rx_ring probe is a dependent expression: on a
    // board with no assigned route (or no RX-DMA/IDLE driver hooks) the ring
    // branch is discarded instead of hard-erroring in a concrete main.
    [&]<class U>(U& u) {
        if constexpr (requires { u.rx_ring(rxbuf); }) {
            // Claims the board-assigned channel, programs circular p2m of
            // bytes, starts it, raises DMAR, arms IDLE — armed BEFORE the
            // marker prints, so a master that talks the moment it sees the
            // marker cannot lose its first byte.
            auto ring = u.rx_ring(rxbuf);
            u.write("modbus: dma ring rx\r\n");
            for (;;) {
                alloy::sleep_until_event();  // IDLE on silicon; any IRQ works
                const std::span<const std::uint8_t> bytes = ring.readable();
                if (!bytes.empty() && server.on_bytes(bytes)) {
                    // on_bytes accepted the whole span (the framer owns
                    // assembly and discards internally) — consume exactly
                    // what was fed, never re-feed.
                    ring.consume(bytes.size());
                }
            }
        } else {
            for (;;) {
                // Bounded work per call: drain what the uart has, dispatch
                // at most one request, answer at most once. Never blocks.
                (void)server.poll();
            }
        }
    }(uart);
}
