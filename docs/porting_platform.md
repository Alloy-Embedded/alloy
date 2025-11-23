# Porting to a New Platform {#porting_platform}

This guide shows you how to port MicroCore to a **completely new MCU platform** (e.g., new vendor, architecture, or peripheral design).

[TOC]

## Overview

Porting involves implementing **hardware policies** that satisfy MicroCore's **C++20 concepts**. The framework handles the rest.

**Estimated effort:** 20-40 hours for a basic port (GPIO, Clock, UART)

## What Needs Implementation?

| Component | Required | Difficulty | Time |
|-----------|----------|------------|------|
| GPIO Hardware Policy | ✅ Yes | Easy | 4h |
| Clock Configuration | ✅ Yes | Medium | 6h |
| Peripheral Base Addresses | ✅ Yes | Easy | 2h |
| UART Hardware Policy | Recommended | Medium | 6h |
| SysTick Integration | Recommended | Easy | 2h |
| SPI Hardware Policy | Optional | Medium | 8h |
| I2C Hardware Policy | Optional | Medium | 8h |
| Startup Code | Optional* | Hard | 10h |

*Most vendors provide startup code

## Prerequisites

Before starting, gather:

1. **MCU Datasheet** - Register addresses, bitfields
2. **Reference Manual** - Detailed peripheral descriptions
3. **SVD File** (optional) - Can auto-generate registers
4. **Vendor SDK** (optional) - For reference/comparison
5. **Development Board** - For testing

## Step-by-Step Guide

### Step 1: Create Platform Directory Structure

```bash
mkdir -p src/hal/vendors/myvendor/myplatform
cd src/hal/vendors/myvendor/myplatform
```

Create directory structure:

```
src/hal/vendors/myvendor/myplatform/
├── gpio_hardware_policy.hpp  # GPIO implementation
├── clock.hpp                  # Clock configuration
├── peripherals.hpp            # Base addresses
├── uart_hardware_policy.hpp   # UART implementation (optional)
└── mymcu/
    └── generated/
        └── registers.hpp      # Register definitions
```

### Step 2: Define Peripheral Base Addresses

Create `peripherals.hpp`:

```cpp
#pragma once

#include <cstdint>

namespace ucore::generated::mymcu {

/**
 * @brief Peripheral base addresses for MY_MCU
 */
namespace peripherals {
    // GPIO ports
    constexpr uint32_t GPIOA = 0x40020000;
    constexpr uint32_t GPIOB = 0x40020400;
    constexpr uint32_t GPIOC = 0x40020800;
    constexpr uint32_t GPIOD = 0x40020C00;

    // System peripherals
    constexpr uint32_t RCC   = 0x40023800;  // Reset and Clock Control
    constexpr uint32_t PWR   = 0x40007000;  // Power Control

    // Communication peripherals
    constexpr uint32_t UART0 = 0x40011000;
    constexpr uint32_t UART1 = 0x40011400;
    constexpr uint32_t SPI0  = 0x40013000;
    constexpr uint32_t I2C0  = 0x40005400;

    // Timers
    constexpr uint32_t TIM1  = 0x40010000;
    constexpr uint32_t SYSTICK = 0xE000E010;  // ARM Cortex-M SysTick
}

} // namespace ucore::generated::mymcu
```

**Finding addresses:**
- Check MCU datasheet "Memory Map" section
- Look for peripheral base addresses table
- Or use SVD file to auto-generate

### Step 3: Define Register Structures

Create `mymcu/generated/registers.hpp`:

```cpp
#pragma once

#include <cstdint>

namespace ucore::generated::mymcu {

/**
 * @brief GPIO Register Map
 *
 * Based on MY_MCU Reference Manual section X.Y
 */
struct GPIO_Registers {
    volatile uint32_t MODER;     // 0x00: Mode register
    volatile uint32_t OTYPER;    // 0x04: Output type
    volatile uint32_t OSPEEDR;   // 0x08: Output speed
    volatile uint32_t PUPDR;     // 0x0C: Pull-up/pull-down
    volatile uint32_t IDR;       // 0x10: Input data
    volatile uint32_t ODR;       // 0x14: Output data
    volatile uint32_t BSRR;      // 0x18: Bit set/reset
    volatile uint32_t LCKR;      // 0x1C: Lock register
    volatile uint32_t AFRL;      // 0x20: Alternate function low
    volatile uint32_t AFRH;      // 0x24: Alternate function high
};

/**
 * @brief UART Register Map
 */
struct UART_Registers {
    volatile uint32_t CR1;       // 0x00: Control register 1
    volatile uint32_t CR2;       // 0x04: Control register 2
    volatile uint32_t CR3;       // 0x08: Control register 3
    volatile uint32_t BRR;       // 0x0C: Baud rate register
    volatile uint32_t SR;        // 0x10: Status register
    volatile uint32_t DR;        // 0x14: Data register
};

/**
 * @brief RCC Register Map (Reset and Clock Control)
 */
struct RCC_Registers {
    volatile uint32_t CR;        // 0x00: Clock control
    volatile uint32_t PLLCFGR;   // 0x04: PLL configuration
    volatile uint32_t CFGR;      // 0x08: Clock configuration
    volatile uint32_t CIR;       // 0x0C: Clock interrupt
    volatile uint32_t AHB1ENR;   // 0x30: AHB1 peripheral clock enable
    volatile uint32_t APB1ENR;   // 0x40: APB1 peripheral clock enable
    volatile uint32_t APB2ENR;   // 0x44: APB2 peripheral clock enable
};

} // namespace ucore::generated::mymcu
```

**Tips:**
- Match register order exactly as in datasheet
- Use `volatile` for all hardware registers
- Document offset addresses in comments
- Use `uint32_t` for 32-bit registers

### Step 4: Implement GPIO Hardware Policy

Create `gpio_hardware_policy.hpp`:

```cpp
#pragma once

#include "mymcu/generated/registers.hpp"
#include "peripherals.hpp"
#include <cstdint>

namespace ucore::hal::myvendor::myplatform {

using namespace ucore::generated::mymcu;

/**
 * @brief GPIO Hardware Policy for MY_MCU
 *
 * Implements GpioPin concept using CRTP pattern.
 *
 * @tparam Port GPIO port base address
 * @tparam Pin Pin number (0-15)
 *
 * Usage:
 * @code
 * using Led = GpioHardwarePolicy<peripherals::GPIOA, 5>;
 * Led::configure_output();
 * Led::set_high();
 * @endcode
 */
template <uint32_t Port, uint8_t Pin>
class GpioHardwarePolicy {
public:
    static constexpr uint32_t port = Port;
    static constexpr uint8_t pin = Pin;
    static constexpr uint32_t pin_mask = (1u << Pin);

    /**
     * @brief Configure pin as output
     */
    static void configure_output() {
        auto& gpio = get_registers();

        // Clear mode bits (2 bits per pin)
        uint32_t mode = gpio.MODER;
        mode &= ~(0b11 << (Pin * 2));
        mode |= (0b01 << (Pin * 2));  // 01 = output
        gpio.MODER = mode;
    }

    /**
     * @brief Configure pin as input
     */
    static void configure_input() {
        auto& gpio = get_registers();

        // Clear mode bits (00 = input)
        uint32_t mode = gpio.MODER;
        mode &= ~(0b11 << (Pin * 2));
        gpio.MODER = mode;
    }

    /**
     * @brief Set pin HIGH
     */
    static void set_high() {
        auto& gpio = get_registers();
        gpio.BSRR = pin_mask;  // Atomic set
    }

    /**
     * @brief Set pin LOW
     */
    static void set_low() {
        auto& gpio = get_registers();
        gpio.BSRR = (pin_mask << 16);  // Atomic reset
    }

    /**
     * @brief Toggle pin state
     */
    static void toggle() {
        auto& gpio = get_registers();
        gpio.ODR ^= pin_mask;
    }

    /**
     * @brief Read pin state
     *
     * @return true if pin is HIGH, false if LOW
     */
    static bool read() {
        auto& gpio = get_registers();

        // Check if configured as input or output
        uint32_t mode = gpio.MODER;
        uint32_t pin_mode = (mode >> (Pin * 2)) & 0b11;

        if (pin_mode == 0b00) {
            // Input mode - read from IDR
            return (gpio.IDR & pin_mask) != 0;
        } else {
            // Output mode - read from ODR
            return (gpio.ODR & pin_mask) != 0;
        }
    }

    /**
     * @brief Configure pull-up resistor
     */
    static void configure_pull_up() {
        auto& gpio = get_registers();

        uint32_t pupd = gpio.PUPDR;
        pupd &= ~(0b11 << (Pin * 2));
        pupd |= (0b01 << (Pin * 2));  // 01 = pull-up
        gpio.PUPDR = pupd;
    }

    /**
     * @brief Configure pull-down resistor
     */
    static void configure_pull_down() {
        auto& gpio = get_registers();

        uint32_t pupd = gpio.PUPDR;
        pupd &= ~(0b11 << (Pin * 2));
        pupd |= (0b10 << (Pin * 2));  // 10 = pull-down
        gpio.PUPDR = pupd;
    }

private:
    /**
     * @brief Get GPIO register structure for this port
     */
    static GPIO_Registers& get_registers() {
        return *reinterpret_cast<GPIO_Registers*>(Port);
    }
};

} // namespace ucore::hal::myvendor::myplatform
```

**Key points:**
- Use CRTP template pattern
- All methods must be `static`
- Use atomic operations (BSRR) when possible
- Match datasheet bit patterns exactly

### Step 5: Verify GPIO Satisfies Concept

Create a test file to verify:

```cpp
#include "hal/concepts/gpio_pin.hpp"
#include "hal/vendors/myvendor/myplatform/gpio_hardware_policy.hpp"

using namespace ucore::hal::myvendor::myplatform;
using namespace ucore::generated::mymcu;

// Compile-time verification
using TestPin = GpioHardwarePolicy<peripherals::GPIOA, 5>;

// This will fail at compile-time if concept not satisfied
static_assert(GpioPin<TestPin>, "GpioHardwarePolicy must satisfy GpioPin concept");
```

### Step 6: Implement Clock Configuration

Create `clock.hpp`:

```cpp
#pragma once

#include "core/result.hpp"
#include "core/error_code.hpp"
#include "mymcu/generated/registers.hpp"
#include "peripherals.hpp"

namespace ucore::hal::myvendor::myplatform {

using namespace ucore::core;
using namespace ucore::generated::mymcu;

/**
 * @brief Clock configuration structure
 */
struct ClockConfig {
    uint32_t hse_hz;              // External crystal frequency
    uint32_t system_clock_hz;     // Target system clock
    uint32_t pll_m;               // PLL input divider
    uint32_t pll_n;               // PLL multiplier
    uint32_t pll_p_div;           // System clock divider
    uint32_t pll_q;               // USB clock divider
    uint32_t ahb_prescaler;       // AHB bus prescaler
    uint32_t apb1_prescaler;      // APB1 bus prescaler
    uint32_t apb2_prescaler;      // APB2 bus prescaler
    uint32_t flash_latency;       // Flash wait states
};

/**
 * @brief System Clock Management
 */
class Clock {
public:
    /**
     * @brief Initialize system clock with PLL
     *
     * @param config Clock configuration
     * @return Result<void> Success or error code
     */
    static Result<void> initialize(const ClockConfig& config) {
        auto& rcc = get_rcc();

        // 1. Enable HSE (external crystal)
        rcc.CR |= (1 << 16);  // HSEON = 1

        // Wait for HSE to stabilize
        uint32_t timeout = 100000;
        while (!(rcc.CR & (1 << 17)) && timeout--) {
            // Wait for HSERDY
        }

        if (timeout == 0) {
            return Result<void>::err(ErrorCode::CLOCK_INIT_FAILED);
        }

        // 2. Configure Flash latency
        set_flash_latency(config.flash_latency);

        // 3. Configure PLL
        uint32_t pllcfgr = 0;
        pllcfgr |= (config.pll_m << 0);     // PLLM
        pllcfgr |= (config.pll_n << 6);     // PLLN
        pllcfgr |= (((config.pll_p_div / 2) - 1) << 16);  // PLLP
        pllcfgr |= (config.pll_q << 24);    // PLLQ
        pllcfgr |= (1 << 22);               // PLLSRC = HSE

        rcc.PLLCFGR = pllcfgr;

        // 4. Enable PLL
        rcc.CR |= (1 << 24);  // PLLON = 1

        // Wait for PLL to lock
        timeout = 100000;
        while (!(rcc.CR & (1 << 25)) && timeout--) {
            // Wait for PLLRDY
        }

        if (timeout == 0) {
            return Result<void>::err(ErrorCode::CLOCK_INIT_FAILED);
        }

        // 5. Configure bus prescalers
        uint32_t cfgr = rcc.CFGR;
        cfgr &= ~(0xF << 4);   // Clear HPRE
        cfgr |= (ahb_prescaler_to_bits(config.ahb_prescaler) << 4);

        cfgr &= ~(0x7 << 10);  // Clear PPRE1
        cfgr |= (apb_prescaler_to_bits(config.apb1_prescaler) << 10);

        cfgr &= ~(0x7 << 13);  // Clear PPRE2
        cfgr |= (apb_prescaler_to_bits(config.apb2_prescaler) << 13);

        rcc.CFGR = cfgr;

        // 6. Switch system clock to PLL
        cfgr = rcc.CFGR;
        cfgr &= ~(0x3 << 0);   // Clear SW
        cfgr |= (0x2 << 0);    // SW = PLL
        rcc.CFGR = cfgr;

        // Wait for PLL to be used
        timeout = 100000;
        while (((rcc.CFGR >> 2) & 0x3) != 0x2 && timeout--) {
            // Wait for SWS = PLL
        }

        if (timeout == 0) {
            return Result<void>::err(ErrorCode::CLOCK_INIT_FAILED);
        }

        // Store current clock configuration
        current_config = config;

        return Result<void>::ok();
    }

    /**
     * @brief Get system clock frequency
     */
    static uint32_t get_system_clock_hz() {
        return current_config.system_clock_hz;
    }

    /**
     * @brief Get AHB clock frequency
     */
    static uint32_t get_ahb_clock_hz() {
        return current_config.system_clock_hz / current_config.ahb_prescaler;
    }

    /**
     * @brief Get APB1 clock frequency
     */
    static uint32_t get_apb1_clock_hz() {
        return get_ahb_clock_hz() / current_config.apb1_prescaler;
    }

    /**
     * @brief Get APB2 clock frequency
     */
    static uint32_t get_apb2_clock_hz() {
        return get_ahb_clock_hz() / current_config.apb2_prescaler;
    }

private:
    static inline ClockConfig current_config{};

    static RCC_Registers& get_rcc() {
        return *reinterpret_cast<RCC_Registers*>(peripherals::RCC);
    }

    static void set_flash_latency(uint32_t latency) {
        volatile uint32_t* FLASH_ACR = reinterpret_cast<volatile uint32_t*>(0x40023C00);
        *FLASH_ACR = (*FLASH_ACR & ~0x7) | (latency & 0x7);
    }

    static uint32_t ahb_prescaler_to_bits(uint32_t prescaler) {
        // Convert prescaler value to register bits
        switch (prescaler) {
            case 1: return 0b0000;
            case 2: return 0b1000;
            case 4: return 0b1001;
            case 8: return 0b1010;
            case 16: return 0b1011;
            case 64: return 0b1100;
            case 128: return 0b1101;
            case 256: return 0b1110;
            case 512: return 0b1111;
            default: return 0b0000;
        }
    }

    static uint32_t apb_prescaler_to_bits(uint32_t prescaler) {
        switch (prescaler) {
            case 1: return 0b000;
            case 2: return 0b100;
            case 4: return 0b101;
            case 8: return 0b110;
            case 16: return 0b111;
            default: return 0b000;
        }
    }
};

} // namespace ucore::hal::myvendor::myplatform
```

### Step 7: Add RCC Helper Functions

Create `rcc.hpp` for enabling peripheral clocks:

```cpp
#pragma once

#include "mymcu/generated/registers.hpp"
#include "peripherals.hpp"

namespace ucore::hal::myvendor::myplatform {

class RCC {
public:
    static void enable_gpioa_clock() {
        auto& rcc = get_rcc();
        rcc.AHB1ENR |= (1 << 0);  // GPIOAEN
    }

    static void enable_gpiob_clock() {
        auto& rcc = get_rcc();
        rcc.AHB1ENR |= (1 << 1);  // GPIOBEN
    }

    static void enable_uart0_clock() {
        auto& rcc = get_rcc();
        rcc.APB2ENR |= (1 << 4);  // UART0EN
    }

    // Add more enable_xxx_clock() methods as needed

private:
    static RCC_Registers& get_rcc() {
        return *reinterpret_cast<RCC_Registers*>(peripherals::RCC);
    }
};

} // namespace ucore::hal::myvendor::myplatform
```

### Step 8: Create CMake Integration

Create `CMakeLists.txt` in platform directory:

```cmake
# MY_PLATFORM HAL Implementation

add_library(ucore_hal_myplatform INTERFACE)

target_sources(ucore_hal_myplatform INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_hardware_policy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/clock.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rcc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/peripherals.hpp
)

target_include_directories(ucore_hal_myplatform INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ucore_hal_myplatform INTERFACE
    ucore::core
)

# Add alias
add_library(ucore::hal::myplatform ALIAS ucore_hal_myplatform)
```

### Step 9: Create a Test Board

Create `boards/my_test_board/board.yaml`:

```yaml
board:
  name: "My Test Board"
  vendor: "My Company"
  version: "1.0.0"

platform: myplatform

mcu:
  part_number: "MY_MCU123"
  architecture: cortex-m4
  frequency_mhz: 100

clock:
  source: PLL
  system_clock_hz: 100000000
  hse_hz: 8000000
  pll:
    m: 8
    n: 400
    p: 4
    q: 8
  ahb_prescaler: 1
  apb1_prescaler: 2
  apb2_prescaler: 1
  flash_latency: 3

leds:
  - name: led_green
    port: GPIOA
    pin: 5
    active_high: true
```

### Step 10: Test Your Port

```bash
# Generate board config
./ucore generate-board my_test_board

# Build blink example
./ucore build my_test_board blink

# Flash and test
./ucore flash my_test_board blink
```

## Testing Checklist

- [ ] GPIO output works (LED blinks)
- [ ] GPIO input works (button reads)
- [ ] Clock configuration correct (verify with oscilloscope/logic analyzer)
- [ ] UART transmission works
- [ ] UART reception works
- [ ] SPI transfer works
- [ ] I2C communication works
- [ ] Interrupts work
- [ ] All concepts satisfied (static_assert)

## Common Pitfalls

### 1. Incorrect Register Offsets

**Problem:** Registers don't work
**Solution:** Double-check datasheet offsets. Use `static_assert`:

```cpp
static_assert(offsetof(GPIO_Registers, ODR) == 0x14, "ODR offset wrong");
```

### 2. Missing `volatile`

**Problem:** Compiler optimizes away register accesses
**Solution:** Always use `volatile` for hardware registers

### 3. Wrong Bit Patterns

**Problem:** Configuration doesn't work
**Solution:** Verify bit patterns match datasheet exactly

### 4. Clock Timing Issues

**Problem:** Random crashes, unstable operation
**Solution:** Check flash latency matches frequency

## Optimization

After basic port works, optimize:

### 1. Use Inline Assembly for Critical Paths

```cpp
static void set_high() {
    asm volatile("str %0, [%1]" : : "r"(pin_mask), "r"(&gpio->BSRR));
}
```

### 2. Constexpr Where Possible

```cpp
static constexpr uint32_t calculate_baud_divisor(uint32_t clock, uint32_t baud) {
    return (clock + (baud / 2)) / baud;
}
```

### 3. Template Metaprogramming

```cpp
template <uint32_t Frequency>
struct ClockDivider {
    static constexpr uint32_t ahb = /* calculated */;
    static constexpr uint32_t apb1 = /* calculated */;
};
```

## Documentation

Document your port:

1. **README.md** in platform directory
2. **Doxygen comments** on all public APIs
3. **Examples** for each peripheral
4. **Known limitations**
5. **Errata workarounds**

## Contributing Your Port

1. Follow MicroCore coding style
2. Add unit tests (host platform)
3. Add integration tests (hardware)
4. Update platform support matrix
5. Create pull request

## See Also

- @ref concepts - C++20 concepts your port must satisfy
- @ref adding_board - Creating boards for your platform
- [MicroCore Contributing Guide](../CONTRIBUTING.md)
- Example ports: `src/hal/vendors/st/stm32f4/`, `src/hal/vendors/same70/`

---

**Estimated total time:** 20-40 hours for GPIO + Clock + UART basic port
