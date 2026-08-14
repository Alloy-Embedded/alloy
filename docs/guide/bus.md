# The bus — services talking in messages

`libs/bus` brings the part of microservices that survives contact with a
Cortex-M0: a service is a task that owns its state; the contract is the
message type; coupling is publish/subscribe, not cross-calls; and every
service tests on the host by injecting messages. The parts that do not
survive (HTTP, JSON, discovery, per-service deploy) are refused, not
approximated.

It is the **slow plane** — events, telemetry, mode and config changes, tens
to hundreds of messages a second. A 20 kHz control loop calls a function; it
does not publish.

## Local: the type is the topic

```cpp
#include <bus.hpp>   // alloy lib add bus

struct temp_reading { std::int16_t centi_c; std::uint8_t sensor_id; };

// Anywhere — thread or ISR context:
alloy::lib::bus::publish(temp_reading{2543, 0});

// A coroutine service:
alloy::lib::bus::subscriber<temp_reading, 4> temps;   // outlives the task
const auto t = co_await temps.next();

// Or a superloop, no executor anywhere:
temp_reading t{};
while (temps.try_next(t)) { /* ... */ }

// Latest-value state (never a queue — get() sees the NEWEST write):
alloy::lib::bus::watch<temp_reading> current;
alloy::lib::bus::watch_route<temp_reading> feed{current};
```

No registry, no IDs, no configuration: local messages are plain structs and
the fan-out is resolved from the type. Each subscriber owns its own fixed
queue, so a slow consumer drops **its own** messages — drop-newest, counted
in `missed()`, like a hardware overrun. One task awaits one subscriber; a
second traps.

## Crossing a wire: declare it in `bus.toml`

The line the library draws: local is zero-config; a message that crosses to
another board needs a declared, stable identity. That declaration is
`bus.toml`, beside `alloy.toml` — and both ends of the link compile from
the same file, copied verbatim and reviewed like the contract it is.

```toml
schema = "alloy.bus.v1"

[messages.temp_reading]
id = 0x0101          # explicit u16 — never auto-assigned
version = 1
fields = [
  { name = "centi_c",   type = "i16" },
  { name = "sensor_id", type = "u8"  },
]

[messages.old_reading]
id = 0x0100
retired = true       # a dead id is a tombstone, never deleted or reused
```

`alloy gen` renders it into `<alloy/bus_messages.hpp>`: the structs plus
`WireBinding`-shaped codecs (`temp_reading_wire`), little-endian via
`alloy::byteorder`, in exactly the shape a hand-written binding has — so
frames are byte-identical whether the binding was written or generated.

The id rules are the nvm-key rules, because the failure mode is identical —
reordering a table must never silently renumber what is deployed in the
field: ids are explicit, `0x0000` and `0xFF00..` are reserved, a layout
change is a **new id**, and `version` is a runtime guard against a stale
peer, not a license to change bytes under an id.

## The bridge

```cpp
#include <alloy/bus_messages.hpp>

alloy::lib::bus::bridge<512> link;
alloy::lib::bus::bridge_route<messages::temp_reading_wire> r{link};
```

One route per forwarded message covers both directions. Publishing encodes
straight into the link's frame ring (so one TX task awaits one
`tx_pending()` for any number of routes), and frames arriving from the wire
republish on the local bus — skipping wire nodes, so a message cannot echo
back out or hop through a second bridge. The transport is yours: feed
`link.on_bytes(...)` from whatever owns the uart, drain `link.tx_take(...)`
to it. `examples/bus_bridge` is the complete two-board story, proven under
Renode against a peer implemented independently in the test itself.

Datagrams are at-most-once: no ack, no retry, no timers. Every failure mode
has a counter — `tx_missed()`, `rx_unknown()`, `rx_dropped()`, `lost()`,
`bad_frames()` — witnesses, not repairs. Reliability, where a message needs
it, belongs to an application protocol above the bus (exactly as the OTA
transport already does for itself).

## The CLI

```
alloy bus validate      # every problem at once, located (--json: alloy.bus_validate.v1)
alloy bus list          # the registry as a table
alloy bus manifest      # alloy.bus_manifest.v1 — feeds the IDE monitor's decode
```

Validation runs again at generation: a bad `bus.toml` fails `alloy gen`
with every problem listed, never the compile.

## Watching the bus

A central bus makes a sniffer nearly free: a `bridge_route` pointed at the
debug uart taps whichever topics you choose to forward, and the host end
turns those bytes back into named messages.

```
alloy monitor
...
alloy bus_bridge ready
[bus] reading  seq=12  centi_c=2543  ok=true
```

Decoding happens in the CLI, against the same `bus.toml` the firmware
compiled from — automatically, whenever the project has one. Bytes that are
not a complete, CRC-valid frame pass through as ordinary log text, so a
`~` in a log line never costs you the line; and an id the manifest cannot
name (or a body whose layout disagrees) degrades to hex with a note saying
what was expected, because a monitor that went blank when the two ends
disagree would fail at the one moment it matters.

In the VS Code monitor panel the same messages arrive on the same timeline
as the log, marked apart from printf output. Since fields render as
`name=value`, numeric telemetry flows into the panel's sparklines with no
extra plumbing, and the filter box works on messages like any other line.

`alloy bus manifest` remains the machine-readable registry for other tools;
the panel does not need it, because what reaches the editor is already
decoded.

## Costs

Measured numbers — RAM per subscriber, instructions per delivery, bridge
and receiver footprints — live in `libs/bus/README.md` and are re-measured
as part of the library's acceptance gates. The wire overhead is 12 bytes
per datagram; bodies are capped at 128 bytes to bound the work done under
the publish mask.
