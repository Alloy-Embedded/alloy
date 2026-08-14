# bus — typed zero-heap pub/sub for the slow plane

The topic IS the C++ type: `publish(temp_reading{...})` reaches every live
`subscriber<temp_reading>` in the program. No registry, no string names, no
heap — fan-out is an intrusive per-type list walked under the irq mask, and
every subscriber owns its own fixed queue, so a slow consumer drops **its
own** messages (counted in `missed()`), never a sibling's.

This is the slow plane: events, telemetry, mode and config changes — tens to
hundreds of messages a second. A 20 kHz control loop calls a function; it
does not publish.

```cpp
struct temp_reading { std::int16_t centi_c; std::uint8_t sensor_id; };

// Anywhere (thread or ISR context — the walk is masked):
alloy::lib::bus::publish(temp_reading{2543, 0});

// A coroutine service (one awaiting task per subscriber — a second traps):
alloy::lib::bus::subscriber<temp_reading, 4> temps;   // outlives the task
alloy::async::task display(alloy::async::task_storage<256>&) {
    for (;;) {
        const auto t = co_await temps.next();
        draw(t.centi_c);
    }
}

// Or a superloop, no executor anywhere:
temp_reading t{};
while (temps.try_next(t)) { draw(t.centi_c); }

// Latest-value cell for state (get() sees the NEWEST write, never queues):
alloy::lib::bus::watch<temp_reading> current;
alloy::lib::bus::watch_route<temp_reading> feed{current};  // topic -> cell
```

## Shape

| Header | One sentence |
|---|---|
| `bus.hpp` | Umbrella + the slow-plane doctrine and executor-sizing note |
| `bus/topic.hpp` | `detail::topic<T>` intrusive lists + `publish()` — the fan-out and its exact concurrency story |
| `bus/subscriber.hpp` | `subscriber<T, Depth>` — private FIFO, drop-newest counted, `try_next()` or `co_await next()` |
| `bus/watch.hpp` | `watch<T>` latest-value cell + `watch_route<T>` to feed it from a topic |
| `bus/wire.hpp` | The wire boundary, sans-IO: frame codec, byte-at-a-time RX machine, `WireBinding` — local topics never touch it |
| `bus/bridge.hpp` | `bridge<RingBytes>` + `bridge_route<B>` — topics extended over a byte link, encode-at-publish, structural anti-echo |

## Measured (arm-none-eabi-gcc 14.2.1, `-Os -mcpu=cortex-m0plus`, 8-byte message)

RAM, per object:

| Object | Bytes |
|---|---|
| `subscriber<msg8, 4>` | 72 (node 16 + ring 48 + slot 4 + counter 4) |
| `subscriber<msg8, 8>` | 104 |
| `watch<msg8>` | 16 |
| `watch_route<msg8>` | 20 |

Flash: `publish<T>` walk 48 B; `subscriber<T, Depth>::deliver` 144 B per
(T, Depth) instantiation (weak, shared across TUs); `try_next` 52 B. The
whole 3-subscriber probe TU: 444 B text.

Publish cost under the mask (instruction counts from the disassembly):

- walk overhead: 16 instructions + irq save/restore, then 7 per subscriber
  plus its deliver;
- deliver, queue has room, nobody parked (the common case): ~30
  instructions — the 8-byte copy itself is an `ldmia/stmia` pair;
- deliver, queue full: ~13 instructions (count and leave);
- deliver with a parked task: adds `executor_core::schedule()`, which
  includes a software `__aeabi_uidivmod` on Cortex-M0+ (the executor's
  ready-ring modulo — an executor cost the bus inherits on its worst path).

## The wire (sans-IO — the bridge phase drives it)

A message that crosses a wire needs a declared, stable identity; a local
struct needs nothing. The frame is the house length-prefixed convention
(OTA's shape): `0x7E | type | seq | len u16 LE | payload | crc32 LE`, no
escaping — a `0x7E` inside a body is legal because length is read before
payload, so delimitation never needs a clock (the injected `tick()` exists
only to abandon a stalled half frame). Datagrams are at-most-once: no ack,
no retry, no device timers; `lost()` counts seq gaps as a witness, not a
repair.

A binding names one message on the wire. Write it by hand, or declare the
message in `bus.toml` and let `alloy gen` emit exactly this shape into
`<alloy/bus_messages.hpp>` — the frames are byte-identical either way, so
moving a hand binding into the registry changes authorship, not bytes:

```cpp
struct temp_reading_wire {
    using message = temp_reading;
    static constexpr std::uint16_t id  = 0x0101;  // explicit, never reused
    static constexpr std::uint8_t  ver = 1;
    static constexpr std::size_t  size = 3;
    static void encode(const temp_reading&, std::uint8_t* out) noexcept;
    static temp_reading decode(const std::uint8_t* in) noexcept;
};
```

```toml
# bus.toml — the same thing, declared once for both ends of the link
[messages.temp_reading]
id = 0x0101
fields = [ { name = "centi_c", type = "i16" },
           { name = "sensor_id", type = "u8" } ]
```

`alloy bus validate` enforces the id doctrine (explicit, never
auto-assigned, retired rather than deleted), and `alloy monitor` decodes
these datagrams on the link so a sniffing route reads as
`[bus] temp_reading  seq=12  centi_c=2543` instead of binary.

Measured, same toolchain: `wire_receiver<>` is 172 B of RAM (131 B payload
buffer included); the whole `feed()` machine is 426 B of text plus a 48 B
shared bytewise CRC-32; `encode_datagram<B>` is ~110 B per binding. Frame
overhead on the wire: 12 B per datagram (9 frame + 3 message header).

## The bridge (examples/bus_bridge is the two-board story)

One declaration per forwarded message covers both directions:

```cpp
alloy::lib::bus::bridge<512> link;                       // frames ready to send
alloy::lib::bus::bridge_route<temp_reading_wire> r{link};  // topic <-> wire

// TX task (or a superloop draining tx_take): ONE await for ALL routes,
// because delivery ENCODES into the link's ring at publish time —
// no when_any needed, and cross-topic order is publication order.
co_await link.tx_pending();
const auto frame = link.tx_take(staging);   // staging: caller's DMA-visible RAM

// RX feeder: whoever owns the uart hands bytes in; complete datagrams
// decode through the routes and republish locally, SKIPPING wire nodes —
// a message cannot echo back out its own link or hop onward (single-hop
// is structure, not configuration).
link.on_bytes(bytes, alloy::uptime_us());
```

### What the wire costs, measured on silicon

Measured on a SAME70 Xplained running `examples/bus_bridge`, ping→pong
round trips through the bus:

| | 115200 | 230400 |
|---|---|---|
| Round-trip latency | 3.4 ms | 1.9 ms |
| Sustained, no loss | ~290/s | ~400/s |
| Wire's own ceiling | 320/s | 640/s |

The latency is the WIRE, not the software: a ping plus a pong is 36 bytes,
which is 3.1 ms of airtime at 115200 — the firmware adds about 0.3 ms on
top. So the knob that moves this number is the baud rate
(`[roles.debug_uart] baud` in `alloy.toml`), not the code.

**Take interrupt RX if the board has it.** Writing a byte busy-waits on the
TX-ready flag, and these parts hold ONE received byte, so a purely polled
loop is deaf for the whole length of a transmission. Same board, same wire,
20 messages sent back to back: **polled delivers 7, interrupt RX delivers
20.** The example arms it behind a `requires` probe and falls back to
polling, so this costs portability nothing.

Past the wire's ceiling, messages are lost — and the counters name exactly
where, which is the point of witnesses over repairs. A 40-message burst at
230400, well beyond what the link can answer:

```
bus: served=27 bad_frames=0 lost=13 sub_missed=0 tx_missed=0 rx_overflow=0
```

Nothing corrupted, no queue full, no ring full: the library dropped none of
them. `lost=13` is the seq gaps catching every message the UART overran
before it was ever a frame.

Two ceilings above this one are known and unmeasured. A board whose
`debug_uart.rx` has a DMA route removes the deafness entirely (the ring
receives while the CPU transmits). And `publish()` holds the irq mask
across the frame encode, CRC included — on a fast link that window is
itself a limit, which is the risk the design named from the start.

### The rest of the contract

The ring accepts a frame whole or not at all (`tx_missed()` is the witness);
unknown ids (`rx_unknown()`) and stale layouts (`rx_dropped()`) are counted,
never guessed at. Dropped-at-source frames still consume seq, so the peer's
`lost()` sees them. Measured, same toolchain: `bridge<512>` is 728 B of RAM
(512 ring + receiver), a route is 36 B, and the whole bridge machinery
(send + take + on_bytes + one binding) is ~1.6 KB of text. The complete
`bus_bridge` example — bridge, two routes, service, superloop — is 4.1 KB of
text on the G0, and its Renode leg plays a peer with an independently
written CRC-32 in monitor Python.

## The rules that carry the design

- **Drop-newest only, counted.** The sanctioned SPSC `ring_buffer` gives the
  producer one index and the consumer the other; drop-oldest would need a
  producer-side pop and is therefore an RFC on `ring_buffer`, not a policy
  flag here. Same answer `async::uart_reader` gives: a full queue drops like
  a HW overrun, and `missed()` is the witness.
- **One awaiting task per subscriber** (`waiter_slot` traps a second parker).
  A shared feed wants one owning task that fans out — or two subscribers.
- **Construct subscribers outside the coroutine**, in storage that outlives
  every task awaiting them; destructors unlink from the topic.
- **Publish can wake N tasks; the executor's ready queue traps on overflow.**
  Count a publish burst when sizing `executor<MaxReady>`.
- **Single-core by contract.** `irq_save` masks this core only. Crossing to
  another board is the bridge, above; crossing to another CORE is neither —
  that needs a transport this library does not have yet.

The full guide, including the wire contract and the monitor, is
`docs/guide/bus.md` in the alloy repository.
