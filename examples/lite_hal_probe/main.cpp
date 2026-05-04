/// @file examples/lite_hal_probe/main.cpp
/// Integration smoke-test for the alloy.device.v2.1 lite HAL stack.
///
/// Exercises every lite driver in sequence on a Nucleo-G071RB board:
///   GPIO  — PA5 (LED) as output; blink to confirm clock is running
///   UART  — USART1 on PA9/PA10 (AF1) at 115200; print probe log
///   SPI   — SPI1 on PB3/4/5 (AF0) mode 0, div/8; configure + status
///   I2C   — I2C1 on PB6/7 (AF6); configure + bus scan (0x00–0x7F)
///   Timer — TIM3 as time-base; PSC/ARR for 1 ms tick; delay_ticks()
///   ADC   — ADC1 on PA0 (IN0); calibrate + single conversion; print raw
///   DAC   — DAC1 on PA4 (OUT1); write 2048 (~Vdda/2); read DOR1
///
/// Requires:
///   ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE   = 1  (peripheral_traits.h present)
///   ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE = 1  (rcc_enable.hpp present)
///
/// Run alloy-codegen to populate alloy/device/st/stm32g0/stm32g071rb/ first:
///   cd alloy-codegen
///   python3 -m ... st/stm32g0/stm32g071rb \
///       --emit peripheral_traits --emit peripheral_id --emit rcc_enable \
///       --out ../alloy/device
///
/// Clock assumption: HSI16 (16 MHz) — the G071RB reset default, used for
/// BRR/ARR calculations.  If a PLL is active, pass the correct clock_hz.

#include "device/runtime.hpp"

#if ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE && ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE

#include "hal/adc.hpp"
#include "hal/dac.hpp"
#include "hal/gpio.hpp"
#include "hal/i2c.hpp"
#include "hal/rcc.hpp"
#include "hal/spi.hpp"
#include "hal/timer.hpp"
#include "hal/uart.hpp"

#include <array>
#include <cstdint>

// ---------------------------------------------------------------------------
// Namespace aliases
// ---------------------------------------------------------------------------

namespace dev = alloy::device::selected::device_traits;

using Gpio  = alloy::hal::gpio::lite::port<dev::gpioa>;
using GpioB = alloy::hal::gpio::lite::port<dev::gpiob>;
using Uart1 = alloy::hal::uart::lite::port<dev::usart1>;
using Spi1  = alloy::hal::spi::lite::port<dev::spi1>;
using I2c1  = alloy::hal::i2c::lite::port<dev::i2c1>;
using Tim3  = alloy::hal::timer::lite::port<dev::tim3>;
using Adc1  = alloy::hal::adc::lite::port<dev::adc>;
using Dac1  = alloy::hal::dac::lite::port<dev::dac>;

namespace gpio  = alloy::hal::gpio::lite;
namespace uart  = alloy::hal::uart::lite;
namespace spi   = alloy::hal::spi::lite;
namespace i2c   = alloy::hal::i2c::lite;
namespace timer = alloy::hal::timer::lite;
namespace adc   = alloy::hal::adc::lite;
namespace dac   = alloy::hal::dac::lite;

// ---------------------------------------------------------------------------
// Board constants — Nucleo-G071RB with HSI16 (16 MHz)
// ---------------------------------------------------------------------------

static constexpr std::uint32_t kClockHz   = 16'000'000u;
static constexpr std::uint8_t  kLedPin    = 5u;   ///< PA5 — green LED (LD4)
static constexpr std::uint8_t  kUartTxPin = 9u;   ///< PA9  — USART1 TX (AF1)
static constexpr std::uint8_t  kUartRxPin = 10u;  ///< PA10 — USART1 RX (AF1)
static constexpr std::uint8_t  kSckPin    = 3u;   ///< PB3  — SPI1 SCK  (AF0)
static constexpr std::uint8_t  kMisoPin   = 4u;   ///< PB4  — SPI1 MISO (AF0)
static constexpr std::uint8_t  kMosiPin   = 5u;   ///< PB5  — SPI1 MOSI (AF0)
static constexpr std::uint8_t  kCsPin     = 0u;   ///< PB0  — SPI1 CS (GPIO output)
static constexpr std::uint8_t  kSclPin    = 6u;   ///< PB6  — I2C1 SCL (AF6)
static constexpr std::uint8_t  kSdaPin    = 7u;   ///< PB7  — I2C1 SDA (AF6)
static constexpr std::uint8_t  kAdcPin    = 0u;   ///< PA0  — ADC IN0 (analog)
static constexpr std::uint8_t  kDacPin    = 4u;   ///< PA4  — DAC OUT1 (analog)

// TIM3 @ 16 MHz: PSC=15999, ARR=0 → CK_CNT = 1 kHz → delay_ticks(1) = 1 ms
static constexpr std::uint32_t kTimPsc = 15'999u;
static constexpr std::uint32_t kTimArr = 0u;       // irrelevant for delay_ticks

// ---------------------------------------------------------------------------
// Minimal UART console helpers
// ---------------------------------------------------------------------------

namespace console {

static void write(std::string_view s) noexcept {
    Uart1::write(s);
}

static void writeln(std::string_view s) noexcept {
    Uart1::write(s);
    Uart1::write("\r\n");
}

static void write_hex8(std::uint8_t v) noexcept {
    static constexpr const char* kHex = "0123456789ABCDEF";
    const std::array<char, 2> buf = { kHex[v >> 4u], kHex[v & 0xFu] };
    Uart1::write(std::string_view{buf.data(), 2u});
}

static void write_hex32(std::uint32_t v) noexcept {
    write_hex8(static_cast<std::uint8_t>(v >> 24u));
    write_hex8(static_cast<std::uint8_t>(v >> 16u));
    write_hex8(static_cast<std::uint8_t>(v >>  8u));
    write_hex8(static_cast<std::uint8_t>(v));
}

}  // namespace console

// ---------------------------------------------------------------------------
// LED blink (polling, uses busy loop — pre-timer init)
// ---------------------------------------------------------------------------

static void blink_led(std::uint32_t n) noexcept {
    for (std::uint32_t i = 0u; i < n; ++i) {
        Gpio::set_high(kLedPin);
        for (volatile std::uint32_t d = 0u; d < 200'000u; ++d) { }
        Gpio::set_low(kLedPin);
        for (volatile std::uint32_t d = 0u; d < 200'000u; ++d) { }
    }
}

// ---------------------------------------------------------------------------
// Probe sections
// ---------------------------------------------------------------------------

static void probe_gpio() noexcept {
    console::writeln("--- GPIO ---");

    // PA5 is already configured as output from init.
    Gpio::set_high(kLedPin);
    console::writeln("  PA5 set_high  OK");
    for (volatile std::uint32_t d = 0u; d < 100'000u; ++d) { }
    Gpio::set_low(kLedPin);
    console::writeln("  PA5 set_low   OK");
    Gpio::toggle(kLedPin);
    console::writeln("  PA5 toggle    OK");
    Gpio::toggle(kLedPin);  // back off
    console::writeln("  PA5 final off");
}

static void probe_spi() noexcept {
    console::writeln("--- SPI1 ---");
    console::writeln("  configured: mode0 div/8 8-bit");
    console::write("  tx_empty = ");
    console::writeln(Spi1::tx_empty() ? "true" : "false");
    console::write("  busy     = ");
    console::writeln(Spi1::busy() ? "true" : "false");

    // Assert CS, send 0x9F (generic JEDEC ID command), release CS.
    GpioB::set_low(kCsPin);
    const std::uint8_t rx = Spi1::transfer(0x9Fu);
    GpioB::set_high(kCsPin);
    console::write("  transfer(0x9F) → 0x");
    console::write_hex8(rx);
    console::writeln(" (0xFF expected without device)");
    Spi1::flush();
    console::writeln("  SPI1 probe OK");
}

static void probe_i2c() noexcept {
    console::writeln("--- I2C1 scan 0x08-0x77 ---");
    std::uint8_t found = 0u;
    for (std::uint8_t addr = 0x08u; addr < 0x78u; ++addr) {
        // probe() issues START + addr + STOP (NBYTES=0, AUTOEND=1).
        // ACK → Status::Ok (device present); NACK → Status::Nack → skip.
        const auto status = I2c1::probe(addr);
        if (status == i2c::Status::Ok) {
            console::write("  found 0x");
            console::write_hex8(addr);
            console::writeln("");
            ++found;
        }
    }
    if (found == 0u) {
        console::writeln("  no devices found (normal without I2C hardware)");
    }
    console::writeln("  I2C1 scan done");
}

static void probe_timer() noexcept {
    console::writeln("--- TIM3 ---");
    Tim3::configure(kTimPsc, kTimArr);
    console::writeln("  configured PSC=15999 → 1kHz tick");

    // Blocking 500 ms delay using TIM3 one-pulse mode.
    console::write("  delay_ticks(500) start...");
    Tim3::delay_ticks(500u);
    console::writeln(" done (500 ms elapsed)");

    console::write("  count register = 0x");
    console::write_hex32(Tim3::count());
    console::writeln("");
    console::writeln("  TIM3 probe OK");
}

static void probe_adc() noexcept {
    console::writeln("--- ADC ---");
    // PA0 configured as analog in main() before peripheral init.
    // configure() calibrates and enables; default = 12-bit, all channels.
    Adc1::configure();
    const std::uint16_t raw = Adc1::convert(0u);  // IN0 = PA0
    console::write("  IN0 raw = 0x");
    console::write_hex8(static_cast<std::uint8_t>(raw >> 8u));
    console::write_hex8(static_cast<std::uint8_t>(raw));
    console::writeln(" (12-bit right-aligned)");
    console::writeln("  ADC probe OK");
}

static void probe_dac() noexcept {
    console::writeln("--- DAC ---");
    // PA4 configured as analog in main() — avoids contention with DAC output.
    Dac1::configure_channel(0u);   // channel 1 (index 0), no trigger
    Dac1::enable_channel(0u);      // enable output on PA4
    Dac1::write(0u, 2048u);        // ~Vdda/2  (0x0800)
    const std::uint16_t dor = Dac1::output(0u);
    console::write("  DOR1 = 0x");
    console::write_hex8(static_cast<std::uint8_t>(dor >> 8u));
    console::write_hex8(static_cast<std::uint8_t>(dor));
    console::writeln(" (expect 0x0800)");
    console::writeln("  DAC probe OK");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // ── 1. Enable peripheral clocks and release resets ────────────────────────
    dev::peripheral_on<dev::gpioa>();
    dev::peripheral_on<dev::gpiob>();
    dev::peripheral_on<dev::usart1>();
    dev::peripheral_on<dev::spi1>();
    dev::peripheral_on<dev::i2c1>();
    dev::peripheral_on<dev::tim3>();
    dev::peripheral_on<dev::adc>();
    dev::peripheral_on<dev::dac>();

    // ── 2. GPIO pin configuration ─────────────────────────────────────────────
    // PA5  = LED output
    Gpio::configure_output(kLedPin, gpio::Speed::Low);
    // PA9/PA10 = USART1 TX/RX (AF1 on G071)
    Gpio::configure_af(kUartTxPin, 1u);
    Gpio::configure_af(kUartRxPin, 1u);
    // PB3/4/5 = SPI1 SCK/MISO/MOSI (AF0), PB0 = CS output
    GpioB::configure_af(kSckPin,  0u, gpio::Speed::High);
    GpioB::configure_af(kMisoPin, 0u, gpio::Speed::High);
    GpioB::configure_af(kMosiPin, 0u, gpio::Speed::High);
    GpioB::configure_output(kCsPin, gpio::Speed::High);
    GpioB::set_high(kCsPin);  // CS inactive
    // PB6/7 = I2C1 SCL/SDA (AF6 on G071), open-drain + pull-up
    GpioB::configure_af(kSclPin, 6u, gpio::Speed::High,
                        gpio::Pull::Up, gpio::OutputType::OpenDrain);
    GpioB::configure_af(kSdaPin, 6u, gpio::Speed::High,
                        gpio::Pull::Up, gpio::OutputType::OpenDrain);
    // PA0 = ADC IN0 (analog), PA4 = DAC OUT1 (analog)
    Gpio::configure_analog(kAdcPin);
    Gpio::configure_analog(kDacPin);

    // ── 3. Peripheral configuration ───────────────────────────────────────────
    Uart1::configure({.baudrate = 115200u, .clock_hz = kClockHz});
    Spi1::configure({.baud_div = spi::BaudDiv::Div8});
    I2c1::configure({.timingr = 0x20404768u});  // 100 kHz @ 16 MHz HSI16
    // TIM3 configured inside probe_timer()
    // ADC/DAC configured inside their probe functions

    // ── 4. Startup blink (3×) — UART not ready yet so use LED ────────────────
    blink_led(3u);

    // ── 5. Print header ───────────────────────────────────────────────────────
    console::writeln("");
    console::writeln("=== lite_hal_probe ===");
    console::writeln("Board: Nucleo-G071RB  Clock: HSI16 16MHz");
    console::writeln("Drivers: GPIO UART SPI I2C TIM ADC DAC (alloy.device.v2.1 lite)");
    console::writeln("");

    // ── 6. Run probes ─────────────────────────────────────────────────────────
    probe_gpio();
    console::writeln("");
    probe_spi();
    console::writeln("");
    probe_i2c();
    console::writeln("");
    probe_timer();
    console::writeln("");
    probe_adc();
    console::writeln("");
    probe_dac();
    console::writeln("");

    // ── 7. Done — LED heartbeat ───────────────────────────────────────────────
    console::writeln("=== probe complete — LED heartbeat ===");
    Uart1::flush();

    while (true) {
        Gpio::toggle(kLedPin);
        Tim3::delay_ticks(500u);  // 500 ms
    }
}

#else  // !ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE || !ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE

#include <cstdint>

// Fallback: v2.1 artifacts not generated yet.  Blink fast to signal the issue.

static volatile std::uint32_t* const kGpioARcc =
    reinterpret_cast<volatile std::uint32_t*>(0x40021000u + 0x034u);  // IOPENR
static volatile std::uint32_t* const kGpioAModer =
    reinterpret_cast<volatile std::uint32_t*>(0x50000000u);            // GPIOA MODER
static volatile std::uint32_t* const kGpioABsrr =
    reinterpret_cast<volatile std::uint32_t*>(0x50000018u);            // GPIOA BSRR

int main() {
    // Enable GPIOA clock, configure PA5 as output, blink fast (2 Hz error signal).
    *kGpioARcc |= (1u << 0u);
    *kGpioAModer = (*kGpioAModer & ~(3u << 10u)) | (1u << 10u);  // PA5 output
    while (true) {
        *kGpioABsrr = (1u << 5u);                  // PA5 high
        for (volatile std::uint32_t i = 0u; i < 100'000u; ++i) { }
        *kGpioABsrr = (1u << (5u + 16u));           // PA5 low
        for (volatile std::uint32_t i = 0u; i < 100'000u; ++i) { }
    }
}

#endif  // ALLOY_DEVICE_CODEGEN_FORMAT_AVAILABLE && ALLOY_DEVICE_CODEGEN_RCC_ENABLE_AVAILABLE
