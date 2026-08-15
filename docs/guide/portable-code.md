# Portable code

alloy's headline promise is that **one `main.cpp` recompiles for any board, with zero
preprocessor conditionals**. This page explains the three ideas that make that true.

## Board roles

A board provides a fixed set of **roles** — named objects your app talks to instead of raw
peripherals:

| Role | What it is |
| --- | --- |
| `board::led`, `board::button` | the status LED and the user button |
| `board::gpio_bus` | a named group of plain pins, read and written as one word |
| `board::debug_uart` | the console UART |
| `board::low_power_uart` | an LPUART that can receive with the main clock stopped |
| `board::i2c`, `board::spi` | the wired buses |
| `board::can` | a CAN / CAN-FD controller |
| `board::ethernet` | a MAC, for the [network stack](../reference/architecture.md) |
| `board::adc`, `board::dac` | analog in and out |
| `board::led_pwm` | one PWM channel — the "make an LED breathe" role |
| `board::bridge` | a three-phase inverter bridge with dead time ([PWM](pwm.md)) |
| `board::encoder` | a quadrature encoder input |
| `board::tick` | a spare timer you drive yourself |
| `board::rtc` | a calendar that survives reset |
| `board::watchdog`, `board::window_watchdog` | the independent and windowed watchdogs |
| `board::nvm`, `board::fs` | on-chip flash as a key/value store, or as a filesystem |
| `board::eeprom` | an external EEPROM sitting on one of the buses |

That is the whole catalogue — 21 roles, defined once in `tools/alloy/alloy_cli/roles.py` and
consumed by the board validator, the code generator and `alloy board-info` alike, so the three
cannot disagree about what a board needs to declare.

Because every app names roles, not chip-specific peripherals, the source never mentions a pin,
an address, or a vendor. The board data binds each role to the right pins with **compile-time
route checking**.

```cpp
board::init();
board::led.toggle();
auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
uart.write("up\r\n");
```

## Where the roles come from

Roles are not magic and they are not hand-written per board. They come out of one JSON file:

```json title="boards/nucleo_g071rb/board.json (excerpt)"
"roles": {
  "led":        { "pin": "pa5", "active": "high", "label": "LD4 (green, D13)" },
  "button":     { "pin": "pc13", "active": "low", "label": "B1 USER" },
  "debug_uart": { "peripheral": "usart2", "tx": "pa2", "rx": "pa3", "baud": 115200 }
}
```

At build time alloy reads that alongside the chip's data — pin routes, clock tree, register
map — and generates `board.hpp` for you. `"active": "low"` on the button is why
`board::button.is_active()` returns `true` when it is *pressed*: the polarity is a fact about
the PCB, so your application never carries it. `"tx": "pa2"` is checked against the chip's route
table, so a typo is a compile error and not a dead board.

This is the whole reason the source is portable. **Every difference between two
microcontrollers lives in data, not in your `src/`** — which is why changing one line of
`alloy.toml` retargets the program. See [Boards](boards.md) for the full file and
[Adding a board](adding-a-board.md) for writing your own.

## Capabilities and `if constexpr`

Not every board has every role. Instead of `#ifdef`, alloy exposes compile-time booleans under
`board::caps::*` and you branch with `if constexpr`:

```cpp
if constexpr (board::caps::adc) {          // false on every RP2040 and ESP32 board
    auto adc = board::adc::open();
    if (adc.read(3) > 2048u) {
        board::led.on();
    }
}
```

The discarded branch is **not emitted**, so code guarded this way costs nothing on boards that
lack the role — no flash, no RAM, no runtime check. `board::caps::*` is generated for every role
whether the board declares it or not, so the name always exists and the answer is always a
`constexpr bool`. `alloy board-info <id>` prints the list for any board.

!!! info "Roles that are absent still have a name"
    A role a board does not declare is generated as a **no-op stub with every entry point the
    real one has**: `board::rtc` becomes `alloy::rtc::null_rtc`, `board::adc::open()` returns a
    handle whose `read()` answers `0`, `board::watchdog.feed()` does nothing. That is what lets a
    guarded call sit in your source on a board that cannot perform it. What you *cannot* do is
    bind a role to hardware that cannot carry it — that is a `static_assert` at compile time,
    never a surprise at runtime.

### The LED and the button are the exception — use the accessors

Every other role has a stub. The two **pin** roles do not: on a board that declares no button,
`board::button` is not declared at all, and neither `if constexpr (board::caps::button)` nor a
template wrapper saves you, because `board::button` is a non-dependent name and the compiler
looks it up either way. Measured on `rp2040_zero`, which has an LED and no button:

```
error: 'button' is not a member of 'board'; did you mean 'board::caps::button'?
```

The portable spelling is the pair of accessor functions the generator emits for exactly this:

```cpp title="Compiles on all nine shipped boards"
#include <alloy/board.hpp>

int main() {
    board::init();
    auto button = board::user_button();   // the real input, or a null stub
    auto led = board::status_led();
    for (;;) {
        if (button.is_active()) { led.on(); } else { led.off(); }
    }
}
```

`board::user_button()` returns the board's real `gpio::input` where one exists and an
`alloy::gpio::null_input` — whose `is_active()` answers `false` — where it does not.
`board::status_led()` is its `null_output` twin. Five of the nine shipped boards declare a
button; all nine build that program.

!!! note "`is_active()` vs `is_high()`"
    `is_active()` knows the board's `"active": "low"` fact and returns `true` when the button is
    *pressed*, whichever way the PCB wired it. `is_high()` is the raw pin level, for when you
    want the electrical truth and not the logical one.

### The one sharp edge: `if constexpr` still type-checks the branch it drops

This trips everyone once, so it is worth 30 seconds now. `if constexpr` in a **non-template**
function — and `main()` is a non-template function — discards the untaken branch from the
generated code but *still parses and name-looks-up* everything in it. The stubs above exist
mostly to absorb that. When you call something the stub does not have, you get a compile error
on a board the code was never going to run that branch on:

```
src/main.cpp:8:27: error: 'struct board::adc::null_handle' has no member named 'ring'
```

That is `alloy build --board rp2040_zero` on a program whose ADC branch is already guarded by
`if constexpr (board::caps::adc)`. The guard is correct and the error is still real.

The fix is to make the branch **dependent**, by putting it in a template and asking for the
member rather than for the role:

```cpp title="Portable across boards that stream and boards that poll"
#include <alloy/board.hpp>
#include <alloy/dma.hpp>

namespace {
using storage_t = alloy::dma::ring_storage<std::uint16_t, 64>;

// A TEMPLATE, so the branch you did not take is never instantiated.
template <class Uart, class Adc>
void sample(const Uart& uart, Adc& adc) {
    if constexpr (requires(storage_t& s) { adc.ring(s); }) {
        static storage_t storage{};
        auto stream = adc.ring(storage);
        while (!stream.pending()) {}
        uart.write(stream.take().empty() ? "empty\r\n" : "streamed\r\n");
    } else {
        uart.write(adc.read(3) != 0u ? "polled\r\n" : "polled zero\r\n");
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    if constexpr (board::caps::adc) {
        auto adc = board::adc::open();
        sample(uart, adc);
    }
    for (;;) {}
}
```

That builds for all nine shipped boards. Two `if constexpr`s, doing different jobs: the outer
one asks *does this board have the role*, the inner one asks *can this board's engine do it*.
The second question is the interesting one — DMA-fed sampling exists on the STM32G0 boards and
not on the RP2040, and `requires` is how you find out without naming a chip.

Use `board::caps::*` for "does this board have the role". Use
`if constexpr (requires { … })` inside a template for "can this handle do the thing". The
in-repo examples under `examples/` use exactly this pair.

## Errors you can read

Portability is only useful if mistakes are caught early. Bind a UART to a pin that has no route
to it and the compiler stops you, **naming the pin**:

```
alloy/src/alloy/uart.hpp: In instantiation of 'struct alloy::uart::bind<alloy::dev::usart2_t,
  alloy::uart::tx<alloy::dev::pa5_t>, alloy::uart::rx<alloy::dev::pa3_t>, board::clock_profile>':
src/main.cpp:7:32:   required from here
alloy/src/alloy/uart.hpp:502:27: error: static assertion failed: TX pin has no route to this
  UART on the selected chip (check the chip's route table in alloy-devices)
```

That is the verbatim output of `alloy build --board nucleo_g071rb` on a project that binds
USART2's TX to PA5, which the STM32G071 has no route for. This "wrong route → readable compile
error" is a CI-enforced acceptance test, not a nice-to-have — `scripts/check_compile_errors.py`
fails the build if the diagnostic stops naming the pin. See
[Architecture](../reference/architecture.md).

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
            const std::uint16_t raw = adc.read(3);
            uart.write(raw != 0u ? "sample\r\n" : "sample zero\r\n");
        }

        alloy::sleep_for(500ms);
    }
}
```

Compile this for a Nucleo-G071RB, a SAM E70, an RP2040 or an ESP32 DevKit — same bytes of
source, no edits. The ADC block simply vanishes on a board without that role.

!!! warning "Open peripherals once, not once per loop"
    The snippet above opens the ADC inside the loop to keep the `if constexpr` short. Real code
    should open it *before* the loop —
    [the getting-started walkthrough](../getting-started.md#make-it-do-something) shows the shape
    that does that without duplicating the loop body, and measures what the lazy version costs.
    [ADC](adc.md) has the streaming version, which takes the CPU out of the sampling path
    entirely.
