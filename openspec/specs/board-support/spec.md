# board-support Specification

## Purpose
TBD - created by archiving change add-multi-vendor-clock-boards. Update Purpose after archive.
## Requirements
### Requirement: Linker Script for Each Board

The system SHALL provide linker scripts for each supported board with correct memory layout.

**Rationale**: Each MCU has different flash/RAM sizes and locations.

#### Scenario: STM32F103C8 linker script
```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    .text : { *(.isr_vector) *(.text*) } > FLASH
    .data : { *(.data*) } > RAM AT> FLASH
    .bss  : { *(.bss*) *(COMMON) } > RAM
}
```
- **WHEN** linking for STM32F103C8
- **THEN** code SHALL be placed in flash at 0x08000000
- **AND** data SHALL be placed in RAM at 0x20000000
- **AND** linker SHALL error if exceeding memory limits

### Requirement: Startup Code for Each Board

The system SHALL provide startup code that initializes MCU before main().

**Rationale**: MCU needs proper initialization (vectors, .data, .bss, clocks).

#### Scenario: Startup sequence
```cpp
// startup.cpp
extern "C" void Reset_Handler() {
    // 1. Copy .data from flash to RAM
    extern uint32_t _sdata, _edata, _sidata;
    for (auto src = &_sidata, dst = &_sdata; dst < &_edata;) {
        *dst++ = *src++;
    }

    // 2. Zero .bss
    extern uint32_t _sbss, _ebss;
    for (auto dst = &_sbss; dst < &_ebss;) {
        *dst++ = 0;
    }

    // 3. Call C++ constructors
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();
    for (auto ctor = __init_array_start; ctor < __init_array_end; ctor++) {
        (*ctor)();
    }

    // 4. Call main
    extern int main();
    main();

    // 5. Hang if main returns
    while(1);
}
```
- **WHEN** MCU resets
- **THEN** startup code SHALL copy .data section
- **AND** SHALL zero .bss section
- **AND** SHALL call global constructors
- **AND** SHALL call main()

### Requirement: Board Definition Header

The system SHALL provide board.hpp with pin definitions and board-specific configuration.

**Rationale**: Applications should reference logical names (LED) not physical pins (PC13).

#### Scenario: Blue Pill board definition
```cpp
// boards/stm32f103c8/board.hpp
namespace alloy::board {
    constexpr auto LED_PIN = hal::st::stm32f1::GpioPin<PC13>;
    constexpr auto LED_PORT = hal::st::stm32f1::GpioPort::C;
    constexpr uint32_t SYSTEM_CLOCK_HZ = 72000000;
    constexpr uint32_t HSE_FREQUENCY_HZ = 8000000;
}
```
- **WHEN** writing portable application
- **THEN** use board::LED_PIN instead of hardcoded pin
- **AND** application works on any board

### Requirement: Blink LED Example

The system SHALL provide working blink LED example for each board.

**Rationale**: Validates clock, GPIO, and build system are working correctly.

#### Scenario: Portable blink example
```cpp
#include "board.hpp"
#include "hal/interface/clock.hpp"
#include "hal/interface/gpio.hpp"

int main() {
    // Configure system clock
    auto& clock = board::get_system_clock();
    clock.set_frequency(board::SYSTEM_CLOCK_HZ);

    // Configure LED pin
    board::LED_PIN led;
    led.set_mode(hal::PinMode::Output);

    // Blink forever
    while (true) {
        led.toggle();
        delay_ms(500);
    }
}
```
- **WHEN** running blink example
- **THEN** LED SHALL blink at 1Hz
- **AND** same code works on all boards
- **AND** validates clock is running correctly

### Requirement: CMake Board Selection

The system SHALL allow selecting board via CMake option.

**Rationale**: Simple build system, easy to switch between boards.

#### Scenario: Build for different boards
```bash
# Build for STM32F103
cmake -B build -DALLOY_BOARD=stm32f103c8
cmake --build build

# Build for ESP32
cmake -B build -DALLOY_BOARD=esp32_devkit
cmake --build build

# Build for Pico
cmake -B build -DALLOY_BOARD=pico
cmake --build build
```
- **WHEN** specifying ALLOY_BOARD
- **THEN** CMake SHALL select correct toolchain
- **AND** SHALL use correct linker script
- **AND** SHALL compile for correct architecture

