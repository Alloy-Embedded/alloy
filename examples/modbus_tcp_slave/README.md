# modbus_tcp_slave

Modbus TCP slave on port 502 using the SAME70 Xplained Ultra Ethernet port. Exposes 10 variables (same map as `modbus_slave_basic`) over TCP instead of RS-485.

## Build

```bash
alloy build --board same70_xplained --target modbus_tcp_slave
alloy flash  --board same70_xplained --target modbus_tcp_slave
alloy monitor --board same70_xplained
```

Override slave ID:
```bash
cmake -DSLAVE_ID=0x05 ...
```

## Test with pymodbus

```python
from pymodbus.client import ModbusTcpClient

c = ModbusTcpClient("192.168.1.42", port=502)
c.connect()

# Read holding registers 0x0000–0x000D
rr = c.read_holding_registers(0, 14, slave=1)
print(rr.registers)

# Write setpoint (register 2)
c.write_register(2, 600, slave=1)

c.close()
```

## Variable map

| Address | Registers | Type    | Access    | Name        |
|---------|-----------|---------|-----------|-------------|
| 0x0000  | 1         | uint16  | ReadWrite | status      |
| 0x0001  | 1         | int16   | ReadOnly  | temp_raw    |
| 0x0002  | 1         | uint16  | ReadWrite | setpoint    |
| 0x0003  | 1         | int16   | ReadWrite | trim_offset |
| 0x0004  | 2         | float   | ReadWrite | pressure    |
| 0x0006  | 2         | float   | ReadOnly  | voltage     |
| 0x0008  | 2         | int32   | ReadWrite | position    |
| 0x000A  | 2         | uint32  | ReadOnly  | uptime      |
| 0x000C  | 1         | bool    | ReadWrite | enable      |
| 0x000D  | 1         | uint16  | ReadWrite | errors      |

## Architecture

```
TcpStream → satisfies modbus::ByteStream
Slave<TcpStream> ← same Slave template as RTU, different stream
LwipAdapter<EthernetInterface<Same70Gmac, KSZ8081>>
```

This closes the "pending network HAL" note from the Modbus TCP framing task.
See [docs/NETWORK.md](../../docs/NETWORK.md) and [docs/MODBUS.md](../../docs/MODBUS.md).
