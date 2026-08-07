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
#include <alloy/board.hpp>

#include <cstdint>
#include <span>

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
    } else {
        uart.write("i2c: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
