# lite-hal Spec Delta: Refactor Lite HAL Surface

## ADDED Requirements

### Requirement: Every Tier 2 lite driver SHALL expose `irq_number()` sourced from device data

Every `port<P>` or `controller<P>` in the Tier 2 lite HAL (UART, SPI, I2C, GPIO, Timer,
ADC, DAC, RTC, Watchdog, LPTIM) MUST expose:

```cpp
[[nodiscard]] static constexpr auto irq_number(std::size_t idx = 0u) noexcept
    -> std::uint32_t;

[[nodiscard]] static constexpr auto irq_count() noexcept -> std::size_t;
```

The value MUST be sourced from `P::kIrqLines[idx]` (alloy.device.v2.1 flat-struct).
When `P::kIrqLines` is absent (pre-v2.1 artifact), a `static_assert` MUST fire with a
human-readable upgrade message.  The implementation MUST use
`if constexpr (requires { P::kIrqLines[0]; })` so backward compatibility is preserved
at the concept level.

#### Scenario: IRQ number matches device artifact at compile time

- **WHEN** an application instantiates `using Uart1 = alloy::hal::uart::lite::port<dev::usart1>`
  on a device where `dev::usart1::kIrqLines[0]` is `27u`
- **THEN** `Uart1::irq_number()` is a `constexpr` expression that evaluates to `27u`
- **AND** `Nvic::enable_irq(Uart1::irq_number())` compiles and is identical in code-gen
  to `Nvic::enable_irq(27u)`

#### Scenario: Multi-IRQ peripheral exposes full count

- **WHEN** an application uses `controller<dev::tim1>` on a device where TIM1 has
  4 IRQ lines (CC1..4 + update separate)
- **THEN** `irq_count()` returns `4u` and `irq_number(0)` through `irq_number(3)` each
  return the correct NVIC vector number

#### Scenario: Pre-v2.1 device artifact triggers diagnostic

- **WHEN** `port<P>` is instantiated with a PeripheralSpec that lacks `kIrqLines`
- **THEN** the compiler emits a `static_assert` failure containing the text
  `"upgrade device artifact to v2.1"`

---

### Requirement: Tier 1 address-template drivers SHALL document their vendor scope

Every address-template lite driver (DMA, DMAMUX, EXTI, Flash, CRC, RNG, PWR, SYSCFG,
OPAMP, COMP) MUST include a Doxygen block at the top of the class that names the
supported vendor family and the specific register layout revision.

Each driver MUST also include a conditional `static_assert` block gated on
`ALLOY_ASSERT_VENDOR_STM32` that fires when `ALLOY_DEVICE_VENDOR_STM32` is not defined:

```cpp
#if defined(ALLOY_ASSERT_VENDOR_STM32)
  static_assert(ALLOY_DEVICE_VENDOR_STM32,
      "hal/dma/lite.hpp: DMA v1 layout is STM32-only.");
#endif
```

The guard MUST be opt-in (off by default) so existing cross-vendor projects are
not broken by default.

#### Scenario: STM32-only project enables the guard and compiles cleanly

- **WHEN** a CMakeLists.txt defines `-DALLOY_ASSERT_VENDOR_STM32=1 -DALLOY_DEVICE_VENDOR_STM32=1`
  and includes `hal/dma/lite.hpp`
- **THEN** the translation unit compiles without errors

#### Scenario: Wrong-vendor project catches mis-use at compile time

- **WHEN** a CMakeLists.txt defines `-DALLOY_ASSERT_VENDOR_STM32=1` but does NOT define
  `ALLOY_DEVICE_VENDOR_STM32`
- **THEN** the compiler emits a `static_assert` failure naming the STM32 restriction
  and the file that contains it

---

### Requirement: The lite UART driver SHALL dispatch to SAME70 UART register layout

When `P::kTemplate` equals `"uart"` (SAME70 UART / FLEXCOM-UART), the
`alloy::hal::uart::lite::port<P>` class MUST use the SAME70 register map
(`UART_BRGR`, `UART_CR`, `UART_MR`, `UART_RHR`, `UART_THR`, `UART_SR`) for all
register accesses instead of the STM32 SCI2/SCI3 offsets.

The public method surface (`configure()`, `write_byte()`, `read_byte()`,
`try_read_byte()`, `ready_to_send()`, `data_available()`, `enable_rx_irq()`,
`disable_rx_irq()`, `irq_number()`) MUST be identical between STM32 and SAME70 variants
so call sites are vendor-portable.

The dispatch MUST use `if constexpr (SamUart<P>)` — no virtual calls, no runtime
conditionals.

#### Scenario: SAME70 UART0 configures at 115200 baud

- **WHEN** `port<dev::uart0>` is instantiated on a SAME70 device where
  `dev::uart0::kTemplate == "uart"`
- **AND** `configure({.baudrate=115200u, .clock_hz=150'000'000u})` is called
- **THEN** `UART_BRGR` is written with `cd = 150'000'000 / 115200` (truncated)
- **AND** the STM32 BRR / CR1 / CR2 registers are never accessed

#### Scenario: STM32 USART is unaffected by SAME70 dispatch

- **WHEN** `port<dev::usart1>` is instantiated on an STM32G0 device
- **THEN** only the SCI3 register offsets (BRR, CR1, CR2, CR3) are written
- **AND** `SamUart<dev::usart1>` evaluates to `false` at compile time

#### Scenario: Unknown peripheral template fails to compile

- **WHEN** a PeripheralSpec is passed to `port<P>` whose `kTemplate` is neither
  `"usart"` nor `"uart"` (and no other supported concept matches)
- **THEN** the compiler emits a constraint-failure diagnostic referencing `AnyUart<P>`

---

### Requirement: Tier 2 drivers SHALL expose `clock_on()` / `clock_off()` sourced from `kRccEnable`

When `P::kRccEnable` is present, every Tier 2 `port<P>` / `controller<P>` MUST expose:

```cpp
static void clock_on()  noexcept requires (requires { P::kRccEnable; });
static void clock_off() noexcept requires (requires { P::kRccEnable; });
```

These MUST resolve the dotted path `P::kRccEnable` at compile time through a generated
`consteval find_rcc_gate()` lookup, producing the register address and bit mask with
zero runtime overhead.  The caller MUST NOT need to include the descriptor-runtime
`dev::peripheral_on<>()` path.

#### Scenario: `clock_on()` gates the correct RCC bit without runtime lookup

- **WHEN** an application calls `Uart1::clock_on()` where
  `dev::usart1::kRccEnable == "rcc.apbenr2.usart1en"`
- **THEN** the compiled output sets the `USART1EN` bit in `RCC_APBENR2`
  (address `0x40021034`, bit 2) with a single MMIO OR instruction
- **AND** no string comparison, no loop, and no indirection occur at runtime

#### Scenario: Peripheral without `kRccEnable` silently omits the methods

- **WHEN** `port<P>` is instantiated with a PeripheralSpec that has no `kRccEnable`
  member
- **THEN** `clock_on()` and `clock_off()` are not declared on the port type
- **AND** calling them results in a compile error, not a runtime failure

---

### Requirement: `hal/fdcan/lite.hpp` SHALL provide polling TX and RX for STM32 FDCAN

A new `template<std::uintptr_t FdcanBase>` driver class MUST expose:

- `configure(prescaler, tseg1, tseg2, sjw)` — programs NBTP, stays in init mode
- `start()` — clears CCCR.INIT, waits for INIT acknowledgement to clear
- `write_tx(arbitration_id, is_extended, data, dlc)→bool` — returns `false` when TX buffer is full
- `read_rx(out_id, out_is_ext, out_data, out_dlc)→bool` — returns `false` when RX FIFO 0 is empty
- `tx_pending()→bool`, `rx_available()→bool`

The driver MUST NOT use interrupts or DMA internally; it is a pure polling driver.
It MUST operate against the FDCAN register layout documented for STM32G4 / STM32H7.

#### Scenario: Polling loopback at 500 kbps on STM32G4

- **WHEN** `configure(1, 13, 2, 1)` (500 kbps with 16 MHz FDCAN clock) is called,
  followed by `start()`, and `write_tx(0x123, false, data, 8)` is called
- **AND** the CAN bus is connected in loopback or to a terminating peer
- **THEN** `rx_available()` becomes `true` and `read_rx(...)` returns the transmitted frame

#### Scenario: TX full returns false without blocking

- **WHEN** all TX buffers are occupied (TXFQS.TFQF=1)
- **THEN** `write_tx(...)` returns `false` immediately without busy-waiting
