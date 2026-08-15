# Peripherals

Every peripheral follows the same shape: a typed **`bind`** that checks pin routes at compile
time, and an **`open()`** that returns a small, move-only handle. Drivers are selected by the
chip's IP version — no vtables, no heap.

Most apps reach peripherals through [board roles](portable-code.md) (`board::debug_uart`,
`board::i2c`, …). The snippets below use those roles.

## GPIO

```cpp
board::led.on();
board::led.off();
board::led.toggle();

if constexpr (board::caps::button) {
    bool down = board::button.is_active();     // "pressed", in the board's polarity
}
```

`is_active()` answers in the **board's** terms: the board file says the button is active-low, so
a grounded pin reads `true` and the app never learns which way round the hardware is.
`is_high()` is the raw-level twin, for a signal whose polarity belongs to the part rather than
to the board.

### Pin interrupts

An input can report its own edges instead of being polled — what a sensor's DRDY/INT line, a
button that must not be sampled in a loop, and wake-on-pin all need:

```cpp
board::user_button().on_active(+[](void*) { g_pressed = true; });   // ISR context
```

`on_active()` is the portable form: the edge comes from the **board's** declared polarity, so an
active-low button arms the falling edge and an active-high one the rising edge without the app
knowing which. `on_edge(alloy::gpio::edge::rising, fn, ctx)` is the explicit form for a signal
whose polarity belongs to the part, not the board. `clear_on_edge()` stops reporting.

Between edges, the handler keeps a **software** count — never the hardware pending bit, so
reading it can never eat an interrupt:

```cpp
while (btn.take_edge()) { /* once per edge that actually happened */ }
std::uint32_t total = btn.edges();   // monotonic: you can see that you missed some
```

Like every other interrupt hook in alloy, these methods are `requires`-gated on the pin driver
actually having them. On a board with no button, or a chip whose interrupt controller isn't
curated yet, the methods **do not exist** and portable code detects that directly:

```cpp
if constexpr (requires { btn.on_active(nullptr, nullptr); }) { /* ... */ }
```

Two costs worth knowing before you design around them. Each armed pin takes one slot from the
firmware-wide `alloy::irq::kMaxHandlers` pool (16, shared with UART/SPI/I²C/DMA — attaching past
it traps). And on STM32 the interrupt line number **is** the pin number, shared by every port:
PA13 and PC13 cannot both have a pin interrupt at once. That is silicon, not a framework limit.

## UART

```cpp
auto uart = board::debug_uart::open({.baud = 115200});
uart.write("hello\r\n");

std::uint8_t byte{};
if (uart.read(byte)) {          // non-blocking; false when no byte is ready
    uart.write(byte);
}
```

Interrupt-driven RX (where the driver supports it) attaches a callback:

```cpp
uart.on_receive(+[](void* ctx, std::uint8_t b) { /* runs in ISR context */ }, nullptr);
```

Like the pin-interrupt hooks, `on_receive` is `requires`-gated on the driver having an interrupt
path, so portable code probes for it rather than assuming — see `examples/irq_echo/src/main.cpp`
for the `if constexpr (requires { u.on_receive(nullptr, nullptr); })` wrapper. `detach_receive()`
stops reporting.

Taking the CPU out of the byte loop entirely — a DMA ring behind the UART, with no per-byte
interrupt at all — is **[Streaming with DMA](dma.md)**.

## I²C

```cpp
auto i2c = board::i2c::open({.speed_hz = 400'000});

std::uint8_t reg[1] = {0x0F};
std::uint8_t id[1]  = {};
if (i2c.write_read(0x68, reg, id)) {   // returns false on NACK or bus error
    // id[0] now holds the device's WHO_AM_I
}
```

!!! warning "Bounds"
    A missing pull-up or a stuck bus makes I²C return `false` (bounded), not hang. Transfers are
    limited to 255 bytes on the STM32 `i2c_v2` (NBYTES is 8-bit).

`write`/`read`/`write_read` spin once per byte — about 90 µs a byte at 100 kHz, with nothing else
running. `write_async`/`read_async` program the transfer and return; the driver's ISR moves every
byte and calls your function when the STOP lands.

```cpp
volatile bool done = false;
const std::uint8_t cmd[3] = {0x01, 0x02, 0x03};

i2c.write_async(0x68, cmd, +[](void* f) { *static_cast<volatile bool*>(f) = true; },
                const_cast<bool*>(&done));
// ... run the control loop here ...
if (!i2c.wait_transfer()) {     // BOUNDED: false means the interrupt never came
    i2c.detach_transfer();      // disarm, so the bus is not left busy() forever
}
if (!i2c.transfer_ok()) { /* NACK or bus error — the async form of `false` */ }
```

The callback runs in interrupt context — set a flag, wake a task, or start the next transfer,
nothing more. `busy()` reports whether a transfer is still in flight, and `transfer_ok()` carries
the result the blocking calls return directly.

!!! warning "Do not mix the two APIs on one bus"
    The ISR consumes the TXIS/RXNE/STOP flags the blocking calls wait for. Ask `busy()` first.

!!! note "Capability-gated"
    `write_async`/`read_async` exist only where the backing driver implements them — the ST
    `i2c_v2` driver today. Elsewhere the methods are not declared at all, so calling one is a
    compile error rather than a silent fall back to blocking. The ST `i2c_v1` (F1/F4), Microchip
    TWIHS and Espressif drivers have no interrupt path yet; `i2c_v1` additionally has no poll
    budget at all, so a wedged bus hangs it rather than returning `false`.

## SPI

```cpp
auto spi = board::spi::open({.clock_hz = 1'000'000, .mode = 0});
std::uint8_t rx = spi.xfer(0x9F);

std::uint8_t buf[4] = {1, 2, 3, 4};
spi.transfer(buf);              // in-place full-duplex
```

`xfer`/`write`/`transfer` are byte-at-a-time spins: an N-byte exchange burns the CPU for all of
it. `transfer_async` starts the exchange and returns — the driver's ISR clocks every remaining
byte and calls your function when the last one lands.

```cpp
volatile bool done = false;
const std::uint8_t out[3] = {0x11, 0x22, 0x33};
std::uint8_t in[3] = {};

spi.transfer_async(out, in, +[](void* f) { *static_cast<volatile bool*>(f) = true; },
                   const_cast<bool*>(&done));
// ... run the control loop here ...
if (!spi.wait_transfer()) {     // BOUNDED: false means the interrupt never came
    spi.detach_transfer();      // disarm, so the bus is not left busy() forever
}
```

The callback runs in interrupt context — set a flag, wake a task, or start the next transfer,
nothing more. `busy()` reports whether a transfer is still in flight.

!!! warning "Do not mix the two APIs on one bus"
    The ISR consumes the RX-ready flag the blocking `xfer()` waits for. Ask `busy()` first.

!!! note "Capability-gated"
    `transfer_async` exists only where the backing driver implements it — the ST `spi_v1`/`spi_v2`
    drivers today. On the SAM E70 the method is not declared at all (the device database carries
    no field definitions for that SPI's IER/IDR/IMR, so there is no interrupt-enable accessor to
    generate), so calling it is a compile error rather than a silent fall back to blocking.

## ADC

```cpp
auto adc = board::adc::open();
std::uint16_t counts = adc.read(board::adc_vref_channel);
```

## PWM

Duty is **normalized**, not a percentage: `0` is off and `0xFFFF` is always on,
whatever the timer's real resolution turns out to be.

```cpp
auto pwm = board::led_pwm::open({.freq_hz = 1'000});
pwm.set_duty(0x8000);           // half — LED at half brightness
```

Centre-aligned counting, a trigger output for the ADC, and the three-phase
complementary bridge are all in **[PWM and motor control](pwm.md)**.

## DMA

DMA takes the CPU out of the data path entirely. It has **its own page** —
**[Streaming data without the CPU](dma.md)** — because the answer differs on every board and
every engine, and because how far each path has been proven differs too.

The short version: your board file hands out the channels, the generator turns each assignment
into a route constant, and the facade methods that need one simply **exist** on boards that
declared it. You never type a channel number:

```cpp
alloy::dma::ring_storage<std::uint16_t, 256> storage;
auto stream = adc.ring(storage);          // uses the board's `adc.conv` route
auto half   = stream.take();              // one hardware-stable half at a time
```

The same shape gives you `uart.rx_ring()` for a byte stream with no per-byte ISR,
`uart.write_dma()` for a one-shot transmit, and `spi.transfer_dma(tx, rx)` for full duplex
through a claimed channel pair.

You can still claim a channel by index — that is the escape hatch, for a route the board did not
declare or a controller signal the facade does not model:

```cpp
auto chan = alloy::dma::channel<board::dma_t, 1>::claim();
adc.read_burst(chan, board::adc_vref_channel, std::span<std::uint16_t>{samples});
pwm.stream_duty(chan, std::span<const std::uint16_t>{wave});
```

Both forms are capability-checked at compile time: ask for something the board or the silicon
cannot do and the method is **not declared**, so you get an error naming what is missing rather
than a link error or a runtime surprise.

## Interrupts

Attach a handler to an IRQ line, give it a priority, and dispatch runs it. Several peripherals that
share one vector each attach their own handler and all fire — the portable code never touches an
`#ifdef` or a vector table:

```cpp
// The line number is a generated fact: device.hpp emits one per instance.
constexpr auto line = alloy::dev::usart2_t::irq;   // alloy::irq_line{28} on an STM32G0

alloy::irq::attach(line, &on_rx, &ctx);
alloy::irq::enable(line);
alloy::irq::set_priority(line, 1);   // 0 = most urgent; higher = less
```

There is no `board::…_irq` name. Lines come from the **device** descriptor
(`alloy::dev::<instance>_t::irq`), or as a literal `alloy::irq_line{N}` when you are reaching for
a vector no instance owns — `examples/concurrency_probe/src/main.cpp` uses both spellings.

For data shared between an ISR and the main loop, `critical_section` masks only interrupts at a
given level **or less urgent** (level 0 is the most urgent) — a high-priority control loop keeps
running. On Cortex-M3/M4/M7 this uses `BASEPRI`; on Cortex-M0/M0+ and Xtensa (no priority mask) it
safely degrades to a full mask:

```cpp
{
    alloy::irq::critical_section cs{4};   // mask level 4 and any less-urgent line
    shared = update();                    // a level-0 control ISR still fires here
}                                         // mask lifted on scope exit (RAII)
```

## Watchdog, RTC, NVM

```cpp
board::watchdog.feed();                         // no-op stub where absent

if constexpr (board::caps::rtc)  { /* board::rtc  */ }
if constexpr (board::caps::nvm)  { /* board::nvm — key/value over a flash page */ }
```

### Two watchdogs, and they are not interchangeable

`board::watchdog` (an IWDG) asks *is anybody still running?* A loop that has lost its timebase
and spins ten times too fast answers yes, and is never reset. `board::window_watchdog` (a WWDG)
asks the harder question — *is anybody running at the right rate?* — by resetting the part on a
feed that arrives **before** the window opens as readily as on one that never arrives.

```cpp
const auto w = board::window_watchdog.start();  // the window the board file declares
// w.earliest .. w.deadline — what the counter could actually land on
for (;;) { control_step(); board::window_watchdog.feed(); }
```

They are **two roles**, not two modes of one, and a board may declare both — that is the normal
safety shape, an IWDG as the long backstop and a WWDG as the loop-rate monitor. The chip
database records the split at its own gate — `st/wwdg_v2`'s IP class is `window_watchdog`, not
`watchdog` — so getting the two the wrong way round fails rather than quietly changing what
`feed()` means. It fails in two different places, and only one of them is a validator error:
naming a `wwdg` for the `watchdog` role while `window_watchdog` also claims it is rejected by
the personality guard (*"a block runs in one personality at a time"*); naming an `iwdg` for
`window_watchdog` only *warns* at validation — an `ip_class` mismatch is a warning for every
role — and is stopped by a `static_assert` in `alloy/wwdt.hpp` that names the confusion.
Three more consequences worth knowing before you reach for it:

- **it is short.** A WWDG counts its own bus clock through a fixed /4096 and a seven-bit
  counter, so on a 64 MHz APB it tops out near half a *second* — three orders of magnitude
  under the IWDG's ~32 s. Ask for more and you get a compile error that names
  `board::window_watchdog.longest`, not a silent clamp.
- **there is no `stop()`.** The activation bit is cleared only by a reset, so the facade offers
  no method that would have to lie about switching it off.
- `on_early_wakeup(fn)` installs the last-gasp hook — an interrupt one counter tick *before* the
  reset, on a machine that is still running. It pairs with the
  [crash report](crash-reports.md): same rule for the handler (touch nothing that could itself
  fail), same reader (the next boot). Install it before `start()`.

---

Need something the HAL doesn't expose yet? The generated register overlays are typed and
available, so you can drop to registers for one peripheral without leaving the framework — see
[Architecture](../reference/architecture.md).
