// Minimal I2C driver-conformance check for emulation. A bus SCAN (i2c_scan)
// can't be emulated — probe() is a zero-length write (address-only, NBYTES=0),
// which is a valid side-effect-free ACK test on real STM32 silicon but is not
// implemented by Renode's STM32 I2C model. A REAL, >=1-byte transaction is:
// this firmware writes one register byte to a device at 0x08 and reads one byte
// back, then reports whether both transfers were ACKed. A green run under Renode
// (with a DummyI2CSlave at 0x08) proves the driver's START / address / data /
// ACK / STOP sequence actually talks to a device on the bus — no hardware.
//
// A second leg then repeats the traffic INTERRUPT-DRIVEN: the CPU programs the
// transfer and stops touching the bus, and the driver's ISR moves every byte
// and calls back on STOP. The test slave echoes what it was written, so the
// bytes that come back can only be the ones the write ISR sent.
//
// A third leg takes the CPU out of the DATA phase as well, via the board's
// `i2c.rx` DMA assignment (docs/design/dma-streams.md §6, phase 4): the address
// phase stays CPU-written, the engine moves every payload byte, and the CPU's
// only job is to wait for the AUTOEND STOP. HONEST LABEL, printed by the leg
// itself: this is a COMPILE-and-silicon leg, not an emulation one. Renode
// 1.16.1's model of this IP exposes no DMA request line at all (the upstream
// commit that adds one postdates the pinned release), so under emulation the
// engine is never asked to move anything and the leg reports its bounded
// timeout rather than pretending. It goes green on hardware, or on a Renode
// that models the request.
//
// MEASURED, so whoever attaches the board's `i2c.rx` tag to the binder is not
// surprised (nucleo_g071rb, pinned Renode 1.16.1, the emitter patched locally
// to attach the tag): with the leg LIVE the firmware prints
// "i2c dma: no transfer" and the bounded wait costs ~8 s of emulated time.
// i2c_read.robot is unaffected — its last assertion is "i2c irq: fired", which
// this leg prints AFTER, so the run still finishes in 2.3 s (2.5 s with the leg
// inactive). A robot that DID wait for the dma line took 10.7 s. Cost in flash:
// +444 bytes of text on that board.
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

#include <cstdint>
#include <span>
#include <type_traits>

using namespace alloy::literals;

namespace {
template <class Uart>
void write_hex_byte(const Uart& uart, std::uint8_t value) {
    constexpr char digits[] = "0123456789abcdef";
    uart.write(static_cast<std::uint8_t>(digits[value >> 4]));
    uart.write(static_cast<std::uint8_t>(digits[value & 0xF]));
}

// Set from the I2C completion INTERRUPT. Seeing it true after the transfer is
// proof the NVIC line fired and the driver's handler ran — not merely that the
// bytes moved, which the polled leg above already shows.
volatile bool g_i2c_irq_fired = false;
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy i2c_read\r\n");

    if constexpr (board::caps::i2c) {
        auto bus = board::i2c::open({.speed_hz = 100'000});

        constexpr std::uint8_t kAddr = 0x08;
        std::uint8_t reg = 0x00;           // register pointer
        std::uint8_t val = 0xFF;           // receives the byte the device returns
        const bool wrote = bus.write(kAddr, {&reg, 1});
        const bool read = bus.read(kAddr, {&val, 1});

        // "acked" is the conformance signal: both a 1-byte write and a 1-byte
        // read completed with the slave acknowledging. The value itself is
        // whatever the mock returns and is not asserted on.
        uart.write(wrote && read ? "i2c: device acked\r\n" : "i2c: no ack\r\n");

        // Second leg: the SAME kind of traffic, interrupt-driven. The CPU
        // programs the transfer and stops touching the bus; the driver's ISR
        // moves every byte and calls back on STOP. Three bytes are written and
        // three read back, so the printed pattern only appears if the handler
        // ran on both directions. The templated lambda makes `B` dependent, so
        // the `requires` below removes the leg on a board whose I2C driver has
        // no interrupt hook instead of hard-erroring in a concrete main.
        [&uart]<class B>(B& b) {
            if constexpr (requires {
                              b.write_async(0u, std::span<const std::uint8_t>{});
                          }) {
                static const std::uint8_t out[] = {0xDE, 0xAD, 0xBE};
                std::uint8_t in[3] = {};
                bool ok = true;
                b.write_async(kAddr, out, +[](void* flag) {
                    *static_cast<volatile bool*>(flag) = true;
                }, const_cast<bool*>(&g_i2c_irq_fired));
                if (!b.wait_transfer()) {
                    uart.write("i2c async: timeout\r\n");
                    b.detach_transfer();
                    ok = false;
                }
                ok = ok && b.transfer_ok();
                if (ok) {
                    b.read_async(kAddr, in);
                    if (!b.wait_transfer()) {
                        uart.write("i2c async: timeout\r\n");
                        b.detach_transfer();
                        ok = false;
                    }
                    ok = ok && b.transfer_ok();
                }
                if (!ok) {
                    uart.write("i2c async: no ack\r\n");
                }
                uart.write("i2c async: 0x");
                for (std::uint8_t byte : in) {
                    write_hex_byte(uart, byte);
                }
                uart.write("\r\n");
                uart.write(g_i2c_irq_fired ? "i2c irq: fired\r\n" : "i2c irq: NOT fired\r\n");
            } else {
                uart.write("i2c irq: not available on this board\r\n");
            }
        }(bus);

        // Third leg: the same 3-byte read with the CPU out of the DATA phase.
        // Gated TWICE and both gates are dependent names, so a board that
        // assigned no `i2c.rx` route — or a port whose driver has no DMA hooks
        // — removes the leg instead of failing the build. The route is probed
        // through the handle's `rx_route` alias, never through
        // `board::dma::i2c_rx`: a missing namespace member is a hard error, not
        // a substitution failure, so it would not fold here.
        [&uart]<class B>(B& b) {
            if constexpr (!requires { typename B::rx_route; }) {
                // A board with no I2C at all gets a null handle, which has no
                // route alias to ask about.
                uart.write("i2c dma: not available on this board\r\n");
            } else if constexpr (!std::is_void_v<typename B::rx_route>) {
                if constexpr (requires(B& bb, std::span<std::uint8_t> d) {
                                  bb.read_dma(alloy::dma::claim(typename B::rx_route{}),
                                              std::uint8_t{}, d);
                              }) {
                    // Claimed ONCE, held for the transfer's lifetime and after
                    // it: a one-shot borrows the channel, it does not own the
                    // claim (only streams release).
                    auto rx = alloy::dma::claim(typename B::rx_route{});
                    std::uint8_t in[3] = {};
                    if (b.read_dma(rx, kAddr, in)) {
                        uart.write("i2c dma: 0x");
                        for (std::uint8_t byte : in) {
                            write_hex_byte(uart, byte);
                        }
                        uart.write("\r\n");
                    } else {
                        // The bounded wait, doing its job: no request line, no
                        // bytes, an honest false — not a hang.
                        uart.write("i2c dma: no transfer\r\n");
                    }
                } else {
                    uart.write("i2c dma: driver has no hooks\r\n");
                }
            } else {
                uart.write("i2c dma: not assigned\r\n");
            }
        }(bus);
    } else {
        uart.write("i2c: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
