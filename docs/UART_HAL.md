# UART HAL

Covers `alloy::hal::uart::port_handle<Connector>` — extended features added by `close-uart-hal-gaps`.

## Basic usage (existing)

```cpp
#include "hal/uart.hpp"

auto uart = alloy::hal::uart::open<DebugUartConnector>(
    alloy::hal::uart::Config{
        .baud_rate = 115200u,
        .data_bits = 8u,
    });

uart.configure();
uart.enable(true);
uart.write_byte('A');
```

## Hardware flow control (RTS/CTS)

```cpp
// Enable RTS output (CR3.RTSE) + CTS gate (CR3.CTSE) simultaneously.
// Returns NotSupported on peripherals without CR3 (e.g. legacy UART).
const auto r = uart.enable_hardware_flow_control(true);
```

Disable by passing `false`. Both RTSE and CTSE are written atomically.

## RS-485 Driver Enable (DE)

### Enable / disable DE output

```cpp
// DEM: CR3 bit 14 — asserts the DE pin during TX
uart.enable_de(true);

// Configure DE assertion/deassertion timing (in baud-clock sample units)
uart.set_de_assertion_time(5u);    // DEAT: CR1 bits [25:21]
uart.set_de_deassertion_time(3u);  // DEDT: CR1 bits [20:16]
```

### DE polarity

```cpp
// DEP: CR3 bit 15 — 0=active-high (most transceivers), 1=active-low
uart.set_de_polarity(true);   // DE high when asserted (default)
uart.set_de_polarity(false);  // DE low when asserted (inverted)
```

### Modbus RTU example (RS-485, nucleo_g071rb)

```cpp
void send_modbus_frame(std::span<const std::byte> pdu) {
    uart.enable_de(true);
    uart.set_de_polarity(true);           // active-high DE
    uart.set_de_assertion_time(5u);       // ~5 bit-times pre-amble
    uart.set_de_deassertion_time(5u);     // ~5 bit-times post-amble

    for (auto b : pdu) { uart.write_byte(static_cast<std::uint8_t>(b)); }

    // Wait for TC before releasing DE
    while (!uart.tx_complete()) {}
    uart.enable_de(false);
}
```

## Half-duplex mode

```cpp
// HDSEL: CR3 bit 3 — single-wire bidirectional UART
uart.set_half_duplex(true);
```

In half-duplex mode TX and RX share one wire. The TX pin acts as open-drain;
configure the GPIO accordingly.

## LIN mode

```cpp
// LINEN: CR2 bit 14 — enable LIN slave
uart.enable_lin(true);

// Request LIN break transmission (SBKRQ: RQR bit 1)
uart.send_lin_break();

// Poll / ISR
if (uart.lin_break_detected()) {
    uart.clear_lin_break_flag();
    // handle sync + PID bytes
}
```

## Error flag handling

```cpp
// Read all four error flags and clear them in one call.
// ST modern: reads ISR(PE/FE/NE/ORE), writes matching ICR bits.
// ST legacy: reads SR then DR (clears sticky flags).
const alloy::hal::uart::UartErrors errs = uart.read_and_clear_errors();

if (errs.overrun) { /* buffer overrun — data was lost */ }
if (errs.framing) { /* stop bit missing — baud mismatch? */ }
if (errs.noise)   { /* noise detected — check signal quality */ }
if (errs.parity)  { /* parity error */ }

// Convenience: any error at all?
if (errs.any()) { /* log and reset */ }
```

### Error recovery pattern

```cpp
while (true) {
    if (uart.rx_ready()) {
        const std::uint8_t b = uart.read_byte();
        process(b);
    }
    const auto errs = uart.read_and_clear_errors();
    if (errs.overrun) {
        // Flush RX FIFO if present, restart frame sync
        re_sync();
    }
}
```

## Vendor extension points

`cr3_rtse_field`, `cr3_ctse_field`, `cr3_dep_field` are discovered at compile time
via `find_runtime_field_ref_by_register_and_offset` from the peripheral's CR3 register.
No IR patch is required for STM32 targets — the fields are looked up by bit position.

For a new vendor backend:
1. Extend `uart_register_bank_base` with the new fields (default `kInvalidFieldRef`).
2. Add a specialized `uart_register_bank` (or extend an existing one) that populates
   the fields from the vendor's semantic traits.
3. The HAL methods automatically become functional — no HAL source changes required.
