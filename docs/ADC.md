# Alloy ADC HAL

`alloy::hal::adc` is the typed, descriptor-backed ADC abstraction. An
`adc::port_handle<Peripheral>` is a zero-overhead value type that carries the
peripheral's full register map, a runtime config, and every capability method.

All vendor differences — STM32 SAR ADC (single + sequence modes), Microchip
AFEC (per-channel enable/disable) — are handled behind the same API surface
via `if constexpr` capability gates. Methods that have no backing field return
`core::ErrorCode::NotSupported`.

---

## Quick start

### Single-shot read (STM32)

```cpp
#include "hal/adc.hpp"

auto adc = board::make_adc({.peripheral_clock_hz = board::kAdcClockHz});

if (adc.configure().is_err()) { /* handle */ }

adc.set_resolution(adc::Resolution::Bits12);
adc.start();
while (!adc.ready()) {}
auto result = adc.read();  // -> Result<uint32_t, ErrorCode>
```

### Single-shot read (SAME70 AFEC)

```cpp
adc.configure();
adc.enable_channel(0u);   // enable CH0
adc.start();
while (!adc.ready()) {}
auto result = adc.read();
```

The board layer provides `board::make_adc()` which wraps the above.

---

## Config

```cpp
struct Config {
    bool        enable_on_configure = true;   // call enable() at end of configure()
    bool        start_immediately   = false;  // call start() right after enable()
    std::uint32_t peripheral_clock_hz = 0u;
};
```

---

## Core API

| Method | Returns | Notes |
|--------|---------|-------|
| `configure()` | `Result<void, ErrorCode>` | Full peripheral init from `Config` |
| `enable()` | `Result<void, ErrorCode>` | Enable the ADC (power-on, calibrate) |
| `start()` | `Result<void, ErrorCode>` | Begin conversion(s) |
| `stop()` | `Result<void, ErrorCode>` | Stop continuous / scan |
| `ready() -> bool` | `bool` | True when EOC/EOS flag is set |
| `read()` | `Result<uint32_t, ErrorCode>` | Read single latest conversion |

---

## Resolution and alignment

```cpp
enum class Resolution : uint8_t { Bits6, Bits8, Bits10, Bits12, Bits14, Bits16 };
enum class Alignment  : uint8_t { Right, Left };

adc.set_resolution(adc::Resolution::Bits12);  // gated on has_resolution()
adc.set_alignment(adc::Alignment::Right);      // gated on has_alignment_field()
```

Not all resolutions are available on every device (e.g. STM32F4 ADC supports
6/8/10/12 bit, not 14/16). Returns `NotSupported` for unavailable values.

---

## Continuous mode

```cpp
adc.set_continuous(true);   // CR2[1] CONT — keep converting after each trigger
adc.start();
while (true) {
    if (adc.end_of_sequence()) {
        auto v = adc.read();
    }
}
adc.stop();
```

Returns `NotSupported` on backends without a continuous-mode field.

---

## Sample time

```cpp
// Per-channel sample time in ADC clock cycles.
// Channel indices match the typed Channel enum from alloy-codegen.
adc.set_sample_time(0u, 80u);    // CH0 — 80.5 cycles (slow; good for Vrefint)
adc.set_sample_time(1u, 7u);     // CH1 — 7.5 cycles (fast)
```

Returns `NotSupported` when the peripheral has no per-channel SMP register
(e.g. SAME70 AFEC uses a different timing model).

---

## Scan sequence (STM32-style)

```cpp
// Set the ordered scan order — up to kMaxSequenceLength channels.
std::array<uint8_t, 4> channels = {0u, 1u, 4u, 17u};  // 17 = Vrefint on G0
adc.set_sequence(std::span{channels});

// Blocking drain: reads all N conversions from the sequence in order.
std::array<uint16_t, 4> samples{};
adc.read_sequence(std::span{samples});

bool eos = adc.end_of_sequence();  // EOS flag — set after last channel in sequence
```

`set_sequence` returns `NotSupported` on SAME70 AFEC (use `enable_channel` instead).

---

## Per-channel enable (SAME70 AFEC-style)

```cpp
adc.enable_channel(0u);           // CHER bit 0
adc.enable_channel(1u);           // CHER bit 1
bool on = adc.channel_enabled(0u);
adc.disable_channel(1u);          // CHDR bit 1
```

Returns `NotSupported` on STM32 ADC (use `set_sequence` instead).

---

## Hardware trigger

```cpp
enum class TriggerEdge : uint8_t { Disabled, Rising, Falling, Both };

// Source index is vendor-specific: STM32G0 TIM3_TRGO = 3, TIM1_TRGO = 1, etc.
adc.set_hardware_trigger(3u, adc::TriggerEdge::Rising);  // arm HW trigger
// ... wait for conversions to complete on each trigger pulse ...
adc.set_hardware_trigger(0u, adc::TriggerEdge::Disabled);  // disarm
```

Returns `NotSupported` when the external trigger fields are absent.

---

## Overrun detection

```cpp
bool ore = adc.overrun();          // OVR / OVRE flag — previous result overwritten
adc.clear_overrun();               // write to clear; restart conversion if needed
```

---

## DMA

```cpp
// Enable DMA request generation for the ADC.
adc.enable_dma(/*circular=*/false);   // one-shot: stops after buffer full
adc.enable_dma(/*circular=*/true);    // circular: restarts at buffer start

// Configure the DMA channel (sets peripheral address, data width, etc.).
adc.configure_dma(dma_channel, /*circular=*/true);

// Polling DMA wait (see ASYNC.md for interrupt-driven variant).
adc.read_sequence(samples_span);
```

---

## Async — interrupt-driven

```cpp
#include "runtime/async_adc.hpp"

// Single conversion — waits on EOC interrupt.
auto single = async::adc::read<device::PeripheralId::ADC1>(adc);
uint32_t val = co_await single;

// DMA scan — waits on DMA transfer-complete; fills caller's buffer.
std::array<uint16_t, 4> buf{};
auto scan = async::adc::scan_dma<device::PeripheralId::ADC1>(adc, dma_ch, buf);
co_await scan;
```

The vendor ISR must call `adc_event::token<device::PeripheralId::ADC1>::signal()`.
See [ASYNC.md](ASYNC.md) for the full async model.

---

## Runtime capability checks

Each `port_handle` exposes compile-time capability guards:

```cpp
static_assert(Adc::has_resolution());       // true on STM32, false on some Microchip
static_assert(Adc::has_continuous());       // true on STM32 SAR
static_assert(Adc::has_sample_time());      // false on SAME70 AFEC
static_assert(Adc::has_sequence());         // true on STM32 (SQR registers)
static_assert(Adc::has_channel_enable());   // true on SAME70 AFEC
static_assert(Adc::has_hardware_trigger()); // true on STM32 with TRGO
static_assert(Adc::has_end_of_sequence());  // true when EOS field present
```

---

## Per-vendor capability matrix

| Feature | STM32G0/G4/H7 | STM32F1/F4 | SAME70 AFEC |
|---------|:-:|:-:|:-:|
| `configure / enable / start / stop / ready / read` | ✓ | ✓ | ✓ |
| `set_resolution` | ✓ | ✓ | ✓ |
| `set_alignment` | ✓ | ✓ | ✗ |
| `set_continuous` | ✓ | ✓ | ✓ |
| `set_sample_time` | ✓ | ✓ | ✗ |
| `set_sequence` | ✓ | ✓ | ✗ |
| `enable_channel / disable_channel` | ✗ | ✗ | ✓ |
| `set_hardware_trigger` | ✓ | ✓ | ✓ |
| `read_sequence` | ✓ | ✓ | ✓ |
| `end_of_sequence` | ✓ | ✓ | ✓ |
| `overrun / clear_overrun` | ✓ | ✓ | ✓ |
| `enable_dma / configure_dma` | ✓ | ✓ | ✓ |
| `async::adc::read` / `scan_dma` | ✓ | ✓ | ✓ |

✗ = returns `NotSupported` at runtime; feature gate absent on that family.

---

## Related

- [ASYNC.md](ASYNC.md) — runtime async model; ADC EOC + DMA scan sections
- [COOKBOOK.md](COOKBOOK.md) — practical recipes
- `examples/analog_probe_complete/` — single-file demo of every ADC lever
- `examples/analog_probe/` — simple single-shot temperature sensor read
