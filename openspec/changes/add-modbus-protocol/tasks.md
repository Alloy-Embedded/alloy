# Tasks: Modbus Protocol Library

Tasks are ordered. Each phase leaves the tree in a working state and is independently
mergeable. Host-only tests cover every phase that does not require hardware.

## 1. Library skeleton
- [ ] 1.1 Create `drivers/protocol/modbus/{include,src,tests}/` with a CMake
      integration that registers the static library `alloy::modbus` and wires its
      tests into `ctest` when `ALLOY_BUILD_TESTS=ON`.
- [ ] 1.2 Add a `drivers/protocol/modbus/README.md` linking to the user guide and
      describing the layer split.

## 2. PDU codec (pure)
- [ ] 2.1 Implement `pdu.hpp` / `pdu.cpp`: function-code enum, encode/decode for
      FCs 0x01-0x06, 0x0F-0x10, 0x17, plus exception responses.
- [ ] 2.2 Cover with `tests/pdu_codec_test.cpp`: round-trip encode/decode, exception
      paths, malformed-input rejection. Pure host, no I/O.

## 3. RTU framing
- [ ] 3.1 Implement `rtu_frame.hpp`: CRC-16 (0xA001 polynomial, 256-entry table in
      flash), inter-frame silence rules, frame builder and parser.
- [ ] 3.2 Inter-frame silence detection uses a microsecond timestamp source from
      the runtime; the framer takes a `now_us()` callable so tests can drive time.
- [ ] 3.3 Cover with `tests/rtu_framing_test.cpp`: known-good Modbus frames from
      published examples; CRC mismatch rejection; silence-violation rejection.

## 4. byte_stream transport
- [ ] 4.1 Define `byte_stream.hpp`: `read(span, timeout)`, `write(span)`, `flush()`,
      `wait_idle(silence)` interface returning `core::Result`.
- [ ] 4.2 Implement `transport/uart_stream.hpp`: adapter from
      `alloy::hal::uart::handle` to `byte_stream`. Uses the existing `flush()` path
      to wait on TC.
- [ ] 4.3 Implement `transport/loopback_stream.hpp`: in-memory ring buffer pair for
      tests; allows two `slave`/`master` instances to talk in a single host process.
- [ ] 4.4 Tests: loopback round-trip of an arbitrary byte sequence.

## 5. RS-485 DE pin helper
- [ ] 5.1 Implement `transport/rs485_de.hpp`: stream decorator that toggles a GPIO
      around `write()` and waits on the wrapped stream's `flush()`.
- [ ] 5.2 Test: instrument a fake GPIO + loopback stream and assert DE goes high
      before the first byte and low after the last byte's TC.

## 6. Variable registry
- [ ] 6.1 Implement `var.hpp`: `var<T>` template with address, access, name, and
      optional metadata struct. `T` constrained to `int16_t`, `uint16_t`, `int32_t`,
      `uint32_t`, `float`, `double`, `bool`. Word order per-var, default ABCD.
- [ ] 6.2 Implement word-order conversion utilities and cover with
      `tests/word_order_test.cpp` (all four orders, all multi-register types).
- [ ] 6.3 Implement a small registry that the slave indexes by address.
- [ ] 6.4 Validate at compile time that a registry's footprint estimate is below a
      configurable cap (`ALLOY_MODBUS_REGISTRY_MAX_BYTES`, default 8 KB).

## 7. Slave server
- [ ] 7.1 Implement `slave.hpp` / `slave.cpp`: cooperative `poll(timeout)` consuming
      from `byte_stream`, parsing frames, dispatching to FC handlers, writing
      responses (or exceptions). No allocation in the hot path.
- [ ] 7.2 Implement `slave.run()` as the trivial loop helper.
- [ ] 7.3 FC handlers: 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10, 0x17 against
      the variable registry.
- [ ] 7.4 Critical-section guard around multi-register operations to prevent tearing
      against application code.
- [ ] 7.5 Cover with `tests/slave_test.cpp` (over loopback): exercise every FC with
      both happy and exception paths; assert correct responses byte-for-byte.

## 8. Master client
- [ ] 8.1 Implement `master.hpp` / `master.cpp`: cooperative `poll_once()`,
      `poll(var, slave_id, rate)`, `write(var)`. Internal scheduler is a small
      sorted vector keyed by next-deadline timestamp.
- [ ] 8.2 Mirror update is atomic against application reads through the same
      critical-section primitive used on the slave side.
- [ ] 8.3 Stale-data callback hook (per design.md, optional setter).
- [ ] 8.4 Cover with `tests/master_test.cpp` and `tests/master_slave_loopback_test.cpp`:
      master polls slave's vars over loopback; assert mirror values track slave
      vars after the configured poll interval.

## 9. Discovery FC
- [ ] 9.1 Implement `discovery.hpp`: FC 0x65 sub-functions 0x01 (thin: addr, type,
      access, name) and 0x02 (rich: range, unit, desc).
- [ ] 9.2 Slave publishes the discovery table compiled from its registry; master
      can issue a discovery probe and decode the response.
- [ ] 9.3 The default function code is overridable at slave construction
      (`discovery_fc = 0x66`) for users with prior art on 0x65.
- [ ] 9.4 Cover with `tests/discovery_test.cpp`: round-trip discovery probe over
      loopback against a slave with mixed thin and rich vars.

## 10. TCP framing (scaffolded)
- [ ] 10.1 Implement `tcp_frame.hpp`: MBAP header encode/decode and frame builder.
      Compiles unconditionally; not exposed to the slave/master until a TCP stream
      adapter exists.
- [ ] 10.2 Cover with `tests/tcp_framing_test.cpp` against published MBAP examples.
- [ ] 10.3 Document in the user guide that TCP transport is pending the network HAL.

## 11. Examples
- [ ] 11.1 `examples/modbus_slave_basic`: 10 vars (mix of int16/int32/float), RTU
      slave on board UART, foundational board only.
- [ ] 11.2 `examples/modbus_slave_rich`: 30 vars with full metadata + discovery FC
      enabled. Demonstrates dashboard auto-build path.
- [ ] 11.3 `examples/modbus_master_poll`: master polling 3 vars from the slave
      example over a loopback wire (or two boards back to back).

## 12. Documentation
- [ ] 12.1 `docs/MODBUS.md`: user guide -- slave quickstart, master quickstart,
      RS-485 wiring notes, discovery FC reference, footprint guidance, common
      gotchas (word order, DE pin, baud-rate-specific timing).
- [ ] 12.2 `docs/SUPPORT_MATRIX.md`: add a `modbus` peripheral-class entry; tier
      `representative` once host loopback tests pass; `compile-only` until then.
- [ ] 12.3 `docs/CLI.md`: cross-link to MODBUS.md from the runtime tooling section
      so users discover the library when scaffolding a project.
- [ ] 12.4 Reference Python client for FC 0x65 discovery (in
      `tools/modbus_discovery_client/`). Optional but encouraged so users can
      validate end-to-end without writing client code.
