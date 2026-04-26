# Alloy UART HAL

`alloy::hal::uart` is the typed, descriptor-backed UART abstraction. A
`port_handle<Connector>` is a zero-overhead value type that carries the
peripheral's full register map, a runtime config, and every capability method.

All vendor differences — ST SCI3 (modern: ISR/TDR/RDR), ST SCI2 (legacy:
SR/DR), Microchip UART-R, Microchip USART-ZW — are handled behind the same
API surface via `if constexpr` capability gates. Methods that have no backing
field return `core::ErrorCode::NotSupported`.

---

## Quick start

```cpp
#include "hal/uart.hpp"
#include "hal/connect/connector.hpp"

using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2,
                               alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3,
                               alloy::device::SignalId::signal_rx>>;

auto uart = alloy::hal::uart::open<DebugUartConnector>({
    .baudrate            = alloy::hal::Baudrate::e115200,
    .data_bits           = alloy::hal::DataBits::Eight,
    .parity              = alloy::hal::Parity::None,
    .stop_bits           = alloy::hal::StopBits::One,
    .flow_control        = alloy::hal::FlowControl::None,
    .peripheral_clock_hz = 64'000'000u,
});

uart.configure();
uart.write(std::as_bytes(std::span{"Hello\r\n"}));
```

The board layer provides `board::make_debug_uart()` which wraps the above.

---

## Core API

| Method | Returns | Notes |
|--------|---------|-------|
| `configure()` | `Result<void, ErrorCode>` | Full peripheral init from `UartConfig` |
| `write(span<const byte>)` | `Result<size_t, ErrorCode>` | Blocking polled write |
| `write_byte(byte)` | `Result<void, ErrorCode>` | Single-byte blocking write |
| `read(span<byte>)` | `Result<size_t, ErrorCode>` | Blocking polled read |
| `flush()` | `Result<void, ErrorCode>` | Wait for TX complete |
| `config()` | `const UartConfig&` | Read current config |
| `base_address()` | `uintptr_t` | Peripheral MMIO base |

---

## Phase 1: Baudrate / oversampling / kernel clock

### `kernel_clock_hz() -> uint32_t`

Returns `config().peripheral_clock_hz`. `noexcept`. Use to derive expected
baud rate error before calling `set_baudrate`.

```cpp
auto clk = uart.kernel_clock_hz();  // e.g. 64'000'000
```

### `set_baudrate(uint32_t bps) -> Result<void, ErrorCode>`

Computes and writes the BRR divisor without reconfiguring the whole peripheral.

Validation:
- BRR must fit in 16 bits.
- Realised baud rate must be within **±2%** of `bps` (`|realised - bps| * 100 <= bps * 2`).
- Returns `OutOfRange` on either failure; `NotSupported` on Microchip backends (Microchip has its own CD path, also validated).

```cpp
auto result = uart.set_baudrate(921'600u);
if (result.is_err()) {
    // ErrorCode::OutOfRange — clock can't hit 921600 ±2%
}
```

### `set_oversampling(Oversampling) -> Result<void, ErrorCode>`

Sets the `OVER8` bit in CR1 (ST-style only). The peripheral is briefly
disabled (UE=0) during the change and re-enabled immediately after.

```cpp
enum class Oversampling : uint8_t { X16, X8 };

uart.set_oversampling(Oversampling::X8);  // 8x — higher baud rate range
uart.set_oversampling(Oversampling::X16); // 16x — better noise tolerance
```

Returns `NotSupported` on Microchip backends.

---

## Phase 2: Status flags

All flag accessors return `bool`. On backends where the register field is
absent the call returns `false`.

| Method | Flag | ISR/SR bit |
|--------|------|-----------|
| `tx_complete()` | TC | ISR[6] / SR[6] |
| `tx_register_empty()` | TXE / TXFNF | ISR[7] / SR[7] |
| `rx_register_not_empty()` | RXNE / RXFNE | ISR[5] / SR[5] |
| `parity_error()` | PE | ISR[0] / SR[0] |
| `framing_error()` | FE | ISR[1] / SR[1] |
| `noise_error()` | NE | ISR[2] / SR[2] |
| `overrun_error()` | ORE | ISR[3] / SR[3] |
| `tx_fifo_full()` | TXFF | ISR[25] (G0/G4+) |
| `rx_fifo_empty()` | !RXFF | ISR[24] (G0/G4+) |
| `lin_break_detected()` | LBDF | ISR[8] / SR[8] |

Clear methods (`clear_parity_error()`, `clear_framing_error()`,
`clear_noise_error()`, `clear_overrun_error()`, `clear_lin_break_flag()`)
write the corresponding ICR bit on modern ST (ISR-style) peripherals.
Legacy ST (SR/DR) returns `NotSupported` — flags are cleared by reading DR.

---

## Phase 2: Interrupts

```cpp
enum class InterruptKind : uint8_t {
    Tc,               // CR1[6]  — transmission complete
    Txe,              // CR1[7]  — TX register / FIFO not full
    Rxne,             // CR1[5]  — RX register / FIFO not empty
    IdleLine,         // CR1[4]  — IDLE line detected
    LinBreak,         // CR2[6]  — LIN break detection (LBDIE)
    Cts,              // CR3[10] — CTS flag change
    Error,            // CR3[0]  — noise / framing / overrun error
    RxFifoThreshold,  // CR3[27] — RX FIFO threshold reached
    TxFifoThreshold,  // CR3[23] — TX FIFO threshold reached
};

uart.enable_interrupt(InterruptKind::IdleLine);
uart.disable_interrupt(InterruptKind::IdleLine);
```

Each kind is gated on the corresponding control field existing in the
runtime database. `NotSupported` is returned when absent (e.g.
`RxFifoThreshold` on STM32F4 which has no FIFO).

---

## Phase 2: FIFO

FIFO is present on STM32G0/G4/H7/WB (CR1 bit 29 = FIFOEN). Absent on
STM32F1/F4 and Microchip backends — all FIFO methods return `NotSupported`
on those families.

```cpp
enum class FifoTrigger : uint8_t {
    Empty = 0, Quarter = 1, Half = 2, ThreeQuarters = 3, Full = 4
};

uart.enable_fifo(true);
uart.set_tx_threshold(FifoTrigger::Half);     // CR3[31:29] TXFTCFG
uart.set_rx_threshold(FifoTrigger::Quarter);  // CR3[26:24] RXFTCFG

bool full  = uart.tx_fifo_full();   // ISR[25] TXFF
bool empty = uart.rx_fifo_empty();  // !ISR[24] RXFF
```

---

## Phase 3: Mode setters

### LIN

```cpp
uart.enable_lin(true);        // CR2[14] LINEN
uart.send_lin_break();        // RQR[1] SBKRQ — send a break character
bool detected = uart.lin_break_detected();  // ISR[8] LBDF
uart.clear_lin_break_flag();  // ICR[8] LBDCF
uart.enable_lin(false);
```

`send_lin_break()` uses the USART Request Register (RQR), discovered
at constexpr time by register suffix search. Returns `NotSupported`
on Microchip backends.

### RS-485 Driver Enable (DE)

```cpp
// Set assertion / de-assertion guard time in baud-clock sample units.
// CR1[25:21] DEAT, CR1[20:16] DEDT — range [0, 31].
uart.set_de_assertion_time(1u);
uart.set_de_deassertion_time(1u);
uart.enable_de(true);   // CR3[14] DEM — output DE signal on configured pin
```

### Half-duplex

```cpp
uart.set_half_duplex(true);   // CR3[3] HDSEL — TX pin is shared TX/RX
```

### Smartcard / IrDA

```cpp
uart.set_smartcard_mode(true);  // CR3[5] SCEN — ISO 7816 T=0/T=1
uart.set_irda_mode(true);       // CR3[1] IREN — IrDA SIR encoding
```

Both return `NotSupported` on Microchip USART-ZW (SAME70).

---

## DMA transfers

```cpp
// Configure channels first.
uart.configure_tx_dma(tx_dma_channel);
uart.configure_rx_dma(rx_dma_channel);

// Blocking DMA (poll for completion externally).
uart.write_dma(tx_dma_channel, tx_span);
uart.read_dma(rx_dma_channel, rx_span);
```

See [ASYNC.md](ASYNC.md) for the interrupt-driven async DMA wrappers
(`async::uart::write_dma`, `async::uart::read_dma`).

---

## Async — interrupt-driven single events

`async::uart::wait_for<Kind>(port)` arms the interrupt and returns an
`operation<uart_event::token<P, Kind>>` that signals when the ISR fires.
Canonical use-case: **IDLE-line end-of-frame detection** with circular DMA.

```cpp
#include "async.hpp"  // or "runtime/async_uart.hpp"

// 1. Arm the IDLE interrupt and start the DMA.
auto idle_op = async::uart::wait_for<InterruptKind::IdleLine>(uart);
uart.read_dma(rx_channel, big_rx_buffer);

// 2. Wait up to 100 ms for the line to go idle.
auto result = idle_op->wait_for<SysTickSource>(time::Duration::from_millis(100));

// 3. Disarm — prevent spurious re-trigger.
uart.disable_interrupt(InterruptKind::IdleLine);

// 4. Handle partial DMA buffer (bytes received = kBufferSize - DMA_NDTR).
```

The vendor ISR must call:
```cpp
alloy::runtime::uart_event::token<device::PeripheralId::USART2,
                                  InterruptKind::IdleLine>::signal();
```

Other useful kinds: `LinBreak` for LIN bus master detection, `Tc` for
half-duplex direction switching.

---

## Per-vendor capability matrix

| Feature | STM32G0/G4/H7/WB | STM32F1/F4 | SAME70 USART-ZW | SAME70 UART-R |
|---------|:-:|:-:|:-:|:-:|
| `set_baudrate` | ✓ | ✓ | ✓ | ✓ |
| `set_oversampling` | ✓ | ✓ | ✗ | ✗ |
| FIFO (`enable_fifo`, thresholds) | ✓ | ✗ | ✗ | ✗ |
| `tx_complete / tx_register_empty` | ✓ | ✓ | ✗ | ✗ |
| `parity/framing/noise/overrun` | ✓ | ✓ | ✗ | ✗ |
| `clear_*` flags | ✓ (ICR) | ✗ | ✗ | ✗ |
| `enable_interrupt` (Tc/Txe/Rxne) | ✓ | ✓ | ✗ | ✗ |
| `enable_lin` | ✓ | ✓ | ✗ | ✗ |
| `send_lin_break` | ✓ | ✓ | ✗ | ✗ |
| RS-485 DE | ✓ | ✓ | ✗ | ✗ |
| `set_half_duplex` | ✓ | ✓ | ✗ | ✗ |
| `set_smartcard_mode` | ✓ | ✓ | ✗ | ✗ |
| `set_irda_mode` | ✓ | ✓ | ✗ | ✗ |
| DMA (write_dma / read_dma) | ✓ | ✓ | ✓ | ✗ |
| `async::uart::wait_for<Kind>` | ✓ | ✓ | ✗ | ✗ |

✗ = returns `NotSupported` at runtime; feature gate does not exist on that family.

---

## Migration from modm / libopencm3

| modm | Alloy equivalent |
|------|-----------------|
| `Usart::setBaudrate(baud)` | `uart.set_baudrate(baud)` |
| `Usart::setWordLength(8)` | `UartConfig::data_bits = DataBits::Eight` |
| `Usart::enableInterruptVector(true, 5)` | `uart.enable_interrupt(InterruptKind::Rxne)` + NVIC in board ISR |
| `Usart::isReceiveRegisterNotEmpty()` | `uart.rx_register_not_empty()` |
| `Usart::getErrorFlags()` | `uart.parity_error()` + `uart.framing_error()` + … |
| `UartDma::write(buffer)` (async) | `async::uart::write_dma(uart, tx_dma, buffer)` |

---

## Related

- [ASYNC.md](ASYNC.md) — the runtime async model; UART DMA section + `wait_for`
- [COOKBOOK.md](COOKBOOK.md) — practical recipes
- `examples/uart_probe_complete/` — single-file demo of every lever
- `examples/async_uart_timeout/` — DMA + completion-timeout pattern
