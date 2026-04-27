# Alloy SPI HAL

`alloy::hal::spi` is the typed, descriptor-backed SPI abstraction. A
`port_handle<Connector>` is a zero-overhead value type that carries the
peripheral's register map, a runtime config, and every capability method.

All vendor differences — ST SPI v3.3 (G0/G4/H7), ST SPI v2.2 (F4/F1 legacy),
Microchip SPI ZM (SAME70/SAMV) — are handled behind the same API surface via
`if constexpr` capability gates. Methods whose underlying field is not
published by the descriptor return `core::ErrorCode::NotSupported`.

---

## Quick start

```cpp
#include "hal/spi.hpp"
#include "hal/connect/connector.hpp"

using SpiBus = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5,
                                alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;

auto spi = alloy::hal::spi::open<SpiBus>({
    alloy::hal::SpiMode::Mode3,
    16'000'000u,                          // requested clock_speed
    alloy::hal::SpiBitOrder::MsbFirst,
    alloy::hal::SpiDataSize::Bits8,
    64'000'000u,                          // peripheral_clock_hz
});

spi.configure();
std::array<uint8_t, 4> tx{0x9F, 0, 0, 0};
std::array<uint8_t, 4> rx{};
spi.transfer(tx, rx);
```

The board layer provides `board::make_spi()` which wraps the above.

---

## Core API (unchanged)

| Method | Returns | Notes |
|--------|---------|-------|
| `configure()` | `Result<void, ErrorCode>` | Full peripheral init from `SpiConfig` |
| `transfer(span<const u8>, span<u8>)` | `Result<void, ErrorCode>` | Blocking full-duplex |
| `transmit(span<const u8>)` | `Result<void, ErrorCode>` | Blocking write-only |
| `receive(span<u8>)` | `Result<void, ErrorCode>` | Blocking read-only (sends 0xFF) |
| `is_busy()` | `bool` | True while transfer in progress |
| `config()` | `const SpiConfig&` | Read current config |
| `base_address()` | `uintptr_t` | Peripheral MMIO base |

---

## Phase 1: Variable data size, clock speed, kernel clock

### `kernel_clock_hz() -> uint32_t`

Returns `config().peripheral_clock_hz`. `noexcept`.

### `set_data_size(uint8_t bits) -> Result<void, ErrorCode>`

Sets the frame data size at runtime. Validates against the descriptor's
`kSupportedFrameSizes` list:

- **STM32G0** (`kDsField` 4 bits, supported = 4..16): accepts the full range.
- **STM32F4** (`kDffField` 1 bit, supported = {8, 16}): accepts only 8 / 16.
- **SAME70** (`kBitsField`, supported = 8..16): accepts 8..16.

Returns `InvalidParameter` for out-of-range or unsupported widths. The
templated overload `set_data_size_static<Bits>()` adds a compile-time
`static_assert(Bits >= 4 && Bits <= 16)` for compile-time-constant arguments.

```cpp
spi.set_data_size(12u);                 // OK on G0; InvalidParameter on F4
spi.set_data_size_static<8u>();         // compile-time validated
spi.set_data_size_static<3u>();         // static_assert fires
```

### `set_clock_speed(uint32_t hz) -> Result<void, ErrorCode>`

Resolves the BR / SCBR divider against `kernel_clock_hz()` and writes it.
Returns `InvalidParameter` when:

- `hz == 0` or `peripheral_clock_hz == 0`.
- The realised rate falls outside **±5 %** of the request.

Why ±5 %: STM32 prescalers are powers-of-2 (`/2 /4 /8 /16 …`), so realisable
rates leave large gaps. SPI peripherals are usually flexible on exact baud —
device datasheets specify a maximum, not an exact match.

```cpp
spi.set_clock_speed(16'000'000u);       // 64 MHz / 4 = 16 MHz, OK
spi.set_clock_speed( 5'000'000u);       // 64 MHz: realisable {1,2,4,8,16,32}
                                        // closest is 4 MHz (-20 %) → InvalidParameter
```

### `realised_clock_speed() -> uint32_t`

Reads the BR / SCBR field back from MMIO and returns the actual rate. Useful
to verify what `set_clock_speed` produced when the request and the realised
rate differ within the ±5 % envelope.

```cpp
spi.set_clock_speed(16'500'000u);       // 16 MHz realised, within ±5 %, OK
auto actual = spi.realised_clock_speed(); // 16'000'000
```

---

## Phase 2: Frame format, CRC, bidirectional, NSS management

### `set_frame_format(FrameFormat)`

`FrameFormat::Motorola` (default) or `FrameFormat::TI`. Gated independently
on `kSupportsMotorolaFrame` / `kSupportsTiFrame`. Microchip SPI is
permanently Motorola — `set_frame_format(Motorola)` returns `Ok` and
`TI` returns `NotSupported`.

### CRC (gated on `kSupportsCrc`)

```cpp
spi.enable_crc(true);
spi.set_crc_polynomial(0x1021u);        // CCITT (SD card / Bluetooth)
auto rxcrc = spi.read_crc();            // computed CRC after a transfer
if (spi.crc_error()) {
    spi.clear_crc_error();
}
```

SAME70 and Microchip SPI have `kSupportsCrc=false` — every CRC method
returns `NotSupported`.

### Bidirectional 3-wire (gated on `kSupportsBidirectional3Wire`)

STM32 SPI can run on a single MOSI line in bidirectional mode (the line
direction is software-controlled).

```cpp
spi.set_bidirectional(true);
spi.set_bidirectional_direction(BiDir::Receive);
// ... receive bytes ...
spi.set_bidirectional_direction(BiDir::Transmit);
// ... transmit bytes ...
```

### Hardware NSS management (gated on `kSupportsNssHwManagement`)

```cpp
spi.set_nss_management(NssManagement::HardwareOutput); // SSOE=1, SSM=0
spi.set_nss_pulse_per_transfer(true);                  // NSSP=1 (STM32 only)
```

`NssManagement::Software` (default) keeps SSM=1, SSI=1 (master-mode trick).
`HardwareInput` clears SSM and lets the NSS pin drive the slave-select.
`HardwareOutput` sets SSOE so the SPI peripheral drives NSS low while SPE is
active.

`set_nss_pulse_per_transfer(true)` enables NSSP — the NSS line pulses
between consecutive frames. STM32 G0 / F4 publish NSSP; older variants do
not (returns `NotSupported`).

---

## Phase 3: SAM-style per-CS timing

These setters are gated on the SAM SPI's per-CS timing fields. STM32 SPI
returns `NotSupported` for all four. Cycle counts are in raw
peripheral-clock cycles — convert microseconds via `kernel_clock_hz()`.

```cpp
spi.set_cs_decode_mode(false);                          // PCS direct
spi.set_cs_delay_between_consecutive(uint16_t{100});    // DLYBCT
spi.set_cs_delay_clock_to_active(uint16_t{50});         // DLYBS
spi.set_cs_delay_active_to_clock(uint16_t{50});         // DLYBCS
```

---

## Phase 4: Status flags, interrupts, IRQ vector lookup

### Status flags

| Method | STM32 source | SAM source |
|--------|--------------|------------|
| `tx_register_empty()` | SR.TXE | SR.TDRE |
| `rx_register_not_empty()` | SR.RXNE | SR.RDRF |
| `busy()` | SR.BSY | !SR.TXEMPTY |
| `mode_fault()` | SR.MODF | — |
| `frame_format_error()` | SR.FRE | — |

```cpp
if (spi.mode_fault()) {
    static_cast<void>(spi.clear_mode_fault());     // STM32 read-SR + write-CR1
}
```

### Typed interrupts

```cpp
enum class InterruptKind : uint8_t {
    Txe, Rxne, Error, ModeFault, CrcError, FrameError,
};

spi.enable_interrupt(InterruptKind::Rxne);
// ... handler runs ...
spi.disable_interrupt(InterruptKind::Rxne);
```

STM32 muxes `Error / ModeFault / CrcError / FrameError` through the single
ERRIE bit. SAM exposes per-source IER/IDR. Unsupported kinds return
`NotSupported` (e.g. `CrcError` on a peripheral with `kSupportsCrc=false`).

### `static constexpr irq_numbers() -> span<const uint32_t>`

Returns the descriptor's `kIrqNumbers` for the bound peripheral. Use this
to register ISR handlers without hardcoding NVIC line IDs:

```cpp
constexpr auto irqs = decltype(spi)::irq_numbers();
static_assert(irqs.size() >= 1u);
NVIC_EnableIRQ(static_cast<IRQn_Type>(irqs[0]));
```

---

## Async wiring

The DMA wrappers from `complete-async-hal` compose unchanged with the
extended HAL surface:

```cpp
#include "async.hpp"
auto op = alloy::async::spi::transfer_dma(spi, tx_ch, rx_ch, tx, rx);
op.wait_until<SysTickSource>(deadline);
```

`async::spi::wait_for<Kind>(port)` lets a coroutine `co_await` an
interrupt-driven event without polling:

```cpp
auto op = alloy::async::spi::wait_for<InterruptKind::CrcError>(spi);
op.wait_for<SysTickSource>(time::Duration::from_millis(100));
spi.disable_interrupt(InterruptKind::CrcError);
```

The vendor SPI ISR is responsible for calling
`spi_event::token<P, InterruptKind::CrcError>::signal()` when the IRQ fires.

---

## Migration: from raw register hex

Common hex patterns that can now be expressed via the HAL:

| Goal | Old (raw register write) | New (HAL call) |
|------|--------------------------|----------------|
| 12-bit frame on G0 | `SPI1->CR2 \|= (0xB << 8)` | `spi.set_data_size(12)` |
| Set 16 MHz from 64 MHz | `SPI1->CR1 \|= (0x1 << 3)` | `spi.set_clock_speed(16'000'000)` |
| TI frame format | `SPI1->CR2 \|= (1u << 4)` | `spi.set_frame_format(FrameFormat::TI)` |
| Enable CRC + CCITT poly | `SPI1->CR1 \|= (1u << 13); SPI1->CRCPR = 0x1021;` | `spi.enable_crc(true); spi.set_crc_polynomial(0x1021)` |
| Hardware NSS output + NSSP | `SPI1->CR2 \|= (1u << 2) \| (1u << 3)` | `spi.set_nss_management(HardwareOutput); spi.set_nss_pulse_per_transfer(true)` |
| RXNE interrupt | `SPI1->CR2 \|= (1u << 6)` | `spi.enable_interrupt(InterruptKind::Rxne)` |

---

## See also

- [`docs/ASYNC.md`](ASYNC.md) — interrupt-driven SPI wait_for + DMA wiring
- [`docs/COOKBOOK.md`](COOKBOOK.md) — common SPI recipes
- [`docs/SUPPORT_MATRIX.md`](SUPPORT_MATRIX.md) — per-board SPI tier
- [`examples/spi_probe_complete/`](../examples/spi_probe_complete/) — exercises every lever
