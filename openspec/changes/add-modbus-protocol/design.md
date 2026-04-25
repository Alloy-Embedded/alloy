# Design: Modbus Protocol Library

## Context
Modbus is a 1979 protocol that survived because it is small enough to implement in a
weekend and standardised enough that every PC-side tool already speaks it. Building it
inside the runtime means the maintainer's equipment-to-equipment and equipment-to-PC
story stops being "roll your own" and starts being "include this header, declare your
variables, register them with a slave instance".

The library has three jobs that pull in different directions:

- **Be a faithful Modbus implementation.** A non-compliant slave breaks the very tools
  that justify the choice of Modbus. The wire bytes, the function codes, and the
  exception responses must match the standard byte for byte.
- **Be the seed of a protocol-agnostic variable registry.** The `var<T>` model is
  designed so the next protocol (MQTT, CoAP, custom RPC) reuses the same application
  surface. Done badly, this leaks Modbus assumptions into the registry. Done well, the
  application code does not care which protocol is running.
- **Stay inside the Alloy architecture.** No vendor SDKs, no FreeRTOS dependency, no
  pthreads. Cooperative scheduling, host-testable, footprint sized for a 64 KB
  STM32G071RB.

## Goals / Non-Goals

Goals:
- Modbus RTU master and slave that pass standard PC client probes (function codes
  0x01-0x06, 0x0F-0x10, 0x17).
- Single library, three layers, each unit-testable in isolation on the host.
- Variable model that survives a future MQTT or custom-RPC adapter without rewriting
  application code.
- Footprint <=8 KB code + <=4 KB RAM for a 100-variable slave on a Cortex-M0+.
- Cooperative scheduling: no thread, no RTOS dependency. The application owns the loop.

Non-Goals:
- Modbus ASCII. Rare, no value here.
- Modbus over I2C or SPI as a wire protocol. Not standard; loses interop. Bytes can
  flow through I2C/SPI bridge chips that present a UART interface; that is a transport
  question, not a framing question.
- Vendor quirks. The library implements the published spec; quirks are user code.
- Self-hosted gateway features. Reasonable to build on top later; not in v1.
- A scheduler. The library exposes `poll(timeout)` and `run()`; if the user wants a
  thread, they wrap it.

## Decisions

### Decision: Three layers with a `byte_stream` waist
The library shape is `pdu+framing -> byte_stream -> master/slave + var registry`. The
waist matters: framing is pure logic, the stream knows nothing about Modbus, and the
master/slave know nothing about UART or TCP. The waist is what makes future protocols
(MQTT) reuse the var registry without dragging Modbus framing along.

### Decision: Cooperative scheduling, with a `run()` helper for the trivial case
`slave.poll(timeout)` and `master.poll_once()` are the contract. The application owns
the loop. A header-only `slave.run()` helper is provided for the canonical
"`while(true) { srv.poll(timeout); }`" case that 80% of users will write.
- Rationale: no RTOS dependency; works on the smallest Cortex-M0; matches Alloy's
  `blocking-only-path` discipline.
- Trade-off: applications that want a background thread for Modbus must wrap `run()`
  themselves. Acceptable; that is the same shape Alloy uses everywhere.

### Decision: `alloy::modbus::var<T>` with optional metadata
Minimum form is `var<T>{address, name}` -- ~12 bytes per var on 32-bit. Rich form is
`var<T>{address, name, {.access, .range, .unit, .desc}}` -- ~40 bytes per var.
Per-var, so the user controls the footprint trade.
- Rationale: the user explicitly asked for "powerful but bounded footprint". Per-var
  opt-in is the only way to honour both.
- Trade-off: two constructor forms to maintain. Cheap.

### Decision: Custom discovery on FC 0x65
PC tools that learn FC 0x65 auto-discover the slave's vars (address, type, name,
optional metadata). PC tools that do not understand it ignore exceptions and continue
working. FC 0x65 is in Modbus's user-defined range 0x65-0x72; we are not stepping on
the standard.
- Trade-off: it is an Alloy invention. We document it and ship a Python reference
  client so users can validate. Future extension paths (e.g. Modbus device profiles,
  the EtherNet/IP Modbus profile) remain open.

### Decision: Software timer for inter-frame silence
The Modbus RTU spec requires 3.5 character times of silence between frames. We measure
this with a SysTick-based timestamp on the last received byte, and gate frame parsing
on `now - last_byte >= silence_budget`. At 115200 baud, the silence budget is ~300 us;
SysTick at 1 ms tick rate is too coarse, so the implementation reads a free-running
microsecond counter exposed by the HAL.
- Rationale: not every supported MCU exposes a UART idle-line IRQ. The software
  fallback works on all foundational boards.
- Trade-off: jitter is bounded by the application's `poll()` cadence. At 10 ms polling
  the slave can detect silence to ~10 ms granularity, which is fine for 9600/19200
  baud. A hardware idle-line IRQ becomes worthwhile at >=921600 baud or in noisy
  environments and is tracked as a future HAL extension.

### Decision: CRC-16 with a precomputed 256-entry table in flash
~1 KB ROM, two integer ops per byte. The alternatives -- bit-by-bit CRC (~5x slower)
or a 16-entry nibble table (~2x slower, ~64 bytes) -- are not worth the saving on the
target footprint.

### Decision: Word order configurable, default ABCD (big-endian network order)
Modbus is big-endian on the wire. Multi-register types (`int32_t`, `float`) introduce
the word-order question: is register N the upper or lower half? The four orders
(ABCD/CDAB/BADC/DCBA) correspond to the four conventions in the wild. Default ABCD
matches IEEE 754 native big-endian and most modern PLCs; per-var override covers the
"talking to a Schneider PLC" case.

### Decision: RS-485 DE/RE control via a `byte_stream` decorator
A `rs485_de_stream` wraps any underlying stream, drives the DE pin high before write,
calls `underlying.write(...)`, calls `underlying.flush()` (which waits on the HAL's
TC bit), and drops the pin. Any subsequent read clears RE if needed. The decorator is
optional: a TCP stream uses the bare interface.
- Trade-off: the user wires the GPIO once. The decorator does not try to auto-detect
  anything.

## Risks / Trade-offs

- **Tearing on multi-register reads.** A master polling `var<int32_t>` mid-write on
  the slave can read half-old, half-new bytes. The slave-side write of an int32 happens
  inside the function-code handler, which holds a critical section across all
  affected registers; the master-side read is a single FC 0x03 with N registers, so
  the slave responds atomically per request. The remaining tearing window exists when
  the master's mirror is updated by the response while application code reads it; we
  guard the mirror's update with the same critical section the slave uses.
- **Software-timer silence accuracy.** As above, fine to ~115200 baud at 1 kHz polling
  cadence. Documented as a known limitation; HAL idle-line IRQ tracked separately.
- **FC 0x65 conflicts.** Highly unlikely (the range is reserved for user code), but
  possible if a user already deployed FC 0x65 for something else. Mitigation: the
  default function code is configurable at slave-construction time; users with prior
  art can pick 0x66 or 0x67.
- **Variable footprint surprise.** Users who declare 100 rich vars pay ~4 KB flash
  before they realise. Mitigation: documentation, plus a compile-time
  `static_assert` warning when the registry payload exceeds a configurable cap.
- **Cooperative `run()` hides starvation.** A user who calls `srv.run()` from main()
  freezes the rest of their application. Mitigation: docs lead with `poll(timeout)`;
  `run()` is shown only as a "smallest possible main" example.

## Migration Plan
This is additive. No existing capability is touched. The library lands behind no
feature flag; users opt in by including the header. Examples and a docs page guide
new users.

## Open Questions

- **Should the master expose a callback for "stale data detected" (poll deadline
  missed)?** Useful for user code that wants to react to a remote slave going silent.
  Adds a callback per polled var. Lean: yes, but as an optional setter, not a
  constructor argument.
- **Should the discovery FC return register-write history?** No. That is a debug
  feature and would balloon the slave's RAM. If users want telemetry, they expose a
  separate var.
- **Should the variable registry support array vars (`var<int16_t[8]>`) for
  contiguous register blocks?** Yes, but as a follow-up. v1 keeps scalar `var<T>`
  for clarity; arrays compile in a future change without breaking the scalar API.
