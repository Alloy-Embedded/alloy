# modbus_slave_basic

Modbus RTU slave with 10 mixed-type variables. Intended as a minimal starting
point for adding Modbus connectivity to any board with a UART peripheral.

## Variables

| Address | Count | Type    | Access    | Name             |
|---------|-------|---------|-----------|------------------|
| 0x0000  | 1     | uint16  | ReadWrite | status_word      |
| 0x0001  | 1     | int16   | ReadOnly  | temperature_raw  |
| 0x0002  | 1     | uint16  | ReadWrite | setpoint         |
| 0x0003  | 1     | int16   | ReadWrite | trim_offset      |
| 0x0004  | 2     | float   | ReadWrite | pressure_hpa     |
| 0x0006  | 2     | float   | ReadOnly  | voltage_v        |
| 0x0008  | 2     | int32   | ReadWrite | position_steps   |
| 0x000A  | 2     | uint32  | ReadOnly  | uptime_s         |
| 0x000C  | 1     | bool    | ReadWrite | enable_coil      |
| 0x000D  | 1     | uint16  | ReadWrite | error_flags      |

## Quick test with pymodbus

```python
from pymodbus.client import ModbusSerialClient

c = ModbusSerialClient(port="/dev/ttyUSB0", baudrate=115200)
c.connect()
# Read holding registers 0x00..0x0D
r = c.read_holding_registers(address=0, count=14, slave=1)
print(r.registers)
```

## Notes

- Requires `alloy/modbus/transport/uart_stream.hpp` (task 4.2, pending).
- Compile with `-DSLAVE_ID=0x02` to change the slave address.
- Word order defaults to `WordOrder::ABCD` (Modbus big-endian). Adjust
  the `Var<T>` declarations if your master uses a different convention.
