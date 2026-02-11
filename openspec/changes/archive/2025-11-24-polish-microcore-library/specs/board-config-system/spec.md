# Spec: Declarative Board Configuration System

## ADDED Requirements

### Requirement: Boards defined in YAML format
Board configuration MUST be declarative, not code-based.

#### Scenario: Define nucleo_f401re in YAML
```yaml
# boards/nucleo_f401re/board.yaml
board:
  name: nucleo_f401re
  description: "STM32 Nucleo-F401RE Development Board"
  vendor: STMicroelectronics

mcu:
  part_number: STM32F401RE
  family: stm32f4
  architecture: cortex-m4
  max_frequency: 84000000
  flash_size: 524288      # 512 KB
  ram_size: 98304         # 96 KB

clock:
  source: external_crystal
  crystal_frequency: 8000000  # 8 MHz HSE
  target_frequency: 84000000  # 84 MHz SYSCLK
  pll:
    m: 8
    n: 336
    p: 4
    q: 7

peripherals:
  user_led:
    type: gpio
    pin: PA5
    mode: output
    initial: low
    alias: led_green

  user_button:
    type: gpio
    pin: PC13
    mode: input
    pull: none
    alias: button_user

  debug_uart:
    type: uart
    instance: USART2
    tx_pin: PA2
    rx_pin: PA3
    baudrate: 115200
    alias: console

flash:
  tool: st-flash
  interface: stlink
  base_address: 0x08000000
```

**Expected**: Complete board definition in declarative format
**Rationale**: No C++ code required to add boards

#### Scenario: Validate board configuration
```bash
$ ./ucore validate-board nucleo_f401re
✓ YAML syntax valid
✓ MCU part number recognized
✓ Pin assignments valid for STM32F401RE
✓ Clock configuration achievable
✓ Flash tool available
Board configuration valid
```

**Expected**: Comprehensive validation before build
**Rationale**: Catch errors early

### Requirement: Generate board.hpp from YAML
C++ board definitions MUST be auto-generated from YAML.

#### Scenario: Generate board header
```bash
$ ./ucore generate-board nucleo_f401re

Generating: boards/nucleo_f401re/board.hpp
```

```cpp
// Generated from boards/nucleo_f401re/board.yaml
namespace ucore::board::nucleo_f401re {
    using led_green = platform::Gpio::Output<
        platform::Pin::PA5,
        platform::OutputMode::PushPull
    >;

    using button_user = platform::Gpio::Input<
        platform::Pin::PC13,
        platform::PullMode::None
    >;

    using console = platform::SimpleUartConfigTxRx<
        platform::Uart::Uart2,
        platform::BaudRate::_115200
    >;
}
```

**Expected**: Type-safe C++ API from YAML
**Rationale**: Compile-time validation of configuration

### Requirement: Board wizard creates valid configurations
Interactive wizard MUST simplifies board creation.

#### Scenario: Create new board interactively
```bash
$ ./ucore new-board

Board Wizard
============

Board name: my_custom_board
MCU part number: STM32F411CE
Flash size (KB): 512
RAM size (KB): 128

Clock Configuration
-------------------
Crystal frequency (Hz): 25000000
Target CPU frequency (Hz): 100000000

Pin Configuration
-----------------
LED pin (e.g., PA5): PB2
Button pin (optional): PC13
Debug UART instance: USART1
Debug UART TX pin: PA9

✓ Configuration saved to: boards/my_custom_board/board.yaml
✓ Generated: boards/my_custom_board/board.hpp

Next steps:
  1. Review: boards/my_custom_board/board.yaml
  2. Build: ./ucore build my_custom_board blink
  3. Flash: ./ucore flash my_custom_board blink
```

**Expected**: Working board in minutes, no C++ expertise required
**Rationale**: Lower barrier to entry

### Requirement: JSON schema validates YAML structure
Schema MUST prevent invalid configurations.

#### Scenario: Validate against schema
```bash
$ ./ucore validate-board my_custom_board --strict

Validating against schema: tools/schemas/board.schema.json

✓ Required fields present
✓ Field types correct
✓ Enum values valid
✓ Pin names valid for MCU
✗ Error: PLL configuration cannot achieve target frequency
  target: 100 MHz with 25 MHz crystal
  suggestion: Try target: 96 MHz or 108 MHz

Board configuration invalid
```

**Expected**: Detailed validation errors with suggestions
**Rationale**: Guide users to correct configuration

## MODIFIED Requirements

### Requirement: ucore CLI reads YAML for board listing
Extend ucore list boards MUST use YAML metadata.

#### Scenario: List all boards
```bash
$ ./ucore list boards

Available boards (from YAML):
  nucleo_f401re    STM32F401RE Cortex-M4  84 MHz   512KB Flash
  nucleo_f722ze    STM32F722ZE Cortex-M7 216 MHz   512KB Flash
  nucleo_g071rb    STM32G071RB Cortex-M0+ 64 MHz   128KB Flash
  nucleo_g0b1re    STM32G0B1RE Cortex-M0+ 64 MHz   512KB Flash
  same70_xplained  ATSAME70Q21 Cortex-M7 300 MHz  2048KB Flash
```

**Expected**: Rich metadata from YAML
**Rationale**: Better user experience

## REMOVED Requirements

None. This adds new capability while preserving existing code-based boards during migration.
