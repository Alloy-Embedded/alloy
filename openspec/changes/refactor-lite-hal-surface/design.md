# Design: Refactor Lite HAL Surface

## Driver Tier Model

```
┌─────────────────────────────────────────────────────────────────┐
│                     alloy lite HAL tiers                        │
│                                                                 │
│  Tier 0 — Core-fixed          Tier 1 — Address-template        │
│  ┌──────────────────┐         ┌──────────────────────────────┐  │
│  │  NVIC, SysTick   │         │  Flash, PWR, CRC, RNG, EXTI  │  │
│  │  (Cortex-M only) │         │  SYSCFG, DMA, DMAMUX,        │  │
│  │  hardcoded addr  │         │  OPAMP, COMP                 │  │
│  │  no PeriphSpec   │         │  template<uintptr_t Base>    │  │
│  └──────────────────┘         │  vendor-specific, documented │  │
│                               └──────────────────────────────┘  │
│                                                                 │
│  Tier 2 — PeripheralSpec-gated                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  UART, SPI, I2C, GPIO, Timer, ADC, DAC, RTC,            │    │
│  │  Watchdog, LPTIM                                        │    │
│  │                                                         │    │
│  │  template<typename P> requires StXxx<P>                 │    │
│  │  ├─ StModernUsart (sci3_*)  ── STM32G0/G4/F3/H7/L4/WB  │    │
│  │  ├─ StLegacyUsart (sci2_*)  ── STM32F0/F1              │    │
│  │  ├─ [future] SamUart        ── SAME70 UART/USART        │    │
│  │  └─ [future] NrfUarte       ── nRF52 UARTE              │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

## Device-Data Bridge

### What is already in the flat-struct (v2.1)

Every peripheral instance in `peripheral_traits.h` emits:
```cpp
namespace usart1 {
  // --- already present, unused by lite ---
  inline constexpr unsigned    kIrqLines[]  = { 27 };
  inline constexpr const char *kIrqNames[]  = { "USART1_IRQHandler" };
  inline constexpr unsigned    kIrqCount    = 1;
  inline constexpr const char *kSignals[]   = { "ck","cts","de","rx","tx" };
  inline constexpr unsigned    kSignalCount = 7;
  inline constexpr const char *kRccEnable   = "rcc.apbenr2.usart1en";
  inline constexpr const char *kRccReset    = "rcc.apbrstr2.usart1rst";
  inline constexpr const char *kKernelClockMux = "rcc.ccipr.usart1sel";
}
```

### `irq_number()` implementation pattern

```cpp
// In each Tier 2 port<P> class:

/// Returns the NVIC IRQ line for this peripheral.
/// Sources from P::kIrqLines[idx] (flat-struct v2.1).
/// Compiles to a constexpr constant; zero overhead.
[[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
    -> std::uint32_t {
    if constexpr (requires { P::kIrqLines[0]; }) {
        return static_cast<std::uint32_t>(P::kIrqLines[idx]);
    } else {
        static_assert(sizeof(P) == 0,
            "P::kIrqLines not present; upgrade device artifact to v2.1");
        return 0u;
    }
}

[[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t {
    if constexpr (requires { P::kIrqCount; }) {
        return static_cast<std::size_t>(P::kIrqCount);
    }
    return 0u;
}
```

### Usage change (before → after)

```cpp
// Before — user looks up IRQ in RM, hard-codes it:
using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
Nvic::enable_irq(27u);          // 27 is USART1_IRQn on G0 — must manually verify

// After — IRQ comes from device data:
using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
Nvic::enable_irq(Uart1::irq_number());  // always correct for the selected device
```

## Multi-Vendor Concept Extension

### SAME70 UART dispatch

The SAME70 UART peripheral has template `"uart"` and a different register layout:

```
UART_CR  +0x00 — Control (RSTRX, RSTTX, RXEN, TXEN, RSTSTA, STTBRK, STPBRK)
UART_MR  +0x04 — Mode (PAR, BRSRCCK, FILTER, LOOP)
UART_IER +0x08 — Interrupt enable
UART_IDR +0x0C — Interrupt disable
UART_IMR +0x10 — Interrupt mask
UART_SR  +0x14 — Status (RXRDY, TXRDY, ENDRX, ENDTX, OVRE, FRAME, PARE, TXEMPTY)
UART_RHR +0x18 — Receive Holding Register
UART_THR +0x1C — Transmit Holding Register
UART_BRGR +0x20 — Baud Rate Generator (CD field [15:0])
```

Extension to `uart/lite.hpp`:

```cpp
// New consteval detector:
namespace detail {
  [[nodiscard]] consteval auto is_sam_uart(const char* tmpl) -> bool {
      return std::string_view{tmpl} == "uart";  // SAME70 UART/FLEXCOM-UART
  }
}

// New concept:
template <typename P>
concept SamUart =
    device::PeripheralSpec<P> &&
    requires { { P::kTemplate } -> std::convertible_to<const char*>; } &&
    detail::is_sam_uart(P::kTemplate);

// New concept that unifies all UART variants:
template <typename P>
concept AnyUart = StUsart<P> || SamUart<P>;
```

The `port<P>` class gains a `requires AnyUart<P>` constraint (replacing `StUsart<P>`)
and uses `if constexpr` to dispatch:

```cpp
static void configure(const Config& cfg) noexcept {
    if constexpr (StModernUsart<P>) {
        // existing SCI3 path
    } else if constexpr (StLegacyUsart<P>) {
        // existing SCI2 path
    } else if constexpr (SamUart<P>) {
        // SAME70 path: write UART_BRGR, UART_CR, UART_MR
        const auto cd = static_cast<std::uint32_t>(cfg.clock_hz / cfg.baudrate);
        reg(kSamBrgrOfs) = cd & 0xFFFFu;
        reg(kSamCrOfs) = kSamCrRstRx | kSamCrRstTx | kSamCrRstSta;
        reg(kSamCrOfs) = kSamCrRxEn | kSamCrTxEn;
    }
}
```

### nRF52 UARTE dispatch

nRF52 UARTE is EasyDMA-based — no single-byte FIFO.  The lite API differs:

```cpp
// nRF52 provides DMA-start methods instead of byte-level accessors:
static void start_rx_dma(std::uint8_t* buf, std::uint16_t len) noexcept;
static void start_tx_dma(const std::uint8_t* buf, std::uint16_t len) noexcept;
```

These methods are `requires NrfUarte<P>` — they don't exist on STM32 variants.
Calling an STM32-only method on an nRF52 peripheral is a compile error, not a
silent wrong behavior.

## Tier 1 Vendor-Hint Pattern

Address-template drivers SHALL add a `kVendorFamily` documentation constant and a
compile-time assertion that can be enabled per-project:

```cpp
// In hal/dma/lite.hpp, template<std::uintptr_t DmaBase>:

/// Supported layout: STM32 DMA v1 (F0/F3/G0/G4/L4/WB).
/// Register offsets are identical across all listed families.
/// Do NOT use this driver for SAME70 XDMAC, nRF52 DMA, or RP2040 DMA —
/// they have incompatible register maps.
/// If ALLOY_ASSERT_VENDOR_STM32 is defined, a static_assert fires at
/// instantiation time when the project is not an STM32 target.
#if defined(ALLOY_ASSERT_VENDOR_STM32)
  static_assert(ALLOY_DEVICE_VENDOR_STM32,
      "hal/dma/lite.hpp: DMA v1 layout is STM32-only. "
      "Use the vendor-specific DMA driver for your target.");
#endif
```

## `kRccEnable` Compile-Time Lookup (Phase 3)

The dotted string `"rcc.apbenr2.usart1en"` encodes `(register_path, bit_name)`.
A generated lookup table maps each string to `{uint32_t address, uint32_t mask}`:

```cpp
// auto-generated: src/device/rcc_gate_table.hpp
namespace alloy::device::detail {
struct RccGate { std::uint32_t addr; std::uint32_t mask; };
constexpr RccGate kRccGates[] = {
    { 0x40021034u, 1u << 2 },  // "rcc.apbenr2.usart1en"
    { 0x40021034u, 1u << 3 },  // "rcc.apbenr2.usart2en"
    // ...
};
// lookup by string at compile time using consteval
consteval RccGate find_rcc_gate(const char* dotted_path);
}
```

Lite drivers then expose:

```cpp
// In port<P>:
static void clock_on() noexcept
    requires (requires { P::kRccEnable; })
{
    constexpr auto gate = device::detail::find_rcc_gate(P::kRccEnable);
    *reinterpret_cast<volatile std::uint32_t*>(gate.addr) |= gate.mask;
}
```

This removes the dependency on `dev::peripheral_on<>()` for users who want pure
lite-path code.

## Flat-Struct Extensions Required (Codegen)

The following fields are needed by Phase 2 but not yet in the flat-struct:

| Field | Type | Example | Used by |
|-------|------|---------|---------|
| `kDmaRxRequest` | `unsigned` | `29u` (USART1_RX on G0) | DMA auto-configure |
| `kDmaTxRequest` | `unsigned` | `28u` (USART1_TX on G0) | DMA auto-configure |

The IRQ fields (`kIrqLines`, `kIrqNames`, `kIrqCount`) are already emitted by v2.1.

## File Map

```
src/hal/
├── README.md                     ← NEW: tier taxonomy + extension guide
├── uart/
│   └── lite.hpp                  ← ADD: irq_number(), irq_count(); EXTEND: SamUart concept
├── spi/
│   └── lite.hpp                  ← ADD: irq_number(), irq_count()
├── i2c/
│   └── lite.hpp                  ← ADD: irq_number(), irq_count()
├── gpio/
│   └── lite.hpp                  ← ADD: irq_number() (GPIO EXTI line = pin number, device-specific)
├── timer/
│   └── lite.hpp                  ← ADD: irq_number(), irq_count()
├── adc/
│   └── lite.hpp                  ← ADD: irq_number()
├── dac/
│   └── lite.hpp                  ← ADD: irq_number()
├── rtc/
│   └── lite.hpp                  ← ADD: irq_number(), irq_count()
├── watchdog/
│   └── lite.hpp                  ← ADD: irq_number()
├── lptim/
│   └── lite.hpp                  ← ADD: irq_number()
├── dma/
│   └── lite.hpp                  ← ADD: vendor-hint comment + ALLOY_ASSERT_VENDOR_STM32 gate
├── exti/
│   └── lite.hpp                  ← ADD: vendor-hint
├── flash/
│   └── lite.hpp                  ← ADD: vendor-hint
├── crc/
│   └── lite.hpp                  ← ADD: vendor-hint
├── rng/
│   └── lite.hpp                  ← ADD: vendor-hint
├── pwr/
│   └── lite.hpp                  ← ADD: vendor-hint
├── syscfg/
│   └── lite.hpp                  ← ADD: vendor-hint
├── opamp/
│   └── lite.hpp                  ← ADD: vendor-hint
├── comp/
│   └── lite.hpp                  ← ADD: vendor-hint
└── fdcan/
    └── lite.hpp                  ← NEW (Phase 2): STM32 FDCAN
```
