# modbus_master_poll

Modbus RTU master polling 3 variables from a remote slave. Uses the
cooperative two-phase `send_due_request()` / `recv_due_response()` scheduler
so the main loop can do other work between sending and receiving.

## Polled variables

| Address | Type    | Slave ID | Interval | Name        |
|---------|---------|----------|----------|-------------|
| 0x0000  | float   | 0x01     | 500 ms   | temperature |
| 0x0004  | uint16  | 0x01     | 200 ms   | status_word |
| 0x0008  | int32   | 0x01     | 1 s      | position    |

These addresses must match the slave's registry (e.g. `modbus_slave_basic`).

## Wiring

```
Board A (master)          Board B (slave)
  UART TX ─────────────── UART RX
  UART RX ─────────────── UART TX
  GND ──────────────────── GND
```

For RS-485, insert a half-duplex transceiver on each end and assert DE/RE
appropriately (see `Rs485DeStream` in `alloy/modbus/transport/rs485_de.hpp`).

## Notes

- Mirror values (`g_temp_mirror`, etc.) are safe to read from the main loop
  on single-threaded targets. For ISR access, pass a platform `CritSection`
  as the fourth `Master<>` template argument.
- `on_stale` is called when a response times out — wire it to an alarm or
  watchdog in production.
- Requires `alloy/modbus/transport/uart_stream.hpp` (task 4.2, pending).
  Also requires a `board::make_modbus_uart()` factory in the board header.
