# Refactor Lite HAL Surface

## Status
`open` — architectural groundwork required before multi-vendor expansion.

## Why

The lite HAL (`hal/*/lite.hpp`) has grown to 21 drivers across three sessions but
accumulated three structural debts:

### 1. Three inconsistent patterns coexist

| Pattern | Drivers | Problem |
|---------|---------|---------|
| Core-fixed (hardcoded address) | NVIC, SysTick | Correct for Cortex-M core peripherals; no issue |
| Address-template `<uintptr_t Base>` | Flash, PWR, CRC, RNG, EXTI, SYSCFG, DMA, DMAMUX, OPAMP, COMP | **Implicitly STM32-specific** with no compile-time vendor check; silently wrong if used with SAME70 or nRF52 |
| PeripheralSpec-gated `<typename P>` | UART, SPI, I2C, GPIO, Timer, ADC, DAC, RTC, Watchdog, LPTIM | Correct portable pattern; `kTemplate`+`kIpVersion` strings gate correctness |

A user who picks up the library for the first time sees `controller<0x40020000u>` in the
DMA driver and cannot tell from the API that this will silently produce incorrect behavior
on a non-STM32 part.

### 2. Generated device data is unused

Every peripheral in `peripheral_traits.h` (v2.1 flat-struct) emits:
```cpp
inline constexpr unsigned kIrqLines[]  = { 27 };
inline constexpr const char *kIrqNames[] = { "USART1_IRQHandler" };
inline constexpr unsigned kIrqCount    = 1;
inline constexpr const char *kSignals[] = { "ck", "cts", "de", "rx", "tx" };
inline constexpr unsigned kSignalCount = 7;
```

Lite drivers ignore all of this.  A user wiring up an ISR must hand-copy IRQ number 27
from the reference manual into `Nvic::enable_irq(27u)` — despite the generated data
already encoding that mapping.  This defeats the purpose of code generation.

### 3. No path for multi-vendor peripherals

The `expand-chip-coverage` proposal plans to add SAME70 (already partially supported),
nRF52840, RP2350, SAMD21, and others.  Each vendor has its own register layout for
UART, SPI, GPIO, etc.  The current lite UART driver only handles STM32 SCI2/SCI3; a
SAME70 UART0 would produce a compilation error.  There is no specification for how a
lite driver should extend to a new vendor: which concepts to add, which methods are
mandatory vs. vendor-specific, what the dispatch mechanism is.

## What Changes

### A — Tier taxonomy (formal + documented)

Define three driver tiers in `src/hal/README.md` and `design.md`:

- **Tier 0 — Core-fixed** (`NVIC`, `SysTick`): fixed Cortex-M addresses, no
  peripheral concept, Cortex-M only.  Suitable as-is.
- **Tier 1 — Address-template** (`Flash`, `PWR`, `CRC`, `RNG`, `EXTI`, `SYSCFG`,
  `DMA`, `DMAMUX`, `OPAMP`, `COMP`): `template<std::uintptr_t Base>`.  Each driver
  SHALL document the supported vendor/layout in the file header and SHALL add a
  `static_assert` or concept guard that fires when the wrong vendor is selected.
- **Tier 2 — PeripheralSpec-gated** (`UART`, `SPI`, `I2C`, `GPIO`, `Timer`, `ADC`,
  `DAC`, `RTC`, `Watchdog`, `LPTIM`): `template<typename P> requires StXxx<P>`.
  The vendor concept (`StXxx`) is the single source of correctness.

### B — Device-data bridge for Tier 2

Every Tier 2 driver SHALL expose compile-time device-data accessors:

```cpp
// When P has kIrqLines[] (all current v2.1 flat-struct peripherals do):
[[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
    -> std::uint32_t;

// When P has kIrqCount > 0:
[[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t;
```

These replace hard-coded IRQ numbers in user code:
```cpp
// Before:
Nvic::enable_irq(27u);           // user must look up 27 from RM

// After:
Nvic::enable_irq(Uart1::irq_number());  // generated, correct for any board
```

The `PeripheralSpec` concept SHALL be extended with an optional `kIrqLines` check
(using `if constexpr (requires { P::kIrqLines; })`) so the method compiles on any
`PeripheralSpec` regardless of whether the field is present.

### C — Multi-vendor Tier 2 extensions

When `expand-chip-coverage` adds a new vendor's device artifacts, the Tier 2 driver
for each peripheral SHALL be extended with:

1. A new `consteval` variant detector (e.g., `is_sam_uart(kTemplate, kIpVersion)`)
2. A new named concept (e.g., `SamUart`)
3. An `if constexpr` dispatch branch for the vendor-specific register layout
4. The same public API surface that STM32 already exposes

**SAME70 UART0** — targeted for Phase 2 of this change:
- Template `"uart"`, ipversion `"uart_sam70_*"` or similar
- USARTs: separate CR/MR/BRGR/THR/RHR register set
- Same public methods: `configure()`, `write_byte()`, `read_byte()`, `try_read_byte()`,
  `ready_to_send()`, `data_available()`, `enable_rx_irq()`, `irq_number()`

**nRF52 UARTE** — targeted for Phase 3:
- Template `"uarte"`, ipversion starts with `"uarte0_*"`  
- EasyDMA-based TX/RX; no byte-level FIFO
- Subset API: `configure()`, `start_tx_dma()`, `start_rx_dma()`, `irq_number()`

### D — `kRccEnable` compile-time gate

The flat-struct encodes clock gate paths as dotted strings (`"rcc.apbenr2.usart1en"`).
Lite drivers currently require the caller to call `dev::peripheral_on<P>()` separately.

A compile-time RCC register lookup table SHALL be generated (one entry per dotted path,
mapped to a `{register_address, bit_mask}` pair) so that Tier 2 drivers can optionally
call `clock_on()` / `clock_off()` without depending on the full descriptor runtime.

This is a **Phase 3** item (depends on codegen additions to emit the lookup table).

### E — Missing lite drivers scope

The following peripherals have a descriptor-runtime HAL but no lite driver.  This change
defines the acceptance gate for each:

| Peripheral | Lite driver | Scope |
|------------|-------------|-------|
| `fdcan` | `hal/fdcan/lite.hpp` | **Phase 2** — STM32 FDCAN (G4/H7), polling TX+RX |
| `pwm` | use `hal/timer/lite.hpp` | `configure_pwm()` already in timer/lite — no separate file needed |
| `qspi` | deferred | complex flash protocol; out of scope for lite tier |
| `usb` | deferred | protocol stack required; not suitable for lite pattern |
| `eth` | deferred | MAC+PHY complexity; not suitable for lite pattern |
| `sdmmc` | deferred | controller complexity; not suitable for lite pattern |

## What Does NOT Change

- The `hal/*.hpp` descriptor-runtime HAL — no regressions; changes are additive.
- Existing lite driver APIs — all current method signatures are preserved.
- The `PeripheralSpec` concept base requirements — new optional fields are checked
  with `if constexpr (requires {...})` to remain backward compatible.
- The address-template pattern for Tier 1 drivers — the addresses and register layouts
  stay as-is; only vendor-hint documentation and optional assertions are added.

## Alternatives Considered

**Keep three patterns as-is:** The inconsistency compounds as more families are added.
Each new vendor contributor would need to decide the pattern independently.

**Collapse all drivers to address-template:** Removes compile-time safety from the
already-working Tier 2 concept gates.  Backslide.

**Full runtime polymorphism per vendor:** Defeats the zero-cost static abstraction goal
and the whole point of the lite tier.
