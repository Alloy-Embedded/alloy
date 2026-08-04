// Minimal I2C driver-conformance check for emulation. A bus SCAN (i2c_scan)
// can't be emulated — probe() is a zero-length write (address-only, NBYTES=0),
// which is a valid side-effect-free ACK test on real STM32 silicon but is not
// implemented by Renode's STM32 I2C model. A REAL, >=1-byte transaction is:
// this firmware writes one register byte to a device at 0x08 and reads one byte
// back, then reports whether both transfers were ACKed. A green run under Renode
// (with a DummyI2CSlave at 0x08) proves the driver's START / address / data /
// ACK / STOP sequence actually talks to a device on the bus — no hardware.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

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
    } else {
        uart.write("i2c: not available on this board\r\n");
    }

    for (;;) {
        alloy::sleep_for(1s);
    }
}
