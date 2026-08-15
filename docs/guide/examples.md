# Examples

Every example is a single portable `main.cpp` with **zero preprocessor conditionals** — CI
compiles each one for every board. Copy any of them into a project's `src/`.

| Example | Shows |
| --- | --- |
| `blink` | the minimal portable app — `board::led.toggle()` |
| `uart_echo` | UART TX/RX, the debug console |
| `uart_frame` | the three layers of the [peripheral surface](../reference/peripheral-surface.md) in one portable file |
| `hello` | blink + echo together (the `alloy new` scaffold) |
| `pwm_fade` | PWM duty ramps, an LED breathing |
| `analog_probe` | ADC sampling, printing raw counts |
| `i2c_scan` | probing an I²C bus for devices |
| `spi_loopback` | SPI transfers (MOSI→MISO looped) |
| `irq_echo` | interrupt-driven UART RX |
| `pin_irq` | a button that reports its own edges — [pin interrupts](peripherals.md), never polled |
| `dma_probe` | ADC burst + PWM waveform streamed by DMA, zero CPU |
| `encoder` | the same timer as a *position counter* — a [personality](../reference/peripheral-surface.md#personalities-a-block-runs-in-one-mode-at-a-time), not a second knob on `pwm_fade` |
| `adc_watchdog` | an analog window comparator armed *after* `open()` — a [sub-resource](../reference/peripheral-surface.md#sub-resources-a-thing-inside-the-peripheral-with-its-own-lifetime) with its own handle |
| `pwm_fade`, `services` | the cooperative scheduler + control loop |
| `async_blink`, `async_heartbeat` | two concurrent coroutine tasks, no heap, no RTOS |
| `async_sensor` | a [vendored driver](libraries.md) read inside an async task |
| `async_io` | `co_await` over SPI, I²C and DMA interrupts — the [driver awaitables](async.md) |
| `net_probe`, `net_echo` | Ethernet (SAM E70 GMAC + PHY) |
| `tcp_echo`, `udp_echo`, `dhcp_echo`, `http_server` | the TCP/IP stack |
| `fs` | a filesystem on on-chip or external storage |
| `watchdog` | the independent watchdog |
| `window_watchdog` | the *other* watchdog — it also resets on a feed that arrives too **early**, so it catches a loop that has lost its timebase. A second [role](peripherals.md), not a mode of `watchdog`; the two run together |
| `nvm`, `rtc`, `dac`, `can` | on-chip flash key/value, RTC, DAC, CAN |
| `crash_report` | a device that crashes, reboots, and explains itself — the [crash report](crash-reports.md) loop |
| `bootloader_uart` + `ota_app` | the [field-update](firmware-update.md) pair: update over UART, trial boot, confirm |

The examples ending in `_read` (`i2c_read`, `spi_read`, `adc_read`, `dma_uart`) are the ones CI
asserts on under [emulation](emulation.md) — they print a value that only a correct driver
sequence can produce. `async_io` is asserted the same way, on a number only a coroutine that
actually suspended and was woken by an interrupt can print. So is `adc_watchdog`, on four
lines that contradict each other unless the window comparator is real.

`encoder` and `can` have **no** emulation leg and cannot have one: Renode's `Timers.STM32_Timer`
refuses both register writes the encoder personality is made of, and it maps the G0's `fdcan1`
to a bxCAN model that does not answer M_CAN's registers at all. Both build for every board;
neither is executed anywhere but hardware, which has not run them.

`window_watchdog` has none either, for a blunter reason: Renode 1.16.1 has **no WWDG model at
all**. `platforms/cpus/stm32g0.repl` covers the block with `Tag <0x40002C00, 0x40002FFF>
"WWDG"` — an address range with no device behind it — and asking the monitor for the class
directly answers `Error E04: Could not resolve type: 'Timers.STM32_WindowWatchdog'`. Under
emulation the peripheral would swallow every write and never bite, so a green leg would prove
the opposite of what it claimed to. The arithmetic and the register sequence are checked on the
host instead (`tests/test_wwdt.cpp`); that the silicon resets is unwitnessed.

## Running an example

The examples live in the framework tree under `examples/`. Build one for any board:

```console
$ cd examples/dma_probe
$ alloy build --board nucleo_f722ze
$ alloy run --board nucleo_f722ze     # flash + monitor
```

## A guided read: `dma_probe`

`dma_probe` is a good tour of the design — it uses **capability checks inside generic lambdas**
so the same file adapts to whatever the board offers:

```cpp
[&uart]<class Dma, class Adc = board::adc>(Dma*) {
    if constexpr (HasDma<Dma> && board::caps::adc) {
        auto adc  = Adc::open();
        auto chan = alloy::dma::channel<Dma, 1>::claim();
        std::uint16_t samples[32] = {};
        adc.read_burst(chan, board::adc_vref_channel,
                       std::span<std::uint16_t>{samples});   // 32 samples, no CPU
        // ...
    } else {
        uart.write("adc burst DMA: not on this board\r\n");
    }
}(static_cast<board::dma_t*>(nullptr));
```

On a board with a DMA controller and an ADC, this streams 32 conversions into RAM while the CPU
does nothing else; on a board without them, the whole block compiles away and prints the honest
fallback. No `#ifdef` in sight.

### What folds away on the RP2040, and why

The folding is the design, so it is worth reading one board's answers in full rather than
assuming a controller means every DMA example lights up. On `raspberry_pi_pico` and
`rp2040_zero`, with the RP2040 DMA driver present:

| example | on an RP2040 | why |
|---|---|---|
| `dma_uart` | **DMA path** — prints `dma via DMA` | `write_dma()` needs the driver's TX-DMA hooks plus a TX request id; the PL011 has both |
| `dma_probe` | **DMA path for the UART branch**; ADC-burst and PWM branches print their fallback | neither the RP2040 ADC driver nor its PWM driver has DMA hooks |
| `adc_stream` | fallback: `adc ring: not available on this board` | `adc::stream` is built entirely from `take()`/`missed()`/`pending()` — half-buffer events — and this DMA controller has **no half-transfer event at all** |
| `modbus_rtu_server` | fallback: the polled server, no ring | `rx_ring()` needs a ring *and* a frame-gap (IDLE) event; the PL011's `RTIM` analogue cannot be sourced as firing while DMA drains the FIFO |

None of those fallbacks is a workaround or a special case in the example — each one is the same
`if constexpr (requires { … })` probe returning `false`, and the compile error you would get by
calling the method anyway names the missing capability
(`nested requirement 'ring_capable<…>' is not satisfied`). The full reasoning, and what would
have to become true to change any row, is in `docs/design/dma-streams.md` §3.4; the behavioural
evidence that is still owed for this family is in
[the RP2040 DMA hardware checklist](rp2040-dma-hardware-checklist.md).
