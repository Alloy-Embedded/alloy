# modbus

Modbus RTU protocol core for alloy — sans-IO, heap-free, chip-free.

```console
$ alloy lib add modbus
```

## What v0.3.0 is, exactly

The **sans-IO protocol core** — CRC-16 with a compile-time-folded table, PDU
codecs for the eight core function codes (FC01/02/03/04/05/06/0F/10 +
exception responses, all four build/parse quadrants), the spec's t1.5/t3.5
timing arithmetic, and a dual-rule RTU framer (length prediction primary,
t3.5 silence resync) — **plus the blocking `rtu_client` (master)**: any
`alloy::ByteStream` uart, injected microsecond clock, optional RS-485 DE pin
(degrades to a no-op where unwired), full correlation checking (unit,
function, echo), and a pre-request RX drain so a late reply to a timed-out
transaction can never answer the next one. Everything is host-tested under
ASan/UBSan, including regression cases for two wire-reachable framing bugs
found in the reference C implementation this library studied (a permanent
wedge on an oversized length claim, and desync on a torn frame).

```cpp
mb::rtu_client<decltype(uart), /*MaxRegisters=*/8> bus{
    uart, {.baud = 19'200, .response_timeout = 500ms}};
std::array<std::uint16_t, 2> regs{};
if (const auto r = bus.read_holding(/*unit=*/17, 0, regs)) { /* regs valid */ }
```

**And the poll-driven `rtu_server` (slave)**: the register bank is any user
type behind the `DataModel` concept (four spaces, per-item bool access —
false becomes the wire's exception 0x02), `holding_bank<N>` covers the
80% case, and the spec's silences are honored exactly: another unit's
conversation, a CRC-corrupt frame, and every broadcast get no reply —
broadcast writes execute, broadcast reads are dropped whole.

```cpp
mb::holding_bank<64> bank;
bank.regs[0] = 0x1234;
mb::rtu_server<decltype(uart), mb::holding_bank<64>, 8> server{
    uart, bank, {.unit = 17, .baud = 9'600}};
for (;;) { (void)server.poll(); }   // bounded work, never blocks
```

**Not yet here:**

- Modbus TCP (MBAP), ISR-fed RX with per-byte timestamps: deferred

## Honest support claim

Silence detection uses caller-supplied microsecond timestamps
(`alloy::uptime_us()` on a board). At **≤ 19200 baud** t3.5 ≥ 2005 µs and the
1 ms-tick-interpolated clock frames comfortably; **above 19200** the spec
clamps t3.5 to 1750 µs, which still exceeds a tick but leaves less margin —
validated on the bench, not in emulation (Renode's UART models deliver bytes
without inter-character gaps, so silence behaviour is provable only against
the host virtual clock; length-predicted framing, which carries all eight
core FCs, is emulation-provable and is the primary rule).

Feeding the framer from an interrupt handler is NOT supported in v0.1 —
feed/tick/frame/consume share one poll-loop context.

## Shape

| Header | What |
| --- | --- |
| `modbus.hpp` | umbrella |
| `modbus/error.hpp` | one error enum; wire exceptions keep their 0x80-bit encoding |
| `modbus/crc.hpp` | constexpr CRC-16/MODBUS; catalogue vector pinned by `static_assert` |
| `modbus/pdu.hpp` | 8 FC codecs, `request_view`, `expected_adu_length` |
| `modbus/rtu_timing.hpp` | t1.5/t3.5 from baud, spec clamp above 19200 |
| `modbus/rtu_framer.hpp` | `feed(b, now_us)` / `tick(now_us)` / `frame()` / `consume()` |
| `modbus/clock.hpp` | `uptime_clock` — the one place the board timebase is read |
| `modbus/client.hpp` | blocking `rtu_client` (master) |
| `modbus/data_model.hpp` | `DataModel` concept + `holding_bank<N>` |
| `modbus/server.hpp` | poll-driven `rtu_server` (slave) |
