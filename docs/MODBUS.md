# Alloy Modbus Protocol Library

`alloy::modbus` is an allocation-free Modbus RTU/TCP library that lives in
`drivers/protocol/modbus/`. It targets embedded systems: all buffers are
stack-local, no dynamic memory, `noexcept` throughout, and the core codec
is `constexpr`-safe for testing on host.

## Quick-start: RTU slave

```cpp
#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/slave.hpp"
#include "alloy/modbus/transport/uart_stream.hpp"   // requires task 4.2
#include "alloy/modbus/var.hpp"

using namespace alloy::modbus;

// 1. Declare typed variables (compile-time, zero overhead).
constexpr Var<float>         kTemp  {.address=0x00u, .access=Access::ReadOnly,  .name="temperature"};
constexpr Var<std::uint16_t> kStatus{.address=0x02u, .access=Access::ReadWrite, .name="status_word"};

// 2. Backing storage.
float         g_temp{25.0f};
std::uint16_t g_status{0u};

// 3. Bind at runtime, build registry.
auto registry = Registry<2u>{std::array<VarDescriptor, 2u>{
    bind(kTemp,   g_temp),
    bind(kStatus, g_status),
}};

// 4. Wrap your UART handle and run the slave.
UartStream stream{uart};
Slave slave{stream, 0x01u, registry};

while (true) {
    board::application_logic();
    (void)slave.poll(10'000u);  // 10 ms timeout
}
```

Supported function codes: 01/02 (read coils/discrete inputs), 03/04 (read
holding/input registers), 05 (write single coil), 06 (write single register),
0F (write multiple coils), 10 (write multiple registers), 17 (read/write
multiple registers), and vendor FC 0x65 (discovery).

## Quick-start: RTU master

```cpp
#include "alloy/modbus/master.hpp"
#include "alloy/modbus/transport/uart_stream.hpp"

using namespace alloy::modbus;

constexpr Var<float> kTemp{.address=0x00u, .access=Access::ReadOnly, .name="temperature"};
float g_temp_mirror{};

UartStream stream{uart};

// NowFn must return uint64_t microseconds.
auto now_us = []() noexcept -> std::uint64_t { return board::systick_us(); };

Master<UartStream, 4u, decltype(now_us)> master{stream, now_us};

// Register for polling every 500 ms from slave 0x01.
master.add_poll(kTemp, g_temp_mirror, 0x01u, 500'000u);

while (true) {
    (void)master.poll_once(1'000u);  // 1 ms response timeout
    // g_temp_mirror is now up to date.
}
```

## Variable types and register counts

| C++ type        | VarType tag | Registers |
|-----------------|-------------|-----------|
| `bool`          | Bool (0)    | 1         |
| `int16_t`       | Int16 (1)   | 1         |
| `uint16_t`      | Uint16 (2)  | 1         |
| `int32_t`       | Int32 (3)   | 2         |
| `uint32_t`      | Uint32 (4)  | 2         |
| `float`         | Float (5)   | 2         |
| `double`        | Double (6)  | 4         |

## Word order

Multi-register types (int32, uint32, float, double) support four word orders:

| `WordOrder` | Meaning                                 |
|-------------|------------------------------------------|
| `ABCD`      | Big-endian, MSW first (Modbus default)   |
| `CDAB`      | Word-swapped — LSW first, bytes unchanged|
| `BADC`      | Byte-swapped within each word            |
| `DCBA`      | Fully little-endian                      |

Set per variable: `Var<float>{..., .word_order=WordOrder::CDAB}`.
Most PLC masters default to `ABCD`; check your master's documentation.

## RS-485 wiring

For half-duplex RS-485, wrap your stream in `Rs485DeStream`:

```cpp
#include "alloy/modbus/transport/rs485_de.hpp"

struct DePin {
    void set_high() noexcept { board::gpio::de_pin::set_high(); }
    void set_low()  noexcept { board::gpio::de_pin::set_low();  }
};

DePin de_pin{};
Rs485DeStream<UartStream, DePin> stream{uart_stream, de_pin};
Slave slave{stream, 0x01u, registry};
```

`Rs485DeStream` asserts DE on `write()` and de-asserts after `flush()`.
The `DePin` concept only requires `set_high()` and `set_low()`.

## Discovery FC reference

The discovery function code (default 0x65) lets any master enumerate the
slave's variable table without a pre-configured register map.

**Request PDU:**
```
[FC=0x65][sub_fn=0x01|0x02]
```

**Thin response (sub-fn 0x01) — one entry per var:**
```
[FC][0x01][count_hi][count_lo] { [addr_hi][addr_lo][reg_count][type][access][name_len][name...] } × N
```

**Rich response (sub-fn 0x02) — adds metadata per var:**
```
... thin entry ... [unit_len][unit...][desc_len][desc...][range_min_4B_BE][range_max_4B_BE]
```

Override the FC at slave construction if 0x65 conflicts with existing firmware:
```cpp
Slave slave{stream, id, registry, 0x66u};  // use 0x66 instead
```

Attach metadata for rich discovery using `VarMeta`:
```cpp
constexpr VarMeta kMeta{.unit="degC", .desc="ambient temperature",
                        .range_min=-40.0f, .range_max=125.0f};
bind(kTemp, g_temp, &kMeta)  // third arg in bind()
```

## Critical section (ISR safety)

For `float`, `int32_t`, and `double` (multi-register) variables, both `Slave`
and `Master` update backing storage under a `CritSection` RAII guard. The
default `NoOpCriticalSection` is correct for single-threaded / cooperative
code. Replace it for ISR-concurrent access:

```cpp
struct McuCs {
    McuCs() noexcept  { __disable_irq(); }
    ~McuCs() noexcept { __enable_irq();  }
};

Slave<UartStream, N, McuCs> slave{stream, id, registry};
Master<UartStream, MaxPolls, NowFn, McuCs> master{stream, now_us};
```

## Footprint

The registry size is statically checked at compile time:

```
static_assert(N * sizeof(VarDescriptor) <= ALLOY_MODBUS_REGISTRY_MAX_BYTES)
```

Default cap: **8 192 bytes** (≈ 80 variables at ~100 B each). Override:

```cmake
target_compile_definitions(my_target PRIVATE
    ALLOY_MODBUS_REGISTRY_MAX_BYTES=16384
)
```

## TCP framing

`tcp_frame.hpp` provides MBAP encode/decode for Modbus TCP:

```cpp
#include "alloy/modbus/tcp_frame.hpp"

// Encode
std::array<std::byte, kTcpMaxAduBytes> buf{};
auto enc = encode_tcp_frame(buf, tx_id, unit_id, pdu_span);

// Decode
auto dec = decode_tcp_frame(std::span{buf.data(), enc.unwrap()});
const TcpFrame& f = dec.unwrap();
// f.transaction_id, f.unit_id, f.pdu (zero-copy subspan)
```

TCP transport integration is available via `alloy::net::TcpStream` and
`alloy::net::LwipAdapter`. See [NETWORK.md](NETWORK.md) for setup and
[examples/modbus_tcp_slave/](../examples/modbus_tcp_slave/) for a complete
working example on the SAME70 Xplained Ultra.

## Common gotchas

**Word order mismatch** — if your master reads float values as garbage, check
the word order setting. Try `WordOrder::CDAB` if your PLC uses Modicon
byte order.

**DE pin timing** — if the first byte of a slave response is missing, the DE
pin is being released too early. Verify that `flush()` waits for the TC
(transmit complete) interrupt before de-asserting.

**Baud-rate-specific inter-frame silence** — the RTU framer enforces a minimum
3.5-character silence between frames. At low baud rates (9600) this is ~4 ms;
at 115200 it is ~0.3 ms. The framer measures silence using the `now_us()`
callable injected at construction.

**CRC mismatch in tests** — the CRC table is pre-computed and stored as a
256-entry `constexpr` array. If you override the polynomial, verify the table
is recomputed consistently on both master and slave.

## Examples

| Example                  | Description                                      |
|--------------------------|--------------------------------------------------|
| `modbus_slave_basic`     | 10-var slave (int16/float/bool mix), poll loop   |
| `modbus_slave_rich`      | 30-var slave with full `VarMeta` + discovery FC  |
| `modbus_master_poll`     | Master polling 3 vars at mixed intervals         |
| `modbus_tcp_slave`       | Same 10-var slave over Modbus TCP port 502       |

See `examples/modbus_*/README.md` for per-example wiring and test scripts.
