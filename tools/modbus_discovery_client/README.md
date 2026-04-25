# modbus_discovery_client

Python reference client for the Alloy FC 0x65 Modbus discovery protocol.
Sends a thin or rich discovery probe and prints the slave's variable table.

## Install

```bash
pip install pyserial
```

## Usage

```bash
# Thin probe (address, type, access, name)
python modbus_discovery_client.py --port /dev/ttyUSB0

# Rich probe (+ unit, description, engineering range)
python modbus_discovery_client.py --port /dev/ttyUSB0 --sub rich

# Custom slave ID and baud rate
python modbus_discovery_client.py --port COM3 --baud 9600 --slave 0x05

# Override FC if slave uses 0x66 instead of default 0x65
python modbus_discovery_client.py --port /dev/ttyUSB0 --fc 0x66 --sub rich

# JSON output (pipe into jq, save to file, etc.)
python modbus_discovery_client.py --port /dev/ttyUSB0 --sub rich --json | jq .
```

## Example output

```
slave 0x01 — 10 variable(s) [thin]

Addr   Regs  Type     Access      Name
--------------------------------------------------
0x0000  2     Float    ReadOnly    temperature
0x0002  1     Uint16   ReadWrite   status_word
0x0003  1     Int16    ReadWrite   trim_offset
0x0004  2     Float    ReadOnly    voltage_v
...
```

Rich output:
```
slave 0x01 — 10 variable(s) [rich]

Addr   Regs  Type     Access      Name                     Unit     Description                    Range
0x0000  2     Float    ReadOnly    temperature              degC     ambient temperature            -40..125
0x0002  1     Uint16   ReadWrite   status_word                                                     0..0
...
```

## Protocol reference

See [docs/MODBUS.md](../../docs/MODBUS.md) — Discovery FC reference.
