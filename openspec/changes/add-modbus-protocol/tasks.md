# Tasks: Modbus Protocol Library

Tasks are ordered. Each phase leaves the tree in a working state and is independently
mergeable. Host-only tests cover every phase that does not require hardware.

## 1. Library skeleton
- [x] 1.1 Create `drivers/protocol/modbus/{include,src,tests}/` with a CMake
      integration that registers the static library `alloy::modbus` and wires its
      tests into `ctest` when `ALLOY_BUILD_TESTS=ON`. The lib links against
      `Alloy::hal` (which republishes the `core::Result` include path); the
      pre-existing `drivers/CMakeLists.txt` reference to `alloy::alloy` was
      header-only and untested -- the modbus lib is the first real driver
      consumer of the runtime export so a working alias was needed.
- [x] 1.2 Add `drivers/protocol/modbus/README.md` listing the implemented and
      pending layers; cross-links to the OpenSpec change.

## 2. PDU codec (pure)
- [x] 2.1 Implement `pdu.hpp` / `pdu.cpp`: function-code and exception enums,
      typed request/response views, encode/decode for FCs 0x01-0x06, 0x0F-0x10,
      0x17, plus exception responses. Spec quantity caps enforced on encode and
      decode. No allocation. `noexcept` throughout.
- [x] 2.2 `tests/test_pdu.cpp`: 21 cases covering reference-frame round-trips
      (Modbus Application Protocol Spec section 6), bound checks, truncation,
      buffer-too-small, exception encode/decode, and FC dispatch. 84 assertions
      pass on host.

## 3. RTU framing
- [x] 3.1 Implement `rtu_frame.hpp`: CRC-16 (0xA001 polynomial, 256-entry table in
      flash), inter-frame silence rules, frame builder and parser.
- [x] 3.2 Inter-frame silence detection uses a microsecond timestamp source from
      the runtime; the framer takes a `now_us()` callable so tests can drive time.
- [x] 3.3 Cover with `tests/test_rtu_frame.cpp`: known-good Modbus frames from
      published examples (spec slave 0x11); CRC mismatch rejection; silence-violation
      rejection. 24 test cases, 72 assertions pass on host.

## 4. byte_stream transport
- [x] 4.1 Define `byte_stream.hpp`: `read(span, timeout)`, `write(span)`, `flush()`,
      `wait_idle(silence)` interface returning `core::Result`. Expressed as a C++20
      concept `ByteStream<T>` to enable static-dispatch at zero overhead.
- [ ] 4.2 Implement `transport/uart_stream.hpp`: adapter from
      `alloy::hal::uart::handle` to `byte_stream`. Uses the existing `flush()` path
      to wait on TC.
- [x] 4.3 Implement `transport/loopback_stream.hpp`: `LoopbackPair<Cap>` with two
      non-owning `LoopbackEndpoint` views wired back-to-back; satisfies `ByteStream`.
- [x] 4.4 Tests: loopback round-trip, bidirectional, overrun, and end-to-end RTU
      frame encode→loopback→decode — all covered in `test_rtu_frame.cpp`.

## 5. RS-485 DE pin helper
- [x] 5.1 Implement `transport/rs485_de.hpp`: stream decorator (`Rs485DeStream<Stream,
      Pin>`) satisfying `ByteStream`; `DePin` concept requires `set_high()`/`set_low()`;
      DE is asserted on `write()` and de-asserted after `flush()`.
- [x] 5.2 Tests in `tests/test_rs485_de.cpp`: `FakeDePin` instruments state transitions;
      6 cases, 32 assertions verify DE high/low sequencing, pass-through of read/wait_idle,
      re-assertion across multiple write/flush cycles, and ByteStream concept satisfaction.

## 6. Variable registry
- [x] 6.1 Implement `var.hpp`: `VarValueType` concept (bool, int16/uint16, int32/uint32,
      float, double); `WordOrder` enum (ABCD/CDAB/BADC/DCBA); `Access` enum;
      `var_reg_count<T>()` consteval; `Var<T>` descriptor with `encode()`/`decode()`.
- [x] 6.2 `encode_words<T>()` / `decode_words<T>()` constexpr; all four word orders
      for 2-register and 4-register types; verified with round-trips and known IEEE 754
      reference frames. Tests in `tests/test_var.cpp` (24 cases, 88 assertions).
- [x] 6.3 Implement `registry.hpp`: type-erased `VarDescriptor`; `Registry<N>` with
      O(N) `find(address)` and range iteration; `make_registry(Var<Ts>...)` factory.
- [x] 6.4 Compile-time footprint check: `static_assert(N * sizeof(VarDescriptor) <=
      ALLOY_MODBUS_REGISTRY_MAX_BYTES)` (default 8 KB, user-overridable via macro).

## 7. Slave server
- [x] 7.1 Implement `slave.hpp`: cooperative `poll(timeout_us)` consuming from
      `byte_stream`; decodes RTU frame, ignores frames for other IDs, dispatches to
      FC handler, encodes and writes RTU response. All buffers stack-local; no alloc.
- [x] 7.2 `slave.run(timeout_us)` as trivial `[[noreturn]]` loop over `poll()`.
- [x] 7.3 FC handlers: 0x01/02 (read coils/discrete inputs), 0x03/04 (read
      holding/input registers), 0x05 (write single coil), 0x06 (write single
      register), 0x0F (write multiple coils), 0x10 (write multiple registers),
      0x17 (read/write multiple registers). All map to the bound Registry<N>.
- [x] 7.4 Critical-section guard (`CritSection` template param, default
      `NoOpCriticalSection`) wraps each `write_register_word()` RMW to prevent
      tearing of multi-word vars (float/int32/double) against ISR access.
- [x] 7.5 `tests/test_slave.cpp`: 15 cases / 171 assertions over LoopbackPair.
      Every FC exercised with happy path (byte-for-byte response check) and
      exception path (unmapped address → IllegalDataAddress, read-only write →
      IllegalDataAddress). Also: wrong slave ID → no response.

## 8. Master client
- [x] 8.1 `master.hpp`: `Master<Stream, MaxPolls, NowFn, CritSection>` template;
      `add_poll(Var<T>, T& mirror, slave_id, interval_us)` registers type-erased
      `PollDescriptor`; `send_due_request()` + `recv_due_response()` two-phase API
      (allows interleaving with slave in tests); `poll_once()` wraps both phases.
      `write_now<T>()` for immediate FC06/FC10 write. Scheduler: O(N) scan of
      fixed-size array, earliest overdue deadline wins; in-flight entry is skipped.
- [x] 8.2 Mirror update in `recv_due_response()` wrapped in `CritSection` RAII guard
      (same template param pattern as `Slave`; default `NoOpCriticalSection` shared
      from `byte_stream.hpp`).
- [x] 8.3 `set_stale_callback(fn)`: optional C function pointer called with
      (reg_address, slave_id) when `recv_due_response()` times out.
- [x] 8.4 `tests/test_master.cpp`: 11 cases / 82 assertions covering registration,
      SlotsFull guard, mirror update from fake FC03 responses (uint16_t and float),
      scheduling (not due / fires after interval), stale callback, and 3 integrated
      master+slave loopback tests asserting mirror tracks slave value across multiple
      poll cycles.

## 9. Discovery FC
- [x] 9.1 Implement `discovery.hpp`: FC 0x65 sub-functions 0x01 (thin: addr, type,
      access, name) and 0x02 (rich: range, unit, desc). `VarType` enum and
      `var_type_tag<T>()` consteval specializations added to `var.hpp`; `VarMeta`
      struct added for optional unit/desc/range. `VarDescriptor` gains `type_tag`
      and `meta` fields (populated by both `make_descriptor()` and `bind()`).
      Detail helpers: `put_byte/u16/string/f32_be`, `get_byte/u16/string/f32_be`.
- [x] 9.2 `Slave` includes `discovery.hpp` and intercepts PDUs whose first byte
      equals `discovery_fc_` (default 0x65) before standard FC dispatch. Handler
      calls `encode_discovery_thin` or `encode_discovery_rich` based on sub-fn byte.
      Also fixed exception response to echo the original FC byte (per Modbus spec)
      rather than using a placeholder 0x00 for unknown FCs.
- [x] 9.3 Discovery FC is overridable at slave construction
      (`Slave{stream, id, reg, 0x66u}`) or via `set_discovery_fc(uint8_t)`.
- [x] 9.4 `tests/test_discovery.cpp`: 12 cases / 105 assertions. Thin+rich
      encode/decode round-trips; all three vars validated (address, reg_count,
      type_tag, access, name, unit, desc, range); buffer-too-small and truncated-PDU
      error paths; alternate FC override; two slave loopback probes (thin/rich);
      FC 0x65 correctly rejected (IllegalFunction) by a slave configured for 0x66.

## 10. TCP framing (scaffolded)
- [x] 10.1 Implement `tcp_frame.hpp`: MBAP header (7 B) encode/decode and frame
      builder. `encode_tcp_frame(out, tx_id, unit_id, pdu)` → Result<size_t, TcpError>;
      `decode_tcp_frame(adu)` → Result<TcpFrame, TcpError> (zero-copy pdu subspan).
      Constants: `kMbapHeaderBytes=7`, `kTcpMaxPduBytes=253`, `kTcpMaxAduBytes=260`.
      Compiles unconditionally; not wired to slave/master until a TCP stream adapter
      exists. Error cases: BufferTooSmall, Truncated, BadProtocol, PduTooLarge.
- [x] 10.2 `tests/test_tcp_frame.cpp`: 12 cases / 55 assertions. Published MBAP
      reference frame encode+decode; round-trips for max-size PDU, empty PDU,
      and transaction_id preservation; protocol-id rejection; truncated-ADU and
      declared-length-mismatch errors; buffer-too-small and PDU-too-large encode
      errors; zero-copy subspan verification; FC03 integration round-trip.
- [ ] 10.3 Document in the user guide that TCP transport is pending the network HAL.

## 11. Examples
- [x] 11.1 `examples/modbus_slave_basic`: 10 vars (uint16/int16/float/int32/
      uint32/bool mix), RTU slave on board UART. Compile-time Var<T> descriptors,
      bind() to runtime storage, cooperative poll() loop. Slave ID overridable
      via -DSLAVE_ID=0x02. Pending uart_stream.hpp (task 4.2).
- [x] 11.2 `examples/modbus_slave_rich`: 30 vars (4 groups: environmental, power,
      control, diagnostics) with full VarMeta (unit, desc, range). Discovery FC
      0x65 enabled by default. README shows Python discovery probe snippet.
      Pending uart_stream.hpp (task 4.2).
- [x] 11.3 `examples/modbus_master_poll`: Master<UartStream, 3, NowFn> polling
      temperature (float, 500 ms), status_word (uint16, 200 ms), position (int32,
      1 s) from slave 0x01. stale callback, poll_once() loop. Wiring diagram and
      RS-485 note in README. Pending uart_stream.hpp (task 4.2).

## 12. Documentation
- [x] 12.1 `docs/MODBUS.md`: user guide covering slave quickstart, master quickstart,
      variable types + word order table, RS-485 wiring with Rs485DeStream, discovery
      FC 0x65 protocol reference, critical section guide, TCP framing API, footprint
      guidance, common gotchas (word order mismatch, DE pin timing, inter-frame
      silence, CRC table), examples table.
- [x] 12.2 `docs/SUPPORT_MATRIX.md`: `modbus` peripheral class added at tier
      `representative` with host loopback evidence (634 assertions / 113 cases across
      PDU codec, RTU framing, RS-485, registry, slave FC01–17, master, discovery,
      TCP MBAP). UART stream adapter and hardware spot-check noted as pending.
- [x] 12.3 `docs/CLI.md`: "Protocol libraries" section added before Stability,
      cross-linking to MODBUS.md with a bullet summary of the library's contents.
- [x] 12.4 `tools/modbus_discovery_client/modbus_discovery_client.py`: reference
      Python client for FC 0x65 thin+rich discovery. Builds RTU frames with CRC-16
      (no pymodbus dependency, only pyserial). Table and JSON output modes.
      Supports --port, --baud, --slave, --fc, --sub, --timeout, --json flags.
