# alloy::modbus

In-tree Modbus library for the Alloy runtime. Master + slave, transport-agnostic at
the bytes layer, with a typed variable registry that the next protocol (MQTT, CoAP)
can reuse.

The library is being built incrementally per
[`openspec/changes/add-modbus-protocol`](../../../openspec/changes/add-modbus-protocol/).
Current status:

| Layer | Status |
|---|---|
| PDU codec (FC 0x01-0x06, 0x0F-0x10, 0x17, exceptions) | ✅ phase 2 |
| RTU framing (CRC + inter-frame silence) | ⏳ phase 3 |
| `byte_stream` interface + UART adapter + loopback | ⏳ phase 4 |
| RS-485 DE pin helper | ⏳ phase 5 |
| Variable registry (`alloy::modbus::var<T>`) | ⏳ phase 6 |
| Slave (cooperative) | ⏳ phase 7 |
| Master (cooperative) | ⏳ phase 8 |
| Discovery FC 0x65 | ⏳ phase 9 |
| TCP framing scaffolded | ⏳ phase 10 |
| Examples + docs | ⏳ phases 11-12 |

## Tests

```
cmake -B build-tests -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON
cmake --build build-tests
cd build-tests && ctest --output-on-failure -L modbus
```
