# Clock HAL

Runtime clock-tree querying, kernel-clock mux switching, and full clock-profile
transitions — all type-safe and zero-overhead on the fast path.

---

## Peripheral Frequency Query

```cpp
#include "hal/clock/peripheral_frequency.hpp"

using P = alloy::device::runtime::PeripheralId;

auto result = alloy::hal::clock::peripheral_frequency<P::Usart2>();
if (result) {
    uint32_t hz = *result;  // e.g. 64_000_000
}
```

`peripheral_frequency<P>()` reads the live AHB/APB divider registers from
RCC at runtime and returns the bus clock for the peripheral.  No compile-time
constant required.

Returns `core::Err(core::ErrorCode::NotSupported)` when the device IR lacks a
`clock_tree` block.  A `static_assert` fires at the call site to guide the user
to add IR data (see [PORTING_NEW_PLATFORM.md](PORTING_NEW_PLATFORM.md#clock-tree--ir-section-requirements)).

---

## Auto-Baud UART

```cpp
#include "hal/uart/uart.hpp"

auto uart = alloy::hal::uart::open<P::Usart2>();
uart.set_baudrate_auto(115200);   // queries peripheral_frequency internally
```

`set_baudrate_auto` is equivalent to:

```cpp
auto clk = hal::clock::peripheral_frequency<P::Usart2>();
uart.set_baudrate(115200, *clk);
```

The two-argument `set_baudrate(bps, clock_hz)` overload is kept for backward
compatibility when a hardcoded clock constant is preferred.

---

## Auto-Clock SPI

```cpp
#include "hal/spi/spi.hpp"

auto spi = alloy::hal::spi::open<P::Spi1>();
spi.set_clock_speed_auto(8'000'000);   // 8 MHz — prescaler computed at runtime
```

Same pattern as UART.  Falls back to `core::ErrorCode::NotSupported` on
devices without clock-tree IR data.

---

## Kernel Clock Mux

Switch the kernel clock source for a peripheral (e.g. UART from PCLK to HSI16):

```cpp
#include "hal/clock/kernel_clock.hpp"

alloy::hal::clock::set_kernel_clock<P::Usart2>(
    alloy::hal::clock::KernelClockSource::hsi16
);
```

`set_kernel_clock` writes the mux field from
`ClockSemanticTraits<P>::kKernelClockMuxField`.  If that field is not valid
(peripheral has no mux or IR data missing) the call is a compile-time no-op
guarded by `if constexpr`.

`KernelClockSource` values (vendor-neutral):

| Value      | Meaning                   |
|------------|---------------------------|
| `pclk`     | APB peripheral bus clock  |
| `hsi16`    | HSI 16 MHz oscillator     |
| `lse`      | Low-speed external 32 kHz |
| `sysclk`   | System clock (post-PLL)   |
| `pll2_q`   | PLL2 Q output             |
| `hse`      | External oscillator       |

Vendor schemas map these to the 2–3 bit field values in RCC_CCIPR / CCIPR2.

---

## Clock Profile Switching

```cpp
#include "hal/clock/clock_profile.hpp"

// Switch to full-speed PLL (64 MHz on STM32G071).
alloy::hal::clock::switch_to_default_profile();

// Switch to safe low-power mode (HSI 4 MHz).
alloy::hal::clock::switch_to_safe_profile();

// Or use a named profile directly:
alloy::hal::clock::switch_profile(
    alloy::hal::clock::profiles::default_pll_64mhz
);
```

`switch_profile` follows the safe sequence:
1. Increase flash wait states.
2. Start / wait for PLL lock.
3. Switch SYSCLK source.
4. Reduce wait states (if stepping down).

Profiles are emitted by `alloy-cpp-emit` from the `clock_profiles` block in
the board IR.

---

## ClockSemanticTraits (codegen)

`ClockSemanticTraits<P>` is generated from the device IR `clock_tree` block.

```cpp
// Generated in <device>/generated/runtime/devices/<device>/clock_semantic_traits.hpp
template <>
struct ClockSemanticTraits<PeripheralId::Usart2> {
    static constexpr rt::FieldRef kKernelClockMuxField = { /* RCC_CCIPR bits 3:2 */ };
    static constexpr rt::FieldRef kEnableBit            = { /* RCC_APBENR1 bit 17 */ };
    static constexpr const char*  kBus                  = "apb1";
};
```

Adding a new device: supply `clock_tree` in the IR JSON; `alloy-cpp-emit`
handles the rest.  See [PORTING_NEW_PLATFORM.md](PORTING_NEW_PLATFORM.md#clock-tree--ir-section-requirements).

---

## Deprecation Notes

- `uart_handle::set_baudrate(bps, clock_hz)` — keep passing the explicit clock
  when no IR data exists.  Will be removed after clock-tree IR coverage reaches
  all tier-1 devices.
- `spi_handle::set_clock_speed(hz, kernel_hz)` — same deprecation path.

---

## Compile Test

`tests/compile_tests/test_clock_hal.cpp` exercises:
- `clock::peripheral_frequency<PeripheralId::none>()`
- `set_kernel_clock<PeripheralId::none>(KernelClockSource::pclk)`
- `switch_to_default_profile()` / `switch_to_safe_profile()`

Run: `cmake --build build --target compile_tests`.
