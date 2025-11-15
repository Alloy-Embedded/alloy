# Board Abstraction Layer Specification

## MODIFIED Requirements

### Requirement: Platform-Independent Board Code

Board implementation files SHALL NOT contain platform-specific `#ifdef` directives or direct register access.

**Rationale**: Enables true board portability and compile-time abstraction without preprocessor conditionals.

#### Scenario: Board code uses policy types

- **GIVEN** board initialization code
- **WHEN** board.cpp is implemented
- **THEN** code SHALL use type aliases from board_config.hpp
- **AND** code SHALL NOT contain `#ifdef STM32*` or `#ifdef SAME70`
- **AND** code SHALL NOT access hardware registers directly

```cpp
// boards/nucleo_f401re/board.cpp - CORRECT

#include "board.hpp"
#include "board_config.hpp"

namespace board {

void init() {
    // Generic code using policy types
    ClockPlatform::init();

    // Configure LED
    LedPin::configure(OutputMode::PushPull, Speed::Low);

    // Configure SysTick for 1ms tick
    BoardSysTick::init();
}

} // namespace board
```

```cpp
// INCORRECT - contains platform-specific #ifdef

void init() {
    #ifdef STM32F4
        // Enable GPIOA clock
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    #endif

    #ifdef SAME70
        // Enable PIOA clock
        PMC->PMC_PCER0 = (1 << ID_PIOA);
    #endif
}
```

---

### Requirement: board_config.hpp Type Aliases

Each board SHALL define platform-specific type aliases in `board_config.hpp`.

**Rationale**: Centralizes all platform-specific configuration in one header, making board porting clear and systematic.

#### Scenario: Clock platform type alias

- **GIVEN** board requires clock configuration
- **WHEN** board_config.hpp is created
- **THEN** `ClockPlatform` type SHALL be defined
- **AND** type SHALL specify clock frequency
- **AND** type SHALL satisfy `ClockPlatform` concept

```cpp
// boards/nucleo_f401re/board_config.hpp

#pragma once

#include "hal/vendors/st/stm32f4/clock_platform.hpp"
#include "hal/core/types.hpp"

namespace board {

// Clock configuration - 84 MHz system clock
using ClockPlatform = stm32f4::Clock<84'000'000>;

static_assert(ClockPlatform::system_clock_hz == 84'000'000,
              "Clock frequency mismatch");

} // namespace board
```

#### Scenario: GPIO platform type alias

- **GIVEN** board uses GPIO pins
- **WHEN** board_config.hpp is created
- **THEN** `GpioPlatform` type SHALL be defined
- **AND** pin aliases SHALL be defined using GpioPlatform

```cpp
// boards/nucleo_f401re/board_config.hpp

namespace board {

using GpioPlatform = stm32f4::GPIO;

// Pin definitions
using LedPin = GpioPin<GpioPlatform, GPIOA, 5>;      // PA5 - Green LED
using ButtonPin = GpioPin<GpioPlatform, GPIOC, 13>;  // PC13 - User button
using UartTxPin = GpioPin<GpioPlatform, GPIOA, 2>;   // PA2 - USART2 TX
using UartRxPin = GpioPin<GpioPlatform, GPIOA, 3>;   // PA3 - USART2 RX

} // namespace board
```

#### Scenario: SysTick timer type alias

- **GIVEN** board requires timing functions
- **WHEN** board_config.hpp is created
- **THEN** `BoardSysTick` type SHALL be defined
- **AND** type SHALL use ClockPlatform frequency

```cpp
namespace board {

using BoardSysTick = SysTick<ClockPlatform::system_clock_hz>;

static_assert(BoardSysTick::tick_period_ms == 1,
              "SysTick must provide 1ms tick for RTOS");

} // namespace board
```

#### Scenario: Complete board_config.hpp example

- **GIVEN** new board is being ported
- **WHEN** board_config.hpp is created
- **THEN** file SHALL contain all platform type aliases
- **AND** file SHALL contain pin definitions
- **AND** file SHALL include static_asserts for validation

```cpp
// boards/nucleo_f722ze/board_config.hpp - Complete example

#pragma once

#include "hal/vendors/st/stm32f7/clock_platform.hpp"
#include "hal/vendors/st/stm32f7/gpio.hpp"
#include "hal/vendors/st/stm32f7/uart.hpp"
#include "hal/systick.hpp"
#include "hal/core/types.hpp"

namespace board {

// ============================================================================
// Platform Configuration
// ============================================================================

// Clock: 216 MHz system clock
using ClockPlatform = stm32f7::Clock<216'000'000>;

// Peripheral platforms
using GpioPlatform = stm32f7::GPIO;
using UartPlatform = stm32f7::UART;
using I2cPlatform = stm32f7::I2C;
using SpiPlatform = stm32f7::SPI;

// Timing
using BoardSysTick = SysTick<ClockPlatform::system_clock_hz>;

// ============================================================================
// Pin Definitions
// ============================================================================

// LED pins
using Led1Pin = GpioPin<GpioPlatform, GPIOB, 0>;   // Green LED
using Led2Pin = GpioPin<GpioPlatform, GPIOB, 7>;   // Blue LED
using Led3Pin = GpioPin<GpioPlatform, GPIOB, 14>;  // Red LED

// Button
using ButtonPin = GpioPin<GpioPlatform, GPIOC, 13>;

// USART3 (ST-Link VCP)
using UartTxPin = GpioPin<GpioPlatform, GPIOD, 8>;
using UartRxPin = GpioPin<GpioPlatform, GPIOD, 9>;

// ============================================================================
// Validation
// ============================================================================

static_assert(ClockPlatform::system_clock_hz == 216'000'000);
static_assert(BoardSysTick::tick_period_ms == 1);

} // namespace board
```

---

### Requirement: Board Porting Checklist

Project SHALL provide board porting checklist documenting required steps.

**Rationale**: Makes adding new boards systematic and ensures nothing is missed.

#### Scenario: Porting checklist exists

- **GIVEN** developer wants to port new board
- **WHEN** documentation is consulted
- **THEN** PORTING_NEW_BOARD.md SHALL exist
- **AND** checklist SHALL enumerate all required files
- **AND** checklist SHALL provide board_config.hpp template

```markdown
# Board Porting Checklist

## Required Files

- [ ] `boards/<board_name>/board.hpp` - Board interface
- [ ] `boards/<board_name>/board.cpp` - Board initialization
- [ ] `boards/<board_name>/board_config.hpp` - Platform type aliases
- [ ] `boards/<board_name>/<mcu_part>.ld` - Linker script
- [ ] `boards/<board_name>/syscalls.cpp` - System calls (newlib)
- [ ] `boards/<board_name>/CMakeLists.txt` - Build configuration

## board_config.hpp Template

Copy and customize:

```cpp
#pragma once

#include "hal/vendors/<vendor>/<family>/clock_platform.hpp"
#include "hal/vendors/<vendor>/<family>/gpio.hpp"
// ... other includes

namespace board {

// Clock configuration
using ClockPlatform = <vendor>::<family>::Clock<<FREQ_HZ>>;

// Peripheral platforms
using GpioPlatform = <vendor>::<family>::GPIO;
// ... other platforms

// Pin definitions
using LedPin = GpioPin<GpioPlatform, <PORT>, <PIN>>;
// ... other pins

} // namespace board
```

## Validation

- [ ] Build succeeds with new board
- [ ] Blink example runs on hardware
- [ ] SysTick tick rate validated (1ms ±1%)
```
