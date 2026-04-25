# Add Modbus Protocol Library

## Why
Most Alloy users want their equipment to talk to other equipment and to a PC at runtime
-- read sensor values, push setpoints, watch state. Today there is no first-class answer:
the user either rolls their own UART protocol, vendor-locks into someone else's stack, or
copies bits of a Modbus implementation from another repo and edits them per project.

Modbus is the pragmatic default for this scope:

- Industrial-ubiquitous: Node-RED, pymodbus, Home Assistant, and dozens of HMIs speak it
  out of the box, so the PC side is "use what already exists".
- Maps cleanly to "expose a runtime variable": registers and coils are exactly that.
- Three transports cover the realistic fleet today (RS-485 RTU) and tomorrow
  (Modbus/TCP over Ethernet/WiFi).

The maintainer's stated scope is concrete: ~100 variables per device, mixed read/write,
polling at 10-100 ms, RS-485 today, every transport eventually. That fits Modbus
comfortably; it does not justify CAN, MQTT, OPC UA, or a custom RPC.

This change introduces a Modbus library inside the runtime that any Alloy-targeted MCU
can use. It is built so that the next protocol the project picks up (MQTT, CoAP, etc.)
slots in without rewriting application code: the variable model is the contract;
Modbus is one of many adapters.

## What Changes

### A protocol library at `drivers/protocol/modbus/`
Lives next to the other drivers. Three layers:

1. **PDU + framing** -- pure encode/decode of Modbus PDUs (function codes, payload
   parsers/builders) and RTU frame wrapping (CRC-16 with the standard 0xA001 polynomial,
   inter-frame silence rules). No I/O. Hostable, fuzzable.
2. **`byte_stream` transport** -- a small interface the framers consume. Implementations
   plug a UART, a TCP socket (when the network HAL lands), USB-CDC, or any I2C/SPI-to-UART
   bridge. Modbus is **not** layered onto I2C or SPI as a wire protocol -- doing so would
   break interop with the very PC tools that motivate the choice of Modbus.
3. **Variable registry + master/slave** -- a typed `alloy::modbus::var<T>` carries a
   register address, an access policy, and an optional metadata block. The slave server
   exposes vars; the master client polls and mirrors vars. The same `var<T>` works on
   both sides.

### Core protocol coverage
- **Slave server**: cooperative `slave.poll(timeout)` plus a `slave.run()` helper that
  is the trivial loop on top. Standard function codes 0x01/0x02/0x03/0x04 (read coils
  / discrete inputs / holding regs / input regs), 0x05/0x06 (write single coil / reg),
  0x0F/0x10 (write multiple), and 0x17 (read/write multiple regs). Exception responses
  per Modbus spec.
- **Master client**: cooperative scheduling. `cli.poll(var, slave_id, rate)` schedules
  periodic reads; `cli.write(var)` queues writes; `cli.poll_once()` advances the
  scheduler one tick. Mirrors the slave's variable contract: the master writes through
  the same `var<T>` instance.
- **CRC-16**: precomputed table (~1 KB ROM) for both encode and verify.
- **Word ordering**: per-var configurable across all four orders (ABCD/CDAB/BADC/DCBA);
  default ABCD.

### Custom discovery function code (alloy extension, not stock Modbus)
- Function code 0x65 (in Modbus's user-defined range 0x65-0x72) returns a compact table
  describing the slave's exposed vars: register address, type id, access flags, name.
  A second sub-function returns rich metadata (range, unit, description) when the var
  declared it. PC tools that ignore 0x65 still work; tools that understand it auto-build
  dashboards.
- The `var<T>` carries name as a `string_view` to a flash literal (zero RAM cost). Rich
  metadata is opt-in per var, so a 100-var slave that declares everything thin pays
  ~1.2 KB flash for the registry; declaring everything rich is ~4 KB.

### RS-485 helper
Half-duplex DE/RE pin control via a `rs485_de` adapter on top of `byte_stream`. The
adapter latches DE high before TX, calls the underlying stream, waits for the UART's
TC (transmission complete) bit through the existing HAL `flush()` path, and drops DE.
No HAL changes required.

### TCP framing scaffolded behind a feature flag
The MBAP header (transaction id + length, no CRC) is defined in `tcp_frame.hpp` from
day one but is not enabled by default. When `alloy::hal::tcp::socket` lands, the only
work is implementing a `tcp_stream` adapter; the framing and the slave/master sides
are already there.

### Examples and tests
Three runnable examples (`modbus_slave_basic`, `modbus_slave_rich`,
`modbus_master_poll`) plus host-only tests covering PDU codec, RTU framing, master/slave
loopback, word-order conversion, and discovery FC. Hardware validation gates ride on
existing board CI when the user's RS-485 transceiver lands.

## Impact

- Affected specs:
  - `protocol-modbus` (new capability spec; created in this change)

- Affected code:
  - new tree: `drivers/protocol/modbus/include/alloy/modbus/`,
    `drivers/protocol/modbus/src/`, `drivers/protocol/modbus/tests/`
  - new examples: `examples/modbus_slave_basic/`, `examples/modbus_slave_rich/`,
    `examples/modbus_master_poll/`
  - documentation: `docs/MODBUS.md` user guide; `docs/SUPPORT_MATRIX.md` adds a
    `modbus` peripheral-class entry tracked at `representative`/`compile-only` until
    hardware spot-checks land.

- Hard prerequisites:
  - none. The library depends only on existing public HAL surfaces (UART read/write,
    flush, GPIO toggle for DE pin) and the runtime's `core::Result` / typed time
    primitives. No alloy-devices changes required.

- Out of scope for this change:
  - Modbus ASCII (rare; not on any roadmap the maintainer cares about).
  - TCP transport implementation (requires `alloy::hal::tcp` HAL surface that does not
    yet exist; framing is scaffolded but unused until that lands).
  - Vendor-specific quirks (Schneider extended exception codes, AB-style register
    offsets). The lib stays standard-compliant and exposes hooks for users who need
    quirks in their own code.
  - Modbus gateway features (forwarding between transports). Would build naturally on
    the master+slave primitives but is its own change.
  - Hardware idle-line / RX-timeout HAL extension. The library uses a software timer
    against SysTick to detect inter-frame silence, which is sufficient at <=115200 baud
    on the foundational boards. Higher baud (>=921600) on noisy lines would benefit
    from a hardware idle IRQ; that is a separate `runtime-tooling`/HAL extension.
