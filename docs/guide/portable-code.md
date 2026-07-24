# Portable code

alloy's headline promise is that **one `main.cpp` recompiles for any board, with zero
preprocessor conditionals**. This page explains the three ideas that make that true.

## Board roles

A board provides a fixed set of **roles** — named objects your app talks to instead of raw
peripherals:

| Role | What it is |
| --- | --- |
| `board::led` | the status LED |
| `board::button` | the user button |
| `board::debug_uart` | the console UART |
| `board::i2c`, `board::spi` | the wired bus |
| `board::adc`, `board::led_pwm` | analog in / PWM out |
| `board::watchdog`, `board::rtc`, `board::nvm` | system services |

Because every app names roles, not chip-specific peripherals, the source never mentions a pin,
an address, or a vendor. The board data binds each role to the right pins with **compile-time
route checking**.

```cpp
board::init();
board::led.toggle();
auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
```

## Capabilities and `if constexpr`

Not every board has every role. Instead of `#ifdef`, alloy exposes compile-time booleans under
`board::caps::*` and you branch with `if constexpr`:

```cpp
if constexpr (board::caps::button) {
    if (board::button.pressed()) {
        board::led.on();
    }
}
```

The discarded branch is **removed at compile time**, so code guarded this way costs nothing on
boards that lack the role — and the file compiles identically everywhere.

!!! info "Roles that are absent"
    A small role you don't use degrades to a **no-op stub** (so `board::watchdog.feed()` always
    compiles). Using a role a board *truly* lacks — say `board::led` on a headless board —
    fails at **compile time** with a `static_assert` message, never at runtime.

## Errors you can read

Portability is only useful if mistakes are caught early. Bind a UART to a pin that has no route
to it and the compiler stops you, **naming the pin**:

```
error: static assertion failed: TX pin has no route to this UART on the selected chip
   note: in instantiation of 'uart::bind<usart2_t, tx<pa5_t>, rx<pa3_t>, ...>'
```

This "wrong route → readable compile error" is a CI-enforced acceptance test, not a
nice-to-have — see [Architecture](../reference/architecture.md).

## Putting it together

```cpp title="A portable app that adapts to what the board offers"
#include <alloy/board.hpp>
using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("up\r\n");

    while (true) {
        board::led.toggle();

        if constexpr (board::caps::adc) {           // only where an ADC role exists
            auto adc = board::adc::open();
            uart.write("sample ok\r\n");
        }

        alloy::sleep_for(500ms);
    }
}
```

Compile this for a Nucleo-G071RB, a SAM E70, an RP2040 or an ESP32 DevKit — same bytes of
source, no edits. The ADC block simply vanishes on a board without that role.
