# The escape hatch

Adopting alloy is not all-or-nothing. A company arriving with 40 000 lines of
CubeMX-generated code, a register no HAL exposes, and an erratum workaround
copied out of a forum post needs to know three things before it starts:

1. can it call the vendor HAL it already has,
2. can it write a register directly, and
3. does including a vendor CMSIS header alongside alloy's generated ones break
   anything?

Everything below is answered by a build, not by a paragraph.
`examples/escape_hatch/` does all three in one image; every output quoted here
came from compiling and linking it.

---

## The short answers

| Question | Answer |
|---|---|
| Include a CMSIS-style device header next to alloy's? | **No conflict.** Even included *first*. Alloy's generated headers define zero preprocessor macros. |
| Call a vendor HAL C function? | **Yes**, unchanged, `extern "C"`, compiled by `alloy build` like any other file under `src/`. |
| Poke a register directly? | **Yes**, three ways, in decreasing order of how much you should like them. |
| Link a vendor `startup_*.s` / `system_*.c`? | **No** — and it fails loudly at link time, not subtly. |
| Define your own `<PERIPH>_IRQHandler`? | **Yes, and it silently wins.** This is the one hazard on the page. |

---

## 1. Headers: no collision, and why

Alloy's generated headers live in `namespace alloy::dev` / `alloy::ip::…`, in
lower case, and define **no preprocessor macros at all** — 0 `#define` lines
across all 2 577 generated `.hpp` files in this repo (the only generated file
with macros is `lwipopts.h`, which is lwIP's own C configuration and is not
part of the device API).

A vendor CMSIS device header is the opposite: uppercase object-like macros
(`GPIOA`, `RCC`), `<NAME>_TypeDef` structs at global scope, `__IO`, and
`<BLOCK>_<REG>_<FIELD>_Pos` constants. The two name spaces do not intersect,
so the include order does not matter. `examples/escape_hatch/src/main.cpp`
proves the harder direction:

```cpp title="illustrative: `vendor_device.h` is a CubeMX header this repository does not ship — bringing your own is the point of the page"
// Deliberately FIRST: if any vendor macro collided, this is what would break.
#include "vendor_device.h"
#include "vendor_hal.h"

#include <alloy/board.hpp>
#include <alloy/device.hpp>
```

It compiles with `-Wall -Wextra` and no warnings.

!!! note "About `vendor_device.h`"
    The example ships a header written in CMSIS's exact *shape* rather than
    ST's actual `stm32g0xx.h`, which is not redistributable here. The claim
    above is about those shapes — uppercase macros, `_TypeDef` structs, `__IO`,
    `_Pos`/`_Msk` constants, an `IRQn_Type` enum. If you have the real header,
    use it; nothing in the argument depends on the stand-in.

### A bonus you get for free

Both worlds carry the same constants, so make them agree at compile time:

```cpp title="illustrative: `GPIOA_BASE` comes from the vendor header above, not from alloy"
static_assert(GPIOA_BASE == alloy::dev::gpioa.base,
              "vendor header and alloy-devices disagree about GPIOA");
```

If your CMSIS header and the chip database ever disagreed about where a
peripheral lives, that line tells you at build time instead of on the bench.

## 2. Calling a vendor HAL

Vendor code stays vendor code: its own header, its own `.c` file, its own
idiom. `alloy build` compiles every `.c` and `.cpp` under the project's `src/`,
so nothing has to be ported.

```c
/* vendor_hal.c — plain C, knows nothing about alloy */
void VENDOR_GPIO_TogglePin(GPIO_TypeDef* port, uint32_t pin) {
    const uint32_t odr = port->ODR;
    port->BSRR = (odr & (1UL << pin)) ? (1UL << (pin + 16U)) : (1UL << pin);
}
```

The bridge from C++ is one cast, and the address comes from the database:

```cpp title="illustrative: `GPIO_TypeDef` and `VENDOR_GPIO_TogglePin` are the vendor HAL's, not alloy's"
auto* vendor_port = reinterpret_cast<GPIO_TypeDef*>(alloy::dev::gpioa.base);
VENDOR_GPIO_TogglePin(vendor_port, 5);
```

Configure the pin with alloy (`board::led`), poke it with the vendor function,
or the other way round. They are writing the same registers.

## 3. Writing a register directly — three routes

**Route A — through the generated description (recommended).** The address
comes from the chip database and the offsets are `static_assert`ed in the
generated header, so this survives a board change and a database update:

```cpp
using gpio = alloy::dev::gpioa_t::ip;                 // alloy::ip::st::gpio_v2
auto& regs = *reinterpret_cast<gpio::regs*>(alloy::dev::gpioa.base);
regs.BSRR = std::uint32_t{1} << 5;
```

**Route B — through the vendor's own types**, as in §2. Fine, and the
`static_assert` above keeps the two definitions of "where GPIOA is" honest.

**Route C — a raw literal address.** Legal, sometimes the only way to reach an
erratum workaround, and exactly what alloy's first contract guard forbids in
*framework* code:

```cpp
// STM32G071 RM0444 rev 5, §6.4.7: GPIOA_BSRR at 0x5000'0018.
auto* bsrr = reinterpret_cast<volatile std::uint32_t*>(0x50000018u);
*bsrr = std::uint32_t{1} << 5;
```

Nothing stops you. Keep them in one file, comment the datasheet, revision and
page, and know that the board portability of that line is now zero. If you find
yourself writing several, the better fix is usually to add the fact to
`alloy-devices` — then Route A works and every board gets it.

!!! warning "Route A is not always available, and Route C is the only one that always is"
    Route A goes through the generated description, so it reaches exactly what
    the chip database curates — and 28 of the STM32G0B1RE's 65 peripherals are
    uncurated, which means `alloy::dev::tim1_t` **does not exist**, not that it
    exists and is empty. Curation has
    [four separate gates](../reference/peripheral-surface.md#question-0-what-does-the-database-already-know)
    — the peripheral, the register, the field, and the field's *encoding* — and
    a block can pass one and fail the next (the G0's ADC was curated with a map
    that had no `TR1`; FDCAN's `RXGFC.ANFS` was a curated field whose values
    were not curated, so the only way to write it was the bare integer `2`).

    So "you can always drop to registers" is true of Route C and of Route C
    only. That is a real door, and it is the one with no gate address, no IRQ
    number and no route check behind it. Reaching for it because a *peripheral*
    is uncurated is usually the wrong trade against spending an afternoon in
    `alloy-devices`; reaching for it because one *erratum* register is
    undocumented is what it is for.

## 4. What DOES collide: the link, not the include

This is the part that costs a day if nobody writes it down.

Alloy generates its own vector table and links with `-nostartfiles` and its own
linker script. A vendor `startup_stm32g071xx.s` (or the `system_stm32g0xx.c`
that comes with it) defines the same symbols. The link fails, loudly:

```
ld: .alloy/generated/nucleo_g071rb/vector_table.c.obj: in function `Default_Handler':
    multiple definition of `Default_Handler'; src/startup_vendor.c.obj: first defined here
ld: src/alloy/arch/cortex_m/startup.cpp.obj: in function `Reset_Handler':
    multiple definition of `Reset_Handler'; src/startup_vendor.c.obj: first defined here
collect2: error: ld returned 1 exit status
```

The same happens for `SysTick_Handler`, which alloy defines **strongly** in
`src/alloy/arch/cortex_m/systick.cpp` because it drives `uptime_ms()`:

```
ld: src/alloy/arch/cortex_m/systick.cpp:45: multiple definition of `SysTick_Handler'
```

That is good news: alloy's timebase cannot be silently replaced.

**Do not add a vendor startup file, a vendor linker script, or a vendor
`SystemInit`.** Alloy already does reset, `.data` copy, `.bss` zero,
`__init_array` and the vector table. Take the peripheral drivers from your
legacy tree, not the startup.

### The one silent case

Peripheral interrupt handlers in the generated vector table are **weak** thunks
into `alloy_irq_dispatch()`:

```c
__attribute__((weak)) void TIM2_IRQHandler(void) { alloy_irq_dispatch(15u); }
```

A strong definition anywhere in your project wins, with no warning:

```cpp
extern "C" void TIM2_IRQHandler(void) { /* your CubeMX handler */ }
```

```console
$ arm-none-eabi-nm escape_hatch.elf | grep IRQHandler
080000e0 T TIM2_IRQHandler        <- yours, strong
080003e0 W TIM3_IRQHandler        <- alloy's weak thunk
```

That is a **feature** — it is how you keep a hand-tuned ISR — but it means
`alloy::irq`'s registration for that line silently stops being called. If an
interrupt-driven alloy peripheral (I²C, SPI, DMA, UART, pin interrupts) shares
that line, it stops working with no diagnostic. Check for it the same way:
`nm <elf> | grep IRQHandler` and look for a `T` where you expected a `W`.

## 5. Migrating gradually

A workable order, in decreasing risk:

1. **Start from the board, not from the app.** `alloy new --chip <part>` gives
   a project whose `boards/<id>/board.json` you edit to match your schematic.
   Nothing of your application moves yet.
2. **Bring the legacy drivers over as C files** under `src/`, minus the startup
   and minus the linker script. They compile unchanged.
3. **Replace one peripheral at a time** with the alloy role — the LED, then the
   debug UART, then the buses. Each replacement deletes vendor code rather than
   wrapping it.
4. **Keep the `static_assert`s** that pin vendor addresses to
   `alloy::dev::` while both exist. They are free and they catch the one class
   of mistake this migration produces.
5. **Pin the chip database** before you ship — see
   [API stability](../reference/stability.md#pinning-the-chip-database).

## Building the example

```console
$ cd examples/escape_hatch && alloy build
[10/10] Linking CXX executable escape_hatch.elf
   text    data     bss     dec     hex  filename
   2912       0    2224    5136    1410  escape_hatch.elf
```

It is a `nucleo_g071rb` example on purpose: an escape hatch is chip-specific by
nature, and pretending otherwise would be the wrong lesson. Its static
properties are checked with everything else —
`scripts/check_static_limits.sh escape_hatch nucleo_g071rb` reports no heap, no
exceptions, no recursion and a 188-byte peak stack.
