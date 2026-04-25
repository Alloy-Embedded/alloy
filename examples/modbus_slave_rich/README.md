# modbus_slave_rich

Modbus RTU slave with 30 variables — all annotated with `VarMeta` (unit,
description, engineering range). A master or dashboard tool can use the
discovery FC 0x65 sub-function 0x02 to enumerate the entire variable table
without a pre-configured map file.

## Variable groups

| Addresses   | Group         | Types             |
|-------------|---------------|-------------------|
| 0x00–0x07   | Environmental | float ×4          |
| 0x08–0x0F   | Power         | float ×4          |
| 0x10–0x1B   | Control       | float ×5, bool ×2 |
| 0x1C–0x27   | Diagnostics   | uint32/uint16/int16 mix |

## Discovery probe with Python

```python
from pymodbus.client import ModbusSerialClient
import struct

c = ModbusSerialClient(port="/dev/ttyUSB0", baudrate=115200)
c.connect()

# FC 0x65 sub-fn 0x02 — rich discovery
pdu = bytes([0x65, 0x02])
raw = c.execute(pdu)   # raw PDU back
# Decode with alloy tools/modbus_discovery_client (see task 12.4)
```

## Notes

- Discovery FC default is 0x65; override with `Slave{stream, id, reg, 0x66u}`.
- All `VarMeta` strings are stored as `string_view` literals in flash.
- Requires `alloy/modbus/transport/uart_stream.hpp` (task 4.2, pending).
