# Proposal: Clock Management HAL

## Status
`open` — required before UART/SPI baud auto-calculation.

## Problem

Clock-dependent HAL modules (UART, SPI, I2C, Timer) hardcode peripheral clock
frequencies or require the application to pass them manually:

```cpp
// Today: baud divisor computed from a hardcoded constant in board_config.hpp
constexpr uint32_t kApb1ClockHz = 36_000_000;
uart.set_baud(115200, kApb1ClockHz);  // fragile: breaks if clock profile changes
```

Three problems:
1. **Stale constant**: switching clock profiles (e.g. low-power 8 MHz vs normal
   64 MHz) requires manual update of `kApb1ClockHz` — no link between HAL and
   the active profile.
2. **Duplicate configuration**: clock frequencies appear in `board_config.hpp`,
   the codegen IR, and sometimes hardcoded in driver code — three sources diverge.
3. **No HAL API for peripheral clock source selection**: UART3 on STM32 can be
   clocked by APB1, HSI16, LSE, or SYSCLK, but no alloy HAL exposes the kernel
   clock mux — the user must write to RCC registers directly.

## Proposed Solution

### `hal::clock::peripheral_frequency<PeripheralId>()` — compile-time-capable query

```cpp
// src/hal/clock/peripheral_frequency.hpp
namespace alloy::hal::clock {

/// Returns the current peripheral bus clock in Hz.
/// Value is computed from the clock-tree semantic traits; can be constexpr if
/// the active profile is known at compile time.
template <device::PeripheralId P>
[[nodiscard]] auto peripheral_frequency() -> core::Result<uint32_t, core::ErrorCode>;

}  // namespace alloy::hal::clock
```

Usage in HAL (UART baud example):

```cpp
// src/hal/uart/uart_handle.hpp
auto set_baud(uint32_t baud) const -> core::Result<void, core::ErrorCode> {
    auto freq = clock::peripheral_frequency<Peripheral::id>();
    if (!freq.ok()) return core::Err(freq.unwrap_err());
    const uint32_t div = freq.unwrap() / baud;
    // write BRR field ...
}
```

### `hal::clock::set_kernel_clock<PeripheralId>(KernelClockSource)` — mux selection

```cpp
// src/hal/clock/kernel_clock.hpp
namespace alloy::hal::clock {

enum class KernelClockSource : uint8_t {
    pclk,    // peripheral bus clock (default)
    hsi16,
    lse,
    sysclk,
    pll2_q,
    // ... vendor-specific sources via extension point
};

template <device::PeripheralId P>
auto set_kernel_clock(KernelClockSource src)
    -> core::Result<void, core::ErrorCode>;

}
```

Only compiles when `ClockSemanticTraits<P>::kKernelClockMuxField.valid` is true.
Falls back to `core::Err(core::ErrorCode::NotSupported)` otherwise.

### `hal::clock::switch_profile(const ClockProfile&)` — dynamic profile switching

```cpp
// src/hal/clock/clock_profile.hpp
namespace alloy::hal::clock {

struct ClockProfile {
    uint32_t    sysclk_hz;
    uint8_t     ahb_div;      // AHB prescaler (1, 2, 4, 8, 16, 64, 128, 256, 512)
    uint8_t     apb1_div;
    uint8_t     apb2_div;
    uint8_t     flash_wait;   // wait states
    // Source: hsi, hse, pll (encoded in source field)
    uint8_t     source;
    uint32_t    pll_m, pll_n, pll_r;  // ignored when source != pll
};

/// Switches the system clock to the given profile.
/// Adjusts flash wait states before increasing and after decreasing frequency.
/// Returns Err(NotSupported) on targets without PLL.
auto switch_profile(const ClockProfile& profile)
    -> core::Result<void, core::ErrorCode>;

/// Named profiles from generated clock_profiles.hpp
namespace profiles {
    extern const ClockProfile default_pll_64mhz;
    extern const ClockProfile low_power_hsi_4mhz;
    extern const ClockProfile max_freq;
}

}
```

### ClockSemanticTraits — codegen side

The code generator emits `ClockSemanticTraits<PeripheralId>` from IR clock-tree
data:

```cpp
// Generated for STM32G0:
template <>
struct ClockSemanticTraits<PeripheralId::Usart2> {
    static constexpr RuntimeFieldRef kKernelClockMuxField = { RCC_CCIPR, 2u, 2u, true };
    static constexpr RuntimeRegisterRef kEnableReg = { RCC_APBENR1, 17u, 1u, true };
    // peripheral_frequency() reads SYSCLK/AHB/APB dividers at runtime
};
```

### Backward compatibility

`set_baud(uint32_t baud, uint32_t pclk_hz)` overload kept with
`[[deprecated("pass no pclk_hz; HAL queries clock automatically")]]`.
Removed in the next semver minor after all board configs migrated.

## Impact

- UART `set_baud` no longer needs `pclk_hz` argument.
- SPI prescaler calculation becomes automatic.
- Low-power firmware can call `switch_profile(profiles::low_power_hsi_4mhz)`
  and all subsequent baud calculations self-correct.
