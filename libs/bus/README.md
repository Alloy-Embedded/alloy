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
repair. Bindings are hand-written today in the exact shape the `bus.toml`
generator will emit later:

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

Measured, same toolchain: `wire_receiver<>` is 172 B of RAM (131 B payload
buffer included); the whole `feed()` machine is 426 B of text plus a 48 B
shared bytewise CRC-32; `encode_datagram<B>` is ~110 B per binding. Frame
overhead on the wire: 12 B per datagram (9 frame + 3 message header).

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
- **Single-core by contract.** `irq_save` masks this core only; a cross-core
  or cross-board transport is the bridge phase (see `BUS_PROPOSTA.md` at the
  workspace root), which adds declared wire IDs — locally, none of that
  exists on purpose.
