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
    bool down = board::button.pressed();
}
```

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
uart.on_rx([](void*, std::uint8_t b) { /* runs in ISR context */ }, nullptr);
```

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

```cpp
auto pwm = board::led_pwm::open({.freq_hz = 1'000});
pwm.set_duty(50);               // 50% — LED at half brightness
```

## DMA

DMA channels are claimed once, then drive a peripheral operation with zero CPU. Capability is
checked at compile time — a controller without circular mode simply doesn't offer the circular
call:

```cpp
auto chan = alloy::dma::channel<board::dma_t, 1>::claim();

// One-shot ADC burst into RAM:
adc.read_burst(chan, board::adc_vref_channel, std::span<std::uint16_t>{samples});

// Circular PWM waveform (only on controllers that support it):
pwm.stream_duty(chan, std::span<const std::uint16_t>{wave});
```

## Interrupts

Attach a handler to an IRQ line, give it a priority, and dispatch runs it. Several peripherals that
share one vector each attach their own handler and all fire — the portable code never touches an
`#ifdef` or a vector table:

```cpp
alloy::irq::attach(board::uart2_irq, &on_rx, &ctx);
alloy::irq::enable(board::uart2_irq);
alloy::irq::set_priority(board::uart2_irq, 1);   // 0 = most urgent; higher = less
```

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

---

Need something the HAL doesn't expose yet? The generated register overlays are typed and
available, so you can drop to registers for one peripheral without leaving the framework — see
[Architecture](../reference/architecture.md).
