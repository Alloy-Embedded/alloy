# Complete Register Map Usage Guide

## Overview

The `register_map.hpp` file is the **single include** that provides access to all generated register definitions, bit fields, enumerations, and pin functions for an MCU. This is the main convenience header that most users will include.

## Generation

```bash
# Generate complete register map
./codegen generate --register-map

# Generate everything (includes register map)
./codegen generate
```

## Generated Files

For each MCU, a complete directory structure is created:

```
src/hal/vendors/st/stm32f1/stm32f103xx/
├── register_map.hpp          # ← Single include for everything
├── bitfield_utils.hpp         # Template utilities
├── enums.hpp                  # Enumerated values
├── pin_functions.hpp          # Pin AF mappings
├── registers/                 # Register structures
│   ├── rcc_registers.hpp
│   ├── gpio_registers.hpp
│   ├── usart_registers.hpp
│   └── ... (30+ peripherals)
└── bitfields/                 # Bit field definitions
    ├── rcc_bitfields.hpp
    ├── gpio_bitfields.hpp
    ├── usart_bitfields.hpp
    └── ... (30+ peripherals)
```

## Basic Usage

### Single Include

```cpp
// Include everything with one header
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

// Use the namespace
using namespace alloy::hal::st::stm32f1::stm32f103xx;

int main() {
    // Access all peripherals
    rcc::RCC->CR = rcc::cr::HSEON::set(rcc::RCC->CR);
    gpio::GPIOC->ODR = 0x2000;  // Turn on LED
    usart::USART1->BRR = 0x1D4C;  // Configure baud rate

    return 0;
}
```

### Selective Namespace Usage

```cpp
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

// Use only specific peripherals
using namespace alloy::hal::st::stm32f1::stm32f103xx::rcc;
using namespace alloy::hal::st::stm32f1::stm32f103xx::gpio;

int main() {
    // Now you can use RCC and GPIO directly
    RCC->CR = cr::HSEON::set(RCC->CR);
    GPIOC->ODR = 0x2000;

    return 0;
}
```

### Namespace Alias

```cpp
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

// Create short alias
namespace mcu = alloy::hal::st::stm32f1::stm32f103xx;

int main() {
    // Use through alias
    mcu::rcc::RCC->CR = mcu::rcc::cr::HSEON::set(mcu::rcc::RCC->CR);

    return 0;
}
```

## Complete Example: LED Blink

```cpp
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

using namespace alloy::hal::st::stm32f1::stm32f103xx;

void delay(uint32_t count) {
    while (count--) {
        __asm__ volatile("nop");
    }
}

int main() {
    // 1. Enable GPIOC clock
    rcc::RCC->APB2ENR = rcc::apb2enr::IOPCEN::set(rcc::RCC->APB2ENR);

    // 2. Configure PC13 as output (50MHz, push-pull)
    // Set MODE13[1:0] = 11 (output 50MHz)
    gpio::GPIOC->CRH = gpio::crh::MODE13::write(gpio::GPIOC->CRH, 0b11);
    // Set CNF13[1:0] = 00 (push-pull)
    gpio::GPIOC->CRH = gpio::crh::CNF13::write(gpio::GPIOC->CRH, 0b00);

    // 3. Blink LED
    while (true) {
        // Turn on LED (set bit 13)
        gpio::GPIOC->ODR = gpio::odr::ODR13::set(gpio::GPIOC->ODR);
        delay(1000000);

        // Turn off LED (clear bit 13)
        gpio::GPIOC->ODR = gpio::odr::ODR13::clear(gpio::GPIOC->ODR);
        delay(1000000);
    }

    return 0;
}
```

## Complete Example: UART Configuration

```cpp
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

using namespace alloy::hal::st::stm32f1::stm32f103xx;

void uart_init() {
    // 1. Enable clocks
    rcc::RCC->APB2ENR = rcc::apb2enr::USART1EN::set(rcc::RCC->APB2ENR);
    rcc::RCC->APB2ENR = rcc::apb2enr::IOPAEN::set(rcc::RCC->APB2ENR);

    // 2. Configure pins (PA9=TX, PA10=RX)
    // Get AF numbers (if pin_functions available)
    constexpr uint8_t tx_af = pin_functions::AF<9, pin_functions::USART1_TX>;
    constexpr uint8_t rx_af = pin_functions::AF<10, pin_functions::USART1_RX>;

    // PA9 - TX (AF push-pull)
    gpio::GPIOA->CRH = gpio::crh::MODE9::write(gpio::GPIOA->CRH, 0b11);   // 50MHz
    gpio::GPIOA->CRH = gpio::crh::CNF9::write(gpio::GPIOA->CRH, 0b10);    // AF push-pull

    // PA10 - RX (Input floating)
    gpio::GPIOA->CRH = gpio::crh::MODE10::write(gpio::GPIOA->CRH, 0b00);  // Input
    gpio::GPIOA->CRH = gpio::crh::CNF10::write(gpio::GPIOA->CRH, 0b01);   // Floating

    // 3. Configure UART
    // Baud rate: 115200 @ 72MHz
    usart::USART1->BRR = 0x1D4C;

    // Enable TX, RX, USART
    usart::USART1->CR1 = usart::cr1::TE::set(usart::USART1->CR1);   // TX enable
    usart::USART1->CR1 = usart::cr1::RE::set(usart::USART1->CR1);   // RX enable
    usart::USART1->CR1 = usart::cr1::UE::set(usart::USART1->CR1);   // USART enable
}

void uart_putc(char c) {
    // Wait for TX empty
    while (!usart::sr::TXE::test(usart::USART1->SR)) { }

    // Write data
    usart::USART1->DR = c;
}

void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

int main() {
    uart_init();
    uart_puts("Hello, World!\r\n");

    while (true) {
        // Echo received characters
        if (usart::sr::RXNE::test(usart::USART1->SR)) {
            char c = usart::USART1->DR;
            uart_putc(c);
        }
    }

    return 0;
}
```

## Complete Example: System Clock Configuration

```cpp
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

using namespace alloy::hal::st::stm32f1::stm32f103xx;

void system_clock_init() {
    // Configure system clock to 72MHz using HSE and PLL

    // 1. Enable HSE (High Speed External oscillator)
    rcc::RCC->CR = rcc::cr::HSEON::set(rcc::RCC->CR);

    // 2. Wait for HSE ready
    while (!rcc::cr::HSERDY::test(rcc::RCC->CR)) { }

    // 3. Configure Flash latency (2 wait states for 72MHz)
    flash::FLASH->ACR = flash::acr::LATENCY::write(flash::FLASH->ACR, 0b010);

    // 4. Configure PLL: HSE x 9 = 72MHz (8MHz * 9)
    rcc::RCC->CFGR = rcc::cfgr::PLLSRC::set(rcc::RCC->CFGR);        // HSE as PLL source
    rcc::RCC->CFGR = rcc::cfgr::PLLMUL::write(rcc::RCC->CFGR, 0b0111);  // x9 multiplier

    // 5. Enable PLL
    rcc::RCC->CR = rcc::cr::PLLON::set(rcc::RCC->CR);

    // 6. Wait for PLL ready
    while (!rcc::cr::PLLRDY::test(rcc::RCC->CR)) { }

    // 7. Configure bus dividers
    rcc::RCC->CFGR = rcc::cfgr::HPRE::write(rcc::RCC->CFGR, 0b0000);   // AHB = SYSCLK
    rcc::RCC->CFGR = rcc::cfgr::PPRE1::write(rcc::RCC->CFGR, 0b100);   // APB1 = HCLK/2
    rcc::RCC->CFGR = rcc::cfgr::PPRE2::write(rcc::RCC->CFGR, 0b000);   // APB2 = HCLK

    // 8. Select PLL as system clock
    rcc::RCC->CFGR = rcc::cfgr::SW::write(rcc::RCC->CFGR, 0b10);  // PLL

    // 9. Wait for PLL to be used as system clock
    while (rcc::cfgr::SWS::read(rcc::RCC->CFGR) != 0b10) { }

    // System clock is now 72MHz!
}

int main() {
    system_clock_init();

    // Continue with application...
    return 0;
}
```

## Benefits of Single Include

### 1. Simplicity

```cpp
// ✅ Simple - one include
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

// ❌ Complex - multiple includes
#include <hal/st/stm32f1/stm32f103xx/registers/rcc_registers.hpp>
#include <hal/st/stm32f1/stm32f103xx/registers/gpio_registers.hpp>
#include <hal/st/stm32f1/stm32f103xx/registers/usart_registers.hpp>
#include <hal/st/stm32f1/stm32f103xx/bitfields/rcc_bitfields.hpp>
#include <hal/st/stm32f1/stm32f103xx/bitfields/gpio_bitfields.hpp>
#include <hal/st/stm32f1/stm32f103xx/bitfields/usart_bitfields.hpp>
#include <hal/st/stm32f1/stm32f103xx/bitfield_utils.hpp>
```

### 2. Consistency

All projects use the same include, making code more portable:

```cpp
// Works for any MCU
#include <hal/{vendor}/{family}/{mcu}/register_map.hpp>
```

### 3. Complete Access

Single include provides access to:
- ✅ All peripheral registers
- ✅ All bit field definitions
- ✅ All enumerated values
- ✅ Pin alternate functions
- ✅ Utility templates

### 4. Compile Time

Modern compilers with precompiled headers can compile this efficiently:

```cmake
# CMakeLists.txt
target_precompile_headers(my_target PRIVATE
    <hal/st/stm32f1/stm32f103xx/register_map.hpp>
)
```

## Namespace Organization

The namespace hierarchy mirrors the file structure:

```
alloy::hal::st::stm32f1::stm32f103xx
    ├── rcc::          # RCC peripheral
    │   ├── RCC        # Register structure pointer
    │   ├── cr::       # CR register bit fields
    │   ├── cfgr::     # CFGR register bit fields
    │   └── ...
    ├── gpio::         # GPIO peripheral
    │   ├── GPIOA      # Port A registers
    │   ├── GPIOB      # Port B registers
    │   └── ...
    ├── usart::        # USART peripheral
    │   ├── USART1     # USART1 registers
    │   └── ...
    ├── enums::        # All enumerations
    └── pin_functions:: # Pin AF mappings
```

## Best Practices

### 1. Use register_map.hpp for Applications

```cpp
// ✅ Good - application code
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>
using namespace alloy::hal::st::stm32f1::stm32f103xx;
```

### 2. Use Specific Headers for Libraries

```cpp
// ✅ Good - library code (only needs RCC)
#include <hal/st/stm32f1/stm32f103xx/registers/rcc_registers.hpp>
#include <hal/st/stm32f1/stm32f103xx/bitfields/rcc_bitfields.hpp>
```

### 3. Create Project-Specific Aliases

```cpp
// project_config.hpp
#pragma once
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>

namespace mcu = alloy::hal::st::stm32f1::stm32f103xx;

// Now use "mcu::" throughout project
```

### 4. Use Precompiled Headers

```cmake
# Speed up compilation
target_precompile_headers(firmware PRIVATE
    <hal/st/stm32f1/stm32f103xx/register_map.hpp>
)
```

## Performance

**Zero Runtime Overhead** ✅

The complete register map compiles to the same assembly as manual register access:

```cpp
// This code:
#include <hal/st/stm32f1/stm32f103xx/register_map.hpp>
using namespace alloy::hal::st::stm32f1::stm32f103xx;
rcc::RCC->CR = rcc::cr::HSEON::set(rcc::RCC->CR);

// Generates identical assembly to:
*(volatile uint32_t*)0x40021000 |= (1 << 16);

// Both compile to:
// ldr r0, =0x40021000
// ldr r1, [r0]
// orr r1, r1, #0x10000
// str r1, [r0]
```

## Summary

The complete register map provides:

✅ **Single Include** - One header for everything
✅ **Type Safety** - Compile-time validation
✅ **Zero Overhead** - Same performance as manual access
✅ **Complete Access** - All peripherals available
✅ **Organized** - Clear namespace hierarchy
✅ **Portable** - Same pattern across all MCUs
✅ **Efficient** - Works with precompiled headers

Use `register_map.hpp` as your main include for complete type-safe register access!
