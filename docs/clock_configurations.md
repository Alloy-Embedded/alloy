# Clock Configurations

This document describes clock tree configurations for all supported MCUs.

## Overview

Each MCU has different clock sources and PLL configurations. Alloy configures clocks to achieve maximum performance while maintaining stability.

## Clock Summary

| MCU | Source | System Clock | Flash Latency | APB1 | APB2 |
|-----|--------|--------------|---------------|------|------|
| STM32F103C8 | 8 MHz HSE → PLL ×9 | 72 MHz | 2 WS | 36 MHz | 72 MHz |
| STM32F407VG | 8 MHz HSE → PLL | 168 MHz | 5 WS | 42 MHz | 84 MHz |
| RP2040 | 12 MHz XOSC → PLL | 125 MHz | N/A | N/A | N/A |
| ATSAMD21G18 | 32 kHz XOSC32K → DFLL48M | 48 MHz | 1 WS | N/A | N/A |
| ESP32 | 40 MHz XTAL → PLL | 160 MHz | N/A | 80 MHz | N/A |

---

## STM32F103C8 (Blue Pill)

### Clock Tree

```
HSE (8 MHz External Crystal)
    ↓
PLL (×9)
    ↓
System Clock (72 MHz) ← Maximum allowed
    ↓
    ├─→ AHB (72 MHz) ← CPU, DMA, Flash
    │      ├─→ APB1 (36 MHz max) ← Timers 2-7, USART2-3, I2C, SPI2
    │      └─→ APB2 (72 MHz) ← Timers 1,8, USART1, SPI1, ADC
    └─→ Flash Latency: 2 Wait States (required @ 72MHz)
```

### Configuration
- **System Clock**: 72 MHz (max for STM32F103)
- **AHB Bus**: 72 MHz (no divider)
- **APB1 Bus**: 36 MHz (÷2 - max is 36MHz)
- **APB2 Bus**: 72 MHz (no divider)
- **Flash Latency**: 2 wait states (48-72 MHz)
- **PLL Source**: HSE (8 MHz external crystal)
- **PLL Multiplier**: ×9 (8MHz × 9 = 72MHz)

### Code
```cpp
// Alloy configures this automatically in Board::initialize()
// Manual config (if needed):
// 1. Enable HSE and wait for ready
// 2. Configure Flash latency (2 WS)
// 3. Set PLL to HSE × 9
// 4. Set AHB = /1, APB1 = /2, APB2 = /1
// 5. Enable PLL and wait
// 6. Switch to PLL as system clock
```

---

## STM32F407VG (Discovery)

### Clock Tree

```
HSE (8 MHz External Crystal)
    ↓
Main PLL (M=8, N=336, P=2)
    ↓
System Clock (168 MHz) ← Maximum allowed
    ↓
    ├─→ AHB (168 MHz) ← CPU, DMA, Flash, Memory
    │      ├─→ APB1 (42 MHz max) ← Timers 2-7,12-14, USART2-3, I2C, SPI2-3
    │      └─→ APB2 (84 MHz max) ← Timers 1,8-11, USART1,6, SPI1, ADC
    └─→ Flash Latency: 5 Wait States (required @ 168MHz with 3.3V)
```

### PLL Calculation
```
PLL = (HSE / M) × N / P
    = (8 MHz / 8) × 336 / 2
    = 1 MHz × 336 / 2
    = 168 MHz
```

### Configuration
- **System Clock**: 168 MHz (max for STM32F407 @ 3.3V)
- **AHB Bus**: 168 MHz (no divider)
- **APB1 Bus**: 42 MHz (÷4 - max is 42MHz)
- **APB2 Bus**: 84 MHz (÷2 - max is 84MHz)
- **Flash Latency**: 5 wait states (150-168 MHz @ 3.3V)
- **PLL Config**: M=8, N=336, P=2, Q=7

---

## RP2040 (Raspberry Pi Pico)

### Clock Tree

```
XOSC (12 MHz External Crystal)
    ↓
PLL_SYS (REFDIV=1, FBDIV=125, POSTDIV1=6, POSTDIV2=2)
    ↓
System Clock (125 MHz) ← Conservative (max 133 MHz possible)
    ↓
    ├─→ clk_sys (125 MHz) ← Processors, Bus fabric, Memories
    ├─→ clk_peri (125 MHz) ← Peripherals (UART, SPI, I2C, PWM)
    └─→ clk_usb (48 MHz) ← PLL_USB for USB peripheral
```

### PLL Calculation
```
PLL_SYS = (XOSC / REFDIV) × FBDIV / (POSTDIV1 × POSTDIV2)
        = (12 MHz / 1) × 125 / (6 × 2)
        = 12 MHz × 125 / 12
        = 125 MHz
```

### Configuration
- **System Clock**: 125 MHz (conservative, 133 MHz max)
- **Peripheral Clock**: 125 MHz
- **USB Clock**: 48 MHz (from separate PLL_USB)
- **XOSC**: 12 MHz external crystal
- **PLL_SYS**: FBDIV=125, POSTDIV1=6, POSTDIV2=2
- **PLL_USB**: Configured separately for 48 MHz

### Special Notes
- RP2040 has **two independent PLLs**: SYS and USB
- Can overclock to 133MHz+ (not recommended without cooling)
- Dual-core: both cores run at same frequency

---

## ATSAMD21G18 (Arduino Zero)

### Clock Tree

```
XOSC32K (32.768 kHz External Crystal)
    ↓
DFLL48M (×1464.8438)
    ↓
System Clock (48 MHz) ← Maximum and required for USB
    ↓
    ├─→ CPU Clock (48 MHz)
    ├─→ APBA Clock (48 MHz) ← PM, SYSCTRL, GCLK
    ├─→ APBB Clock (48 MHz) ← PORT, NVMCTRL, USB
    └─→ APBC Clock (48 MHz) ← SERCOM, TCC, TC

GCLK0 (48 MHz) → Feeds peripherals via GCLKs
```

### DFLL Configuration
```
DFLL48M = XOSC32K × Multiplier
        = 32.768 kHz × 1464.8438
        = 48 MHz

(1464.8438 = 48,000,000 / 32,768)
```

### Configuration
- **System Clock**: 48 MHz (required for USB, maximum for SAMD21)
- **CPU Clock**: 48 MHz
- **APB Buses**: All at 48 MHz (no division)
- **Flash Wait States**: 1 (required above 24 MHz)
- **Clock Source**: DFLL48M (fed by 32kHz XOSC32K)
- **DFLL Multiplier**: ×1465 (approximately)

### Special Notes
- **DFLL48M**: Digital Frequency Locked Loop, locks to 32kHz crystal
- **GCLK**: Generic Clock Controller routes clocks to peripherals
- USB **requires** exactly 48 MHz - cannot use different frequency

---

## ESP32 (ESP32-WROOM-32)

### Clock Tree

```
XTAL (40 MHz External Crystal)
    ↓
PLL (×4 or ×6)
    ↓
CPU Clock (160 MHz or 240 MHz)
    ↓
    ├─→ CPU_CLK (160/240 MHz) ← Both cores
    ├─→ APB_CLK (80 MHz) ← Fixed at 80 MHz
    └─→ REF_CLK (1 MHz) ← Reference for timers

RTC: 8 MHz internal (RTC_FAST) or 150 kHz (RTC_SLOW)
```

### PLL Configuration
```
CPU 160 MHz: XTAL (40 MHz) × 4 = 160 MHz
CPU 240 MHz: XTAL (40 MHz) × 6 = 240 MHz
APB: CPU_CLK / 2 = 80 MHz (fixed)
```

### Configuration (Alloy Default)
- **CPU Clock**: 160 MHz (conservative, 240 MHz possible)
- **APB Clock**: 80 MHz (always CPU/2)
- **XTAL**: 40 MHz (auto-detected, can be 26 or 40 MHz)
- **PLL Multiplier**: ×4 (for 160 MHz)
- **RTC**: 8 MHz internal (RTC_FAST_CLK)

### Power Modes
- **240 MHz**: Maximum performance, higher power (~80-240 mA)
- **160 MHz**: Balanced (default in Alloy) (~80-200 mA)
- **80 MHz**: Low power CPU mode (~30-80 mA)
- **Light Sleep**: RTC only, wake on timer/GPIO (~0.8 mA)
- **Deep Sleep**: Ultra-low power (~10 μA)

### Special Notes
- ESP32 has **2 Xtensa cores** (PRO_CPU and APP_CPU)
- Both cores run at same frequency
- APB clock is **always** CPU_CLK / 2 = 80 MHz
- WiFi/BT require minimum 80 MHz CPU clock

---

## Clock API Usage

### Query Current Frequency
```cpp
#include "board.hpp"

uint32_t sys_freq = Board::system_clock_hz;
// Returns: 72000000 (STM32F103), 168000000 (STM32F407), etc.
```

### Change Frequency (if supported)
```cpp
// Not all MCUs support runtime frequency changes
// ESP32 example:
esp_pm_config_esp32_t pm_config = {
    .max_freq_mhz = 160,
    .min_freq_mhz = 80,
    .light_sleep_enable = false
};
esp_pm_configure(&pm_config);
```

## Timing Considerations

### Delay Functions
Delay accuracy depends on clock configuration:

```cpp
Board::delay_ms(1000);  // 1 second delay

// Internally uses system clock:
// STM32F103 @ 72MHz: 72,000,000 cycles / second
// Accuracy: ±crystal tolerance (typically ±20 ppm)
```

### Peripheral Clocks
Different peripherals run at different clocks:

| Peripheral | STM32F103 | STM32F407 | RP2040 | SAMD21 | ESP32 |
|------------|-----------|-----------|--------|--------|-------|
| UART | APB1/2 | APB1/2 | clk_peri | GCLK | APB (80MHz) |
| I2C | APB1 | APB1/2 | clk_peri | GCLK | APB (80MHz) |
| SPI | APB1/2 | APB1/2 | clk_peri | GCLK | APB (80MHz) |
| Timers | APB1/2 | APB1/2 | clk_sys | GCLK | APB (80MHz) |
| ADC | ADC_CLK | ADC_CLK | ADC_CLK | GCLK | APB (80MHz) |

## Troubleshooting

### "Code runs slower than expected"
- Check system clock is configured correctly
- Verify flash latency settings (STM32)
- Check for busy-wait loops

### "UART baud rate incorrect"
- Verify APB clock frequency
- Check peripheral clock enable
- Calculate baud rate divisor based on actual APB clock

### "USB not working" (SAMD21, ESP32)
- SAMD21: Must use exactly 48 MHz
- ESP32: Ensure CPU >= 80 MHz for USB peripheral

### "Timing drift"
- Check crystal accuracy (±20-50 ppm typical)
- Use RTC with 32kHz crystal for long-term timing
- Consider temperature effects on crystal frequency

## See Also

- [Boards Documentation](boards.md) - Board specifications
- [Board Header Files](../boards/) - Clock constants
- MCU datasheets and reference manuals for detailed clock trees
