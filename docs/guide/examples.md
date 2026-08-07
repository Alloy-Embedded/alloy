# Examples

Every example is a single portable `main.cpp` with **zero preprocessor conditionals** — CI
compiles each one for every board. Copy any of them into a project's `src/`.

| Example | Shows |
| --- | --- |
| `blink` | the minimal portable app — `board::led.toggle()` |
| `uart_echo` | UART TX/RX, the debug console |
| `hello` | blink + echo together (the `alloy new` scaffold) |
| `pwm_fade` | PWM duty ramps, an LED breathing |
| `analog_probe` | ADC sampling, printing raw counts |
| `i2c_scan` | probing an I²C bus for devices |
| `spi_loopback` | SPI transfers (MOSI→MISO looped) |
| `irq_echo` | interrupt-driven UART RX |
| `dma_probe` | ADC burst + PWM waveform streamed by DMA, zero CPU |
| `pwm_fade`, `services` | the cooperative scheduler + control loop |
| `async_blink`, `async_heartbeat` | two concurrent coroutine tasks, no heap, no RTOS |
| `async_sensor` | a [vendored driver](libraries.md) read inside an async task |
| `net_probe`, `net_echo` | Ethernet (SAM E70 GMAC + PHY) |
| `tcp_echo`, `udp_echo`, `dhcp_echo`, `http_server` | the TCP/IP stack |
| `fs` | a filesystem on on-chip or external storage |
| `watchdog` | the independent watchdog |
| `nvm`, `rtc`, `dac`, `can` | on-chip flash key/value, RTC, DAC, CAN |
| `bootloader_uart` + `ota_app` | the [field-update](firmware-update.md) pair: update over UART, trial boot, confirm |

The examples ending in `_read` (`i2c_read`, `spi_read`, `adc_read`, `dma_uart`) are the ones CI
asserts on under [emulation](emulation.md) — they print a value that only a correct driver
sequence can produce.

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
