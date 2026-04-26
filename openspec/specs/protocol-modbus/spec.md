# protocol-modbus Specification

## Purpose
TBD - created by archiving change add-modbus-protocol. Update Purpose after archive.
## Requirements
### Requirement: The Library Shall Implement Modbus PDU Encoding And Decoding

The library SHALL encode and decode Modbus PDUs for the standard function codes 0x01,
0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10, and 0x17, plus exception responses, byte
for byte against the Modbus Application Protocol Specification.

#### Scenario: Round-trip a read holding registers PDU
- **WHEN** the library encodes a request for FC 0x03 from address 0x0010 reading 4
  registers, then decodes the resulting bytes
- **THEN** the decoded request matches the input byte for byte
- **AND** the decoded request structure exposes the start address, count, and FC
  through a typed accessor

#### Scenario: Decode a malformed PDU
- **WHEN** the library is asked to decode a PDU whose declared length does not match
  its byte count, or whose function code is outside the implemented set
- **THEN** the decoder returns a typed error
- **AND** no partial PDU is exposed to the caller

### Requirement: The Library Shall Frame RTU Traffic With CRC And Inter-Frame Silence

The library SHALL wrap PDUs in Modbus RTU frames using the standard CRC-16 with
polynomial 0xA001 and SHALL detect frame boundaries through inter-frame silence
measured against a runtime-provided microsecond timestamp.

#### Scenario: Compute and verify a frame CRC
- **WHEN** the library frames a PDU into RTU bytes
- **THEN** the trailing two CRC bytes match the value computed by an independent
  Modbus reference implementation for the same payload
- **AND** the verifier rejects a frame whose CRC has been corrupted

#### Scenario: Detect a frame boundary on silence
- **WHEN** the library is fed bytes whose inter-byte gap reaches the configured
  silence threshold for the current baud rate
- **THEN** the framer treats the previously buffered bytes as a complete frame and
  exposes them for parsing
- **AND** subsequent bytes start a new frame

### Requirement: The Library Shall Provide A Transport-Agnostic Byte Stream Interface

The library SHALL define a `byte_stream` interface that the framers and the
master/slave implementations consume, and SHALL ship a UART adapter, an in-memory
loopback adapter for tests, and a documented contract for future TCP, USB-CDC, or
bridged transports.

#### Scenario: Run RTU over a host loopback
- **WHEN** a slave and a master instance both consume the in-memory loopback stream
  during a host test
- **THEN** the master can read and write the slave's variables exactly as it would
  over a real UART
- **AND** no part of the slave or master implementation references UART or TCP
  symbols directly

#### Scenario: Plug a new transport without touching the protocol layer
- **WHEN** a future change adds a TCP stream adapter
- **THEN** that adapter implements the same `byte_stream` interface and the
  master/slave implementations work unchanged
- **AND** the framing layer's TCP MBAP wrapper is the only Modbus-specific change

### Requirement: The Library Shall Provide A Typed Variable Registry With Optional Metadata

The library SHALL provide a `var<T>` template that carries a Modbus register address,
an access policy, a symbolic name, and optional metadata (range, unit, description).
Variables SHALL support the integer types `int16_t`, `uint16_t`, `int32_t`, `uint32_t`,
the floating-point types `float` and `double`, and `bool`. The metadata block SHALL be
opt-in per variable so that lean and rich declarations coexist in the same registry.

#### Scenario: Expose a thin variable
- **WHEN** application code declares `var<float>{0x100, "voltage"}` and registers it
  on a slave
- **THEN** the slave responds to FC 0x03 / 0x04 reads at register 0x100 with the
  current value of `voltage`
- **AND** the registry footprint for that variable does not exceed 16 bytes on a
  32-bit MCU

#### Scenario: Expose a rich variable
- **WHEN** application code declares a `var<int32_t>` with `range`, `unit`, and
  `desc` populated
- **THEN** the slave's discovery response advertises that metadata
- **AND** thin variables in the same registry continue to compile and report only
  their thin metadata

### Requirement: The Library Shall Provide A Slave Server With Cooperative Scheduling

The slave server SHALL drive its work through `slave.poll(timeout)` so that the
application owns the loop and SHALL provide a `slave.run()` helper for the trivial
case. The slave SHALL respond to standard function codes against the variable
registry and SHALL emit standard exception responses for out-of-range addresses,
disallowed access, or malformed requests.

#### Scenario: Application drives the loop
- **WHEN** the application calls `slave.poll(10ms)` from its main loop
- **THEN** the slave processes any pending frame and returns within the requested
  timeout
- **AND** no thread, RTOS task, or background scheduler is required

#### Scenario: Reject a write to a read-only variable
- **WHEN** a master writes FC 0x06 to the address of a variable declared
  `access::read_only`
- **THEN** the slave responds with exception code 0x04 (slave device failure) or
  0x02 (illegal data address) per the configured policy
- **AND** the variable's value is unchanged

### Requirement: The Library Shall Provide A Master Client With Cooperative Scheduling

The master client SHALL drive its work through `master.poll_once()` and SHALL allow
the application to register periodic polls of remote variables and one-shot writes
of remote variables. Mirrored variable values SHALL be readable by application code
atomically with respect to ongoing transport activity.

#### Scenario: Mirror updates after a successful poll
- **WHEN** the master polls a remote `var<int32_t>` at 10 ms and the slave's value
  changes
- **THEN** the next `master.poll_once()` after the response arrives updates the
  master's mirror of that variable
- **AND** an application read of the mirror returns the new value without tearing

#### Scenario: Stale-data callback fires on a missed deadline
- **WHEN** a polled variable's slave goes silent past its configured stale threshold
- **THEN** if the application has registered a stale-data callback, it is invoked
  for that variable with the stale duration
- **AND** subsequent successful polls clear the stale state

### Requirement: The Library Shall Provide A Half-Duplex RS-485 DE Helper

The library SHALL provide a stream decorator that drives a GPIO around transmissions
to control the RS-485 transceiver's DE/RE pin, asserting the pin before the first
TX byte leaves the wire and releasing the pin after the last TX byte is acknowledged
by the underlying stream's `flush()`.

#### Scenario: DE pin sequence around a TX
- **WHEN** application code wraps a UART byte stream with the RS-485 DE helper and
  writes a frame
- **THEN** the configured DE GPIO is high before the first byte is shifted out
- **AND** the GPIO is low only after the underlying stream's `flush()` has reported
  TX complete

### Requirement: The Library Shall Support All Four Modbus Word Orders Per Variable

For multi-register variables, the library SHALL support the four word orders ABCD,
CDAB, BADC, and DCBA, configurable per variable, with ABCD as the default. The order
SHALL apply consistently to encode (slave -> wire) and decode (wire -> slave or
master mirror).

#### Scenario: Default order is ABCD
- **WHEN** application code declares a `var<float>` without specifying word order
- **THEN** the slave encodes the value in ABCD order on the wire
- **AND** a master reading that variable into its own ABCD-default mirror sees the
  same bit pattern

#### Scenario: Per-variable override
- **WHEN** a variable declares `word_order::CDAB`
- **THEN** the slave encodes that variable's bytes in CDAB order on the wire
- **AND** a master configured with a matching word order reads the value correctly

### Requirement: The Library Shall Implement A Custom Discovery Function Code

The library SHALL implement a custom discovery function code (default 0x65, in
Modbus's user-defined range 0x65-0x72, configurable at slave construction) returning
a compact table describing every exposed variable: register address, type id,
access flags, and symbolic name. A second sub-function SHALL return rich metadata
(range, unit, description) for variables that declare it. Standard Modbus clients
that ignore the custom function code SHALL be unaffected.

#### Scenario: Discover a slave's variables
- **WHEN** a master sends FC 0x65 sub-function 0x01 to a slave with three registered
  variables
- **THEN** the response contains exactly three entries listing each variable's
  address, type, access, and name
- **AND** a standard Modbus client unaware of FC 0x65 receives an exception 0x01
  (illegal function) and continues to function for standard FCs

#### Scenario: Avoid a function-code conflict
- **WHEN** a user constructs a slave with `discovery_fc = 0x66` because the user
  already deploys a different feature on 0x65
- **THEN** discovery responses are emitted on FC 0x66 instead
- **AND** FC 0x65 falls through to standard exception handling

### Requirement: The Library Shall Be Testable End-To-End On The Host

The library SHALL be testable end to end on a host machine without any embedded
hardware, using the in-memory loopback stream to wire a master and a slave together
in the same process and exercising every public surface against published Modbus
reference frames.

#### Scenario: Host CI exercises the slave's full FC surface
- **WHEN** the host test suite runs
- **THEN** a slave instance is exercised against every implemented function code
  (read coils, read discrete inputs, read holding registers, read input registers,
  write single coil, write single register, write multiple coils, write multiple
  registers, read/write multiple registers) over the loopback stream
- **AND** every assertion is byte-exact against published reference frames

### Requirement: The Library Shall Stay Inside The Footprint Budget

The library SHALL fit within 8 KB of code and 4 KB of RAM for a slave configured with
100 thin variables on a Cortex-M0+ target. Footprint SHALL be measured against a
representative configuration in CI and tracked across changes.

#### Scenario: 100-variable slave fits in budget
- **WHEN** the size-tracking CI job builds a representative 100-thin-variable slave
  configuration
- **THEN** the resulting `.text + .rodata + .data + .bss` footprint stays under
  12 KB total
- **AND** any change pushing the footprint over budget fails the size gate

