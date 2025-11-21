# Alloy CLI Improvement Proposal

**Date**: 2024-11-17
**Status**: PROPOSAL
**Goal**: Transform Alloy CLI into a powerful, professional development tool

---

## Executive Summary

Transform the Alloy codegen CLI into a comprehensive **embedded development assistant** that rivals modm's usability while maintaining simplicity. The goal is to provide instant access to MCU information, peripheral discovery, board configuration, and code generation with zero learning curve.

**Key Improvements**:
1. **Enhanced discovery commands** (list MCUs, boards, peripherals, pins)
2. **Interactive project initialization** (guided setup wizard)
3. **Smart peripheral configuration** (auto-detect compatible pins)
4. **Build system integration** (keep CMake, add Meson option)
5. **Documentation integration** (datasheet links, examples, pinouts)

**Comparison**:
- **Current**: Basic code generator with limited discovery
- **Target**: Full-featured embedded development assistant (like modm, but simpler)
- **Benefit**: 10x faster development workflow, zero googling for pinouts/datasheets

---

## 1. Current CLI Analysis

### 1.1 Current Capabilities ✅

**Commands Available**:
```bash
alloy codegen generate         # Generate code
alloy codegen status           # Show status
alloy codegen vendors          # List vendors
alloy codegen clean            # Clean generated files
alloy codegen test-parser      # Test SVD parser
alloy codegen config           # Show configuration
```

**Strengths**:
- ✅ Unified entry point (`codegen.py`)
- ✅ Colored output and progress tracking
- ✅ Vendor information display
- ✅ Command aliases (`gen`, `g`, `st`)

### 1.2 Current Gaps ❌

**Missing Discovery Commands**:
- ❌ No `list mcus` command (can't browse available MCUs)
- ❌ No `list boards` command (don't know which boards are supported)
- ❌ No `list peripherals` command (can't see what's implemented)
- ❌ No `show pinout` command (no visual pin mapping)
- ❌ No `search` command (can't find MCU by feature/vendor/family)

**Missing Project Management**:
- ❌ No `init` command (manual project setup required)
- ❌ No `config board` command (manual CMake editing)
- ❌ No `add peripheral` command (manual coding required)
- ❌ No `validate` command (can't check project configuration)

**Missing Documentation Integration**:
- ❌ No `docs` command (can't access datasheets/references)
- ❌ No `examples` command (can't list/view examples)
- ❌ No `pinout` visual display (no ASCII art or web browser)

**Build System Limitations**:
- CMake is powerful but verbose for simple projects
- No quick "just build this" command
- Configuration scattered across multiple files

---

## 2. Proposed CLI Architecture

### 2.1 Command Structure

```
alloy
├── init                    # Project initialization (NEW)
│   ├── wizard              # Interactive setup
│   ├── template            # From template
│   └── minimal             # Minimal setup
│
├── list                    # Discovery commands (ENHANCED)
│   ├── mcus                # List all MCUs
│   ├── boards              # List supported boards
│   ├── vendors             # List vendors (exists)
│   ├── families            # List MCU families
│   ├── peripherals         # List implemented peripherals
│   └── examples            # List available examples
│
├── show                    # Detailed information (NEW)
│   ├── mcu <name>          # MCU details (specs, peripherals, datasheet)
│   ├── board <name>        # Board details (schematic, pinout, examples)
│   ├── peripheral <name>   # Peripheral details (API, examples)
│   ├── pinout <board>      # Visual pinout (ASCII art or browser)
│   └── docs <topic>        # Open documentation
│
├── search                  # Smart search (NEW)
│   ├── mcu <query>         # Search MCUs by feature/name
│   ├── board <query>       # Search boards
│   └── pin <function>      # Find pins for function (e.g., "UART TX")
│
├── config                  # Project configuration (ENHANCED)
│   ├── show                # Show current config (exists)
│   ├── board <name>        # Set target board
│   ├── mcu <name>          # Set target MCU
│   ├── peripheral add      # Add peripheral to project
│   └── validate            # Validate configuration
│
├── codegen                 # Code generation (EXISTS)
│   ├── generate            # Generate code
│   ├── status              # Show status
│   ├── clean               # Clean generated files
│   └── test-parser         # Test SVD parser
│
├── build                   # Build integration (NEW)
│   ├── configure           # Configure build system
│   ├── compile             # Compile project
│   ├── flash               # Flash to board
│   ├── clean               # Clean build artifacts
│   └── size                # Show binary size
│
└── docs                    # Documentation (NEW)
    ├── api                 # API documentation
    ├── datasheet <mcu>     # Open datasheet
    ├── reference <topic>   # Open reference manual
    └── examples            # Browse examples
```

### 2.2 Enhanced Commands Examples

#### **`alloy init` - Interactive Project Setup**

```bash
$ alloy init

╔══════════════════════════════════════════════════════════════════════════════╗
║                    🚀 Alloy Project Initialization                           ║
╚══════════════════════════════════════════════════════════════════════════════╝

Welcome to Alloy Embedded Framework!
This wizard will help you create a new embedded project.

📋 Step 1/5: Project Information
─────────────────────────────────
Project name: my-robot-controller
Author: Leonardo Gili
Description: Robot motor controller with UART communication

📟 Step 2/5: Target Board Selection
────────────────────────────────────
Available boards:
  1. nucleo_f401re      (STM32F401, Cortex-M4, 84MHz, 96KB RAM)
  2. nucleo_f722ze      (STM32F722, Cortex-M7, 216MHz, 256KB RAM)
  3. nucleo_g071rb      (STM32G071, Cortex-M0+, 64MHz, 36KB RAM)
  4. nucleo_g0b1re      (STM32G0B1, Cortex-M0+, 64MHz, 144KB RAM)
  5. same70_xplained    (SAME70Q21B, Cortex-M7, 300MHz, 384KB RAM)
  6. [Custom MCU]

Select board [1-6]: 1

✅ Selected: nucleo_f401re (STM32F401RET6)
   • MCU: STM32F401RET6
   • Core: ARM Cortex-M4F @ 84MHz
   • Flash: 512KB
   • RAM: 96KB
   • Package: LQFP64

🔌 Step 3/5: Peripherals Selection
───────────────────────────────────
Which peripherals do you need? (space to select, enter to continue)

  [x] GPIO      (Digital I/O)
  [x] UART      (Serial communication)
  [ ] SPI       (Serial Peripheral Interface)
  [ ] I2C       (Inter-Integrated Circuit)
  [ ] ADC       (Analog-to-Digital Converter)
  [ ] PWM       (Pulse Width Modulation)
  [x] SysTick   (System Timer)
  [ ] RTOS      (Real-Time Operating System)

⚙️  Step 4/5: UART Configuration
────────────────────────────────
UART1 is available on:
  Option 1: TX=PA9  (AF7), RX=PA10 (AF7)  [Recommended - Arduino D1/D0]
  Option 2: TX=PB6  (AF7), RX=PB7  (AF7)

Select UART pins [1-2]: 1

✅ UART1 configured:
   • TX: PA9 (Arduino D8)
   • RX: PA10 (Arduino D2)
   • Baud: 115200
   • Mode: 8N1

🏗️  Step 5/5: Build System
──────────────────────────
Choose build system:
  1. CMake       (Industry standard, mature)
  2. Meson       (Fast, modern, Python-based)

Select [1-2]: 1

════════════════════════════════════════════════════════════════════════════════

📁 Creating project structure...
   ✅ Created: my-robot-controller/
   ✅ Created: src/main.cpp
   ✅ Created: CMakeLists.txt
   ✅ Created: .vscode/settings.json
   ✅ Generated: board configuration
   ✅ Generated: peripheral drivers

🎉 Project created successfully!

Next steps:
  $ cd my-robot-controller
  $ alloy build configure
  $ alloy build compile
  $ alloy build flash

Or open in VSCode:
  $ code my-robot-controller
```

#### **`alloy list mcus` - Browse Available MCUs**

```bash
$ alloy list mcus --vendor st --family stm32f4

╔══════════════════════════════════════════════════════════════════════════════╗
║                           STM32F4 MCU Family                                 ║
╚══════════════════════════════════════════════════════════════════════════════╝

┌─────────────────┬──────────┬────────┬────────┬──────────┬────────────────────┐
│ MCU             │ Core     │ Freq   │ Flash  │ RAM      │ Package            │
├─────────────────┼──────────┼────────┼────────┼──────────┼────────────────────┤
│ STM32F401CB     │ M4F      │ 84MHz  │ 128KB  │ 64KB     │ LQFP48, UFQFPN48   │
│ STM32F401RB     │ M4F      │ 84MHz  │ 128KB  │ 64KB     │ LQFP64             │
│ STM32F401CC     │ M4F      │ 84MHz  │ 256KB  │ 64KB     │ LQFP48, UFQFPN48   │
│ STM32F401RC     │ M4F      │ 84MHz  │ 256KB  │ 64KB     │ LQFP64             │
│ STM32F401CD     │ M4F      │ 84MHz  │ 384KB  │ 96KB     │ LQFP48, UFQFPN48   │
│ STM32F401RD     │ M4F      │ 84MHz  │ 384KB  │ 96KB     │ LQFP64             │
│ STM32F401CE     │ M4F      │ 84MHz  │ 512KB  │ 96KB     │ LQFP48, UFQFPN48   │
│ STM32F401RE  ✓  │ M4F      │ 84MHz  │ 512KB  │ 96KB     │ LQFP64             │
│ ...                                                                           │
│ STM32F407VG  ✓  │ M4F      │ 168MHz │ 1024KB │ 192KB    │ LQFP100            │
│ STM32F429ZI  ✓  │ M4F      │ 180MHz │ 2048KB │ 256KB    │ LQFP144, BGA176    │
└─────────────────┴──────────┴────────┴────────┴──────────┴────────────────────┘

✓ = Supported boards available

Showing 12 of 87 MCUs. Use --all to see complete list.

Filter options:
  --min-flash 512K    # Minimum flash size
  --min-ram 128K      # Minimum RAM
  --package LQFP64    # Specific package
  --with-peripheral CAN  # Must have CAN peripheral

Examples:
  alloy list mcus --vendor st --min-flash 512K --min-ram 128K
  alloy show mcu STM32F401RE  # Detailed information
```

#### **`alloy show mcu` - MCU Details**

```bash
$ alloy show mcu STM32F401RE

╔══════════════════════════════════════════════════════════════════════════════╗
║                            STM32F401RET6                                     ║
║                      ARM Cortex-M4F Microcontroller                          ║
╚══════════════════════════════════════════════════════════════════════════════╝

📋 General Information
──────────────────────
  Manufacturer:  STMicroelectronics
  Family:        STM32F4 (High Performance)
  Core:          ARM Cortex-M4F with FPU
  Max Frequency: 84 MHz
  Package:       LQFP64
  Temperature:   -40°C to +85°C (Industrial)

💾 Memory
─────────
  Flash:         512 KB
  SRAM:          96 KB
  EEPROM:        -

🔌 Peripherals
──────────────
  GPIO:          50 I/O pins (5V tolerant)
  UART:          3x (USART1, USART2, USART6)
  SPI:           4x (up to 42 Mbit/s)
  I2C:           3x (up to 1 MHz Fast-mode Plus)
  I2S:           2x
  ADC:           1x 12-bit (16 channels, up to 2.4 MSPS)
  DAC:           -
  Timers:        11x (2x 32-bit, 9x 16-bit)
  PWM:           Up to 17 channels
  USB:           1x OTG Full-Speed
  CAN:           -
  DMA:           2 controllers, 16 streams

⚡ Power
───────
  Operating:     1.7V to 3.6V
  Low-power:     Stop, Standby, Sleep modes
  RTC:           Yes (with backup domain)

📚 Documentation
────────────────
  Datasheet:     https://st.com/resource/en/datasheet/stm32f401re.pdf
  Reference:     https://st.com/resource/en/reference_manual/dm00096844.pdf
  Errata:        https://st.com/resource/en/errata_sheet/dm00105230.pdf
  SVD File:      tools/codegen/svd/upstream/STMicro/STM32F401.svd

🎛️  Development Boards
──────────────────────
  ✅ nucleo_f401re    (Official ST Nucleo-64 board)

🔧 Quick Actions
────────────────
  alloy show board nucleo_f401re    # See board details
  alloy show pinout nucleo_f401re   # Visual pinout
  alloy init --board nucleo_f401re  # Create new project
  alloy docs datasheet STM32F401RE  # Open datasheet in browser
```

#### **`alloy show pinout` - Visual Pinout Display**

```bash
$ alloy show pinout nucleo_f401re

╔══════════════════════════════════════════════════════════════════════════════╗
║                   Nucleo-F401RE Pinout (Arduino Layout)                      ║
╚══════════════════════════════════════════════════════════════════════════════╝

             CN5 (Power)              CN6 (Analog)           CN8 (Morpho Left)
         ┌───────────────┐       ┌────────────────┐       ┌──────────────────┐
         │               │       │                │       │                  │
   NC────┤ 1         2   ├───NC  │ 1          2   ├───NC  │ 1            2   ├──GND
  IOREF─┤ 3         4   ├──+5V  │ 3          4   ├──+5V  │ 3  PC10      4   ├──PC11
 RESET─┤ 5         6   ├──GND   │ 5  A0/PA0  6   ├──GND  │ 5  PC12      6   ├──PD2
  +3V3─┤ 7         8   ├──GND   │ 7  A1/PA1  8   ├───NC  │ 7  VDD       8   ├──E5V
   +5V─┤ 9        10   ├──GND   │ 9  A2/PA4  10  ├───NC  │ 9  BOOT0    10   ├──GND
   GND─┤ 11       12   ├───NC   │ 11 A3/PB0  12  ├───NC  │ 11 GND      12   ├──IOREF
   GND─┤ 13       14   ├───NC   │ 13 A4/PC1  14  ├───NC  │ 13 PA13     14   ├──RESET
   VIN─┤ 15       16   ├───NC   │ 15 A5/PC0  16  ├───NC  │ 15 PA14     16   ├──+3V3
         └───────────────┘       └────────────────┘       │ 17 PA15     18   ├──+5V
                                                           │ 19 GND      20   ├──GND
       CN9 (Morpho Right)            CN7 (Digital)        │ 21 PB7      22   ├──GND
     ┌──────────────────┐       ┌────────────────┐       │ 23 PC13     24   ├──VIN
     │                  │       │                │       └──────────────────┘
 PC9─┤ 1            2   ├──PC8  │ 1  D0/PA3  2   ├──D1/PA2
 PB8─┤ 3            4   ├──PC6  │ 3  D2/PA10 4   ├──D3/PB3    💡 LED: PA5
 PB9─┤ 5            6   ├──PC5  │ 5  D4/PB5  6   ├──D5/PB4    🔘 BTN: PC13
AVDD─┤ 7            8   ├──U5V  │ 7  D6/PB10 8   ├──D7/PA8
 GND─┤ 9           10   ├──NC   │ 9  D8/PA9  10  ├──D9/PC7    🔌 UART1:
 PA5─┤ 11 (LED)   12   ├──PA12  │ 11 D10/PB6 12  ├──D11/PA7      TX: PA2
 PA6─┤ 13          14   ├──PA11  │ 13 D12/PA6 14  ├──D13/PA5      RX: PA3
 PA7─┤ 15          16   ├──PB12  │ 15 D14/PB9 16  ├──D15/PB8
 PB6─┤ 17          18   ├──NC    │ 17 GND     18  ├──AREF     🔌 SPI1:
PC7─┤ 19          20   ├──GND   │ 19 SDA/PB9 20  ├──SCL/PB8     SCK:  PA5
 PA9─┤ 21 UART_TX 22   ├──PB2    └────────────────┘               MISO: PA6
 PA8─┤ 23          24   ├──PB1                                    MOSI: PA7
PB10─┤ 25          26   ├──PB15                                   CS:   PB6
 PB4─┤ 27          28   ├──PB14
 PB5─┤ 29          30   ├──PB13  📘 Peripheral Mapping
 PB3─┤ 31          32   ├──AGND  ─────────────────────
PA10─┤ 33 UART_RX 34   ├──PC4   • UART1: PA9(TX), PA10(RX)
 PA2─┤ 35 UART_TX 36   ├──NC    • UART2: PA2(TX), PA3(RX)
 PA3─┤ 37 UART_RX 38   ├──NC    • I2C1:  PB8(SCL), PB9(SDA)
     └──────────────────┘        • SPI1:  PA5/6/7, CS=PB6
                                 • USB:   PA11(DM), PA12(DP)

⌨️  Interactive mode: Use arrow keys to highlight pins
    Press 'i' for detailed pin info | 'f' to find function | 'q' to quit
```

#### **`alloy search pin` - Find Pins for Function**

```bash
$ alloy search pin "UART TX" --board nucleo_f401re

╔══════════════════════════════════════════════════════════════════════════════╗
║                    UART TX Pins on nucleo_f401re                             ║
╚══════════════════════════════════════════════════════════════════════════════╝

Found 3 UART peripherals with TX capability:

┌──────────┬─────────┬────────┬──────────┬────────────────────────────────────┐
│ Instance │ Pin     │ AF     │ Arduino  │ Notes                              │
├──────────┼─────────┼────────┼──────────┼────────────────────────────────────┤
│ UART1    │ PA9     │ AF7    │ D8       │ ✅ Recommended - Arduino compatible│
│          │ PB6     │ AF7    │ D10      │                                    │
│          │                                                                   │
│ UART2    │ PA2  ✓  │ AF7    │ D1, A7   │ ⚠️  Connected to ST-LINK (VCP)    │
│          │ PD5     │ AF7    │ -        │ ⚠️  Not available on LQFP64       │
│          │                                                                   │
│ UART6    │ PC6     │ AF8    │ -        │                                    │
│          │ PA11    │ AF8    │ -        │ ⚠️  Shared with USB_DM            │
└──────────┴─────────┴────────┴──────────┴────────────────────────────────────┘

✓ = Default/recommended pin
⚠️  = Conflict or limitation

💡 Recommendation:
   Use UART1 on PA9 (TX) + PA10 (RX) for general-purpose communication.
   UART2 is connected to ST-LINK Virtual COM Port (debugging).

Example code:
  auto uart = Uart::simple<UartInstance::Uart1, PA9, PA10>(115200);
  uart.send("Hello, World!\n");

See also:
  alloy show mcu STM32F401RE --peripherals
  alloy docs api uart
```

#### **`alloy config peripheral add` - Add Peripheral**

```bash
$ alloy config peripheral add

╔══════════════════════════════════════════════════════════════════════════════╗
║                        Add Peripheral to Project                             ║
╚══════════════════════════════════════════════════════════════════════════════╝

📋 Project: my-robot-controller
🎯 Board:   nucleo_f401re (STM32F401RET6)

Select peripheral:
  1. GPIO      Digital I/O pins
  2. UART      Serial communication
  3. SPI       Serial Peripheral Interface
  4. I2C       Inter-Integrated Circuit
  5. ADC       Analog-to-Digital Converter
  6. PWM       Pulse Width Modulation
  7. Timer     General-purpose timers
  8. USB       USB OTG Full-Speed
  9. DMA       Direct Memory Access
  10. RTOS     Real-Time Operating System

Enter number [1-10]: 2

─────────────────────────────────────────────────────────────────────────────

🔌 UART Configuration

Select UART instance:
  1. UART1  (Available pins: PA9/PA10, PB6/PB7)
  2. UART2  (Available pins: PA2/PA3) ⚠️  ST-LINK VCP
  3. UART6  (Available pins: PC6/PC7, PA11/PA12)

Instance [1-3]: 1

Select pin configuration:
  1. PA9 (TX), PA10 (RX)  ✅ Recommended
  2. PB6 (TX), PB7 (RX)

Pins [1-2]: 1

Baud rate [115200]:
Parity [N]one, [E]ven, [O]dd [N]:
Data bits [8]:
Stop bits [1]:

─────────────────────────────────────────────────────────────────────────────

✅ UART1 configured:
   • TX: PA9 (AF7)
   • RX: PA10 (AF7)
   • Baud: 115200
   • Parity: None
   • Data: 8 bits
   • Stop: 1 bit

📝 Updating project files...
   ✅ Updated: src/peripherals.hpp
   ✅ Updated: src/peripherals.cpp
   ✅ Updated: CMakeLists.txt

Example code added to src/main.cpp:

  #include "peripherals.hpp"

  int main() {
      // Initialize UART1
      peripherals::uart1.init();

      // Send data
      peripherals::uart1.send("Hello, World!\n");

      // Receive data
      while (true) {
          if (auto byte = peripherals::uart1.receive()) {
              peripherals::uart1.send(*byte);  // Echo
          }
      }
  }

Next steps:
  $ alloy build compile
  $ alloy build flash
```

#### **`alloy build` - Build Integration**

```bash
$ alloy build configure

╔══════════════════════════════════════════════════════════════════════════════╗
║                         Configure Build System                               ║
╚══════════════════════════════════════════════════════════════════════════════╝

📋 Project: my-robot-controller
🎯 Board:   nucleo_f401re
🔧 Toolchain: arm-none-eabi-gcc 13.2.0 ✅

Configuring CMake...
  ✅ Generated: build/CMakeCache.txt
  ✅ Generated: build/compile_commands.json (for IDE)
  ✅ Generated: build/generated/board_config.hpp
  ✅ Generated: build/generated/peripherals.hpp

Build configuration:
  • Compiler:   arm-none-eabi-gcc 13.2.0
  • Linker:     STM32F401RE.ld (512KB Flash, 96KB RAM)
  • Optimizer:  -Os (size)
  • Standard:   C++23
  • Float ABI:  hard (FPU enabled)

Ready to build!

$ alloy build compile

🔨 Compiling my-robot-controller...

  [1/12] Building src/main.cpp
  [2/12] Building src/peripherals.cpp
  [3/12] Building board/nucleo_f401re/board.cpp
  [4/12] Generating peripheral drivers...
  [5/12] Building startup code...
  [12/12] Linking my-robot-controller.elf

✅ Build successful!

📊 Binary Size:
  ┌──────────┬──────────┬──────────┐
  │ Section  │ Size     │ Usage    │
  ├──────────┼──────────┼──────────┤
  │ .text    │ 12.4 KB  │ ████ 2%  │
  │ .data    │ 0.5 KB   │ ▏ 0%     │
  │ .bss     │ 2.1 KB   │ ▏ 2%     │
  │ .rodata  │ 1.8 KB   │ ▏ 0%     │
  └──────────┴──────────┴──────────┘
  Total Flash: 14.7 KB / 512 KB (2%)
  Total RAM:   2.6 KB / 96 KB (2%)

Output files:
  • build/my-robot-controller.elf
  • build/my-robot-controller.hex
  • build/my-robot-controller.bin

$ alloy build flash

📡 Flashing to nucleo_f401re...

Detected programmer: ST-LINK V2-1
Target: STM32F401RETx

Erasing flash...     ████████████████████ 100%
Writing firmware...  ████████████████████ 100% (14.7 KB)
Verifying...         ████████████████████ 100%

✅ Flash successful!

Starting program... Done.

🔌 Serial monitor available at /dev/ttyACM0 (115200 baud)
   Run: alloy monitor
```

---

## 3. Build System Evaluation: CMake vs Meson

### 3.1 Keep CMake as Default ✅

**Recommendation**: **Keep CMake as primary build system**

**Reasons**:
1. ✅ **Industry standard** for embedded (vendor support, examples)
2. ✅ **Mature ecosystem** (toolchain files, find modules)
3. ✅ **IDE integration** (VSCode, CLion, Eclipse all support CMake)
4. ✅ **Current investment** (working CMake setup)
5. ✅ **Documentation** (extensive embedded CMake resources)

**CMake Advantages for Embedded**:
- Vendor toolchain files widely available (ST, NXP, etc.)
- CMake presets (JSON configuration)
- Better linker script integration
- Proven at scale (FreeRTOS, Zephyr, ESP-IDF use CMake)

### 3.2 Add Meson as Optional Alternative 🎯

**Recommendation**: **Add Meson support as experimental option**

**Why Meson?**:
1. ✅ **Simpler syntax** (Python-based, more readable)
2. ✅ **Faster builds** (Ninja backend, better incremental builds)
3. ✅ **Better defaults** (cross-compilation easier)
4. ✅ **Native testing** (built-in test framework)

**Meson Advantages**:
- Configuration in Python (familiar for our codegen users)
- Faster configure step (no multiple passes like CMake)
- Cleaner cross-compilation (no complex toolchain files)
- Better dependency management

**Example: Meson vs CMake Configuration**

**CMake** (verbose):
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++23 -fno-exceptions -fno-rtti")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/linker.ld -Wl,--gc-sections")

project(my-project C CXX ASM)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/peripherals.cpp
    board/nucleo_f401re/board.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(${PROJECT_NAME}
    alloy::core
    alloy::hal
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
)
```

**Meson** (simpler):
```python
# meson.build
project('my-project', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++23',
    'b_staticpic=false',
    'warning_level=3'
  ]
)

# Alloy framework
alloy = subproject('alloy')
alloy_dep = alloy.get_variable('alloy_dep')

# Project sources
sources = files(
  'src/main.cpp',
  'src/peripherals.cpp',
  'board/nucleo_f401re/board.cpp'
)

# Executable
elf = executable('my-project',
  sources,
  dependencies: alloy_dep,
  include_directories: [
    include_directories('src'),
    include_directories('include')
  ],
  link_args: [
    '-T' + meson.source_root() / 'linker.ld',
    '-Wl,--gc-sections'
  ]
)

# Generate .hex file
hex = custom_target('hex',
  input: elf,
  output: 'my-project.hex',
  command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'],
  build_by_default: true
)

# Size report
run_target('size',
  command: [size, elf]
)
```

**Cross-compilation** (much simpler in Meson):

**CMake toolchain file** (40+ lines):
```cmake
# arm-none-eabi.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(CMAKE_C_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++ REQUIRED)
find_program(CMAKE_ASM_COMPILER arm-none-eabi-gcc REQUIRED)
find_program(CMAKE_AR arm-none-eabi-ar REQUIRED)
find_program(CMAKE_OBJCOPY arm-none-eabi-objcopy REQUIRED)
find_program(CMAKE_SIZE arm-none-eabi-size REQUIRED)
# ... 30 more lines
```

**Meson cross file** (10 lines):
```ini
# arm-none-eabi.ini
[binaries]
c = 'arm-none-eabi-gcc'
cpp = 'arm-none-eabi-g++'
ar = 'arm-none-eabi-ar'
strip = 'arm-none-eabi-strip'
objcopy = 'arm-none-eabi-objcopy'
size = 'arm-none-eabi-size'

[host_machine]
system = 'none'
cpu_family = 'arm'
cpu = 'cortex-m4'
endian = 'little'
```

### 3.3 Hybrid Approach: Best of Both Worlds ⭐

**Recommendation**: Support both, let users choose

**Implementation**:
```bash
# Initialize with CMake (default)
$ alloy init --build-system cmake

# Initialize with Meson (experimental)
$ alloy init --build-system meson

# CLI abstracts build system
$ alloy build configure   # Calls cmake or meson configure
$ alloy build compile     # Calls ninja or make
$ alloy build flash       # Same command regardless
```

**Benefits**:
- Users comfortable with CMake can continue
- New users can try Meson for faster builds
- CLI provides unified interface (hides complexity)
- We can migrate gradually

**Effort**:
- CMake: Already working ✅
- Meson: ~40 hours to add full support

---

## 4. Enhanced CLI Features

### 4.1 Smart Peripheral Pin Recommendation

**Problem**: Users don't know which pins are available/optimal

**Solution**: AI-powered pin recommendations

```bash
$ alloy config peripheral add spi

🔌 SPI Configuration for nucleo_f401re

Analyzing available pins...

✅ Recommended configuration (SPI1):
   • SCK:  PA5  (Arduino D13) - LED conflict ⚠️
   • MISO: PA6  (Arduino D12)
   • MOSI: PA7  (Arduino D11)
   • CS:   PB6  (Arduino D10)

   Alternatives:
   1. Use SPI1 with different SCK (PA5 → PB3) to avoid LED
   2. Use SPI2 (PB13/PB14/PB15) - no Arduino headers
   3. Use SPI3 (PC10/PC11/PC12) - not available on LQFP64

Select configuration [1-3] or [custom]: 1

✅ Using SPI1 with PB3 (SCK) to avoid LED conflict
```

### 4.2 Interactive Pinout Explorer

**Visual ASCII art pinout** (like `alloy show pinout` above) with:
- Arrow key navigation
- Pin highlighting
- Function search (`/UART` to find all UART pins)
- Conflict detection (red for conflicts, green for available)
- Export to SVG/PNG

### 4.3 Documentation Integration

```bash
$ alloy docs datasheet STM32F401RE
Opening datasheet in browser... ✅
https://st.com/resource/en/datasheet/stm32f401re.pdf

$ alloy docs api uart
Opening UART API documentation... ✅
file:///path/to/docs/uart-api.html

$ alloy docs examples uart-echo
Opening example: UART Echo... ✅

  // examples/uart_echo/main.cpp
  #include "board.hpp"

  int main() {
      board::init();
      auto uart = Uart::simple<1>(115200);

      while (true) {
          if (auto byte = uart.receive()) {
              uart.send(*byte);  // Echo
          }
      }
  }

📋 Available examples:
   1. blink             (GPIO output)
   2. uart_echo         (UART loopback)
   3. rtos_tasks        (RTOS multitasking)
   4. systick_demo      (Timing with SysTick)

Use: alloy docs examples <name> to view
```

### 4.4 Project Templates

```bash
$ alloy init --template

Available templates:
  1. blinky            Simple LED blink (GPIO)
  2. uart_logger       Serial debug logging (UART)
  3. rtos_multi_task   RTOS with multiple tasks
  4. sensor_reader     ADC + I2C sensor reading
  5. motor_control     PWM motor controller
  6. usb_cdc           USB virtual COM port
  7. bare_metal        Minimal startup (advanced)

Select template [1-7]: 3

Creating project from 'rtos_multi_task'...

This template includes:
  ✅ RTOS scheduler (4 tasks pre-configured)
  ✅ Task synchronization (mutex, semaphore)
  ✅ Message queues
  ✅ LED blinking task (example)
  ✅ UART logging task (example)

Project created: my-robot-controller/
```

### 4.5 Validation and Troubleshooting

```bash
$ alloy config validate

╔══════════════════════════════════════════════════════════════════════════════╗
║                    Project Configuration Validation                          ║
╚══════════════════════════════════════════════════════════════════════════════╝

Checking project: my-robot-controller

✅ Board configuration valid
   • Board: nucleo_f401re
   • MCU: STM32F401RET6
   • Toolchain: arm-none-eabi-gcc 13.2.0 found

✅ Peripherals configured correctly
   • UART1: PA9 (TX), PA10 (RX) ✓
   • SPI1:  PB3 (SCK), PA6 (MISO), PA7 (MOSI), PB6 (CS) ✓

⚠️  Warnings:
   • Pin PA5 (LED) is also used by SPI1 SCK (in default config)
     → Resolved: Using PB3 instead ✓

⚠️  Potential issues:
   • UART2 default pins (PA2/PA3) conflict with ST-LINK VCP
     → Use UART1 for application communication

✅ Build configuration valid
   • CMakeLists.txt: OK
   • Linker script: STM32F401RE.ld found
   • Generated files: Up to date

✅ Memory usage within limits
   • Flash: 14.7 KB / 512 KB (2%) ✓
   • RAM:   2.6 KB / 96 KB (2%) ✓

All checks passed! Project is ready to build.

$ alloy troubleshoot

Common issues and solutions:
  1. "arm-none-eabi-gcc not found"
     → Install ARM GCC: brew install arm-none-eabi-gcc

  2. "Flash too large for device"
     → Enable optimizations: set(CMAKE_BUILD_TYPE Release)
     → Check binary size: alloy build size --verbose

  3. "UART not working"
     → Verify pin configuration: alloy show pinout
     → Check baud rate matches: 115200 is common
```

---

## 5. Implementation Plan

### Phase 1: Enhanced Discovery (2 weeks)

**Commands to implement**:
- `alloy list mcus` - Browse MCU database
- `alloy list boards` - Show supported boards
- `alloy list peripherals` - Show implemented peripherals
- `alloy show mcu <name>` - MCU details with datasheets
- `alloy show board <name>` - Board pinout and specs
- `alloy search pin <function>` - Find pins for peripheral

**Work items**:
1. Create MCU database (JSON) from existing SVD files
2. Parse board.hpp files to extract board metadata
3. Implement rich terminal output (colors, tables, boxes)
4. Add datasheet URL database

**Effort**: 40 hours

### Phase 2: Project Initialization (1 week)

**Commands to implement**:
- `alloy init` - Interactive project wizard
- `alloy init --template <name>` - From template
- `alloy config peripheral add` - Add peripheral interactively

**Work items**:
1. Create project templates (blinky, uart, rtos, etc.)
2. Implement interactive wizard with prompts
3. Auto-generate CMakeLists.txt from config
4. Generate initial source files

**Effort**: 24 hours

### Phase 3: Build Integration (1 week)

**Commands to implement**:
- `alloy build configure` - Configure build system
- `alloy build compile` - Compile project
- `alloy build flash` - Flash to board
- `alloy build size` - Show binary size analysis
- `alloy build clean` - Clean build artifacts

**Work items**:
1. Wrap CMake commands with progress tracking
2. Integrate OpenOCD/STLink for flashing
3. Add binary size visualization
4. Error parsing and helpful messages

**Effort**: 24 hours

### Phase 4: Documentation & Validation (1 week)

**Commands to implement**:
- `alloy docs datasheet <mcu>` - Open datasheet in browser
- `alloy docs api <peripheral>` - Open API docs
- `alloy docs examples` - Browse examples
- `alloy config validate` - Validate project config
- `alloy troubleshoot` - Common issues help

**Work items**:
1. Create documentation index (JSON)
2. Implement browser integration
3. Add configuration validation logic
4. Create troubleshooting database

**Effort**: 24 hours

### Phase 5: Advanced Features (2 weeks)

**Commands to implement**:
- `alloy show pinout <board>` - ASCII art pinout
- Interactive pinout explorer
- Smart pin recommendations
- Conflict detection

**Work items**:
1. Create pinout ASCII art generator
2. Implement terminal UI (arrow keys, highlighting)
3. Add pin conflict detection algorithm
4. Implement recommendation engine

**Effort**: 40 hours

### Phase 6: Meson Support (Optional - 1 week)

**Work items**:
1. Create meson.build templates
2. Add cross-compilation files
3. Update CLI to support both CMake and Meson
4. Document Meson workflow

**Effort**: 24 hours (optional)

**Total Effort**: 176 hours (4-5 weeks full-time)

---

## 6. Technology Stack

### 6.1 CLI Framework

**Current**: argparse (Python stdlib)
**Recommended**: **Rich** + **Typer**

**Why Rich?**
- Beautiful terminal output (colors, tables, progress bars)
- Markdown rendering
- Syntax highlighting
- Tree rendering (for file structures)

**Why Typer?**
- Modern CLI framework (built on Click)
- Auto-generated help
- Type hints for validation
- Subcommand groups

**Example with Rich + Typer**:
```python
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
import typer

app = typer.Typer()
console = Console()

@app.command()
def list_mcus(
    vendor: str = typer.Option(None, "--vendor", "-v", help="Filter by vendor"),
    min_flash: str = typer.Option(None, "--min-flash", help="Minimum flash size")
):
    """List available MCUs with filtering"""

    # Create table
    table = Table(title="STM32F4 MCU Family")
    table.add_column("MCU", style="cyan")
    table.add_column("Core", style="magenta")
    table.add_column("Flash", style="green")
    table.add_column("RAM", style="yellow")

    # Add rows
    table.add_row("STM32F401RE", "M4F", "512KB", "96KB")
    table.add_row("STM32F407VG", "M4F", "1024KB", "192KB")

    # Print
    console.print(table)

    # Panel for recommendations
    console.print(Panel.fit(
        "[green]✅ Recommended:[/green] STM32F401RE for beginners",
        title="Recommendation"
    ))
```

### 6.2 Terminal UI

**For interactive features** (`alloy init`, pinout explorer):

**Option 1: Rich Prompts** (simple)
```python
from rich.prompt import Prompt, Confirm

board = Prompt.ask(
    "Select board",
    choices=["nucleo_f401re", "nucleo_f722ze", "same70_xplained"]
)

if Confirm.ask("Add UART peripheral?"):
    uart_instance = Prompt.ask("UART instance", default="1")
```

**Option 2: InquirerPy** (advanced)
```python
from InquirerPy import inquirer

board = inquirer.select(
    message="Select target board:",
    choices=[
        {"name": "Nucleo F401RE (STM32F401, 84MHz, 96KB RAM)", "value": "nucleo_f401re"},
        {"name": "Nucleo F722ZE (STM32F722, 216MHz, 256KB RAM)", "value": "nucleo_f722ze"},
        {"name": "SAME70 Xplained (SAME70Q21B, 300MHz, 384KB RAM)", "value": "same70_xplained"}
    ]
).execute()

peripherals = inquirer.checkbox(
    message="Select peripherals:",
    choices=["GPIO", "UART", "SPI", "I2C", "ADC", "PWM", "RTOS"]
).execute()
```

### 6.3 Data Storage

**MCU/Board Database**: JSON files (human-readable, easy to edit)

**Structure**:
```
tools/codegen/database/
├── mcus/
│   ├── stm32f4.json        # STM32F4 family MCUs
│   ├── same70.json         # SAME70 family MCUs
│   └── ...
├── boards/
│   ├── nucleo_f401re.json  # Board configuration
│   ├── same70_xplained.json
│   └── ...
├── peripherals/
│   ├── uart.json           # UART implementations
│   ├── spi.json
│   └── ...
└── datasheets/
    └── urls.json           # Datasheet/reference manual URLs
```

**Example MCU database** (`mcus/stm32f4.json`):
```json
{
  "family": "stm32f4",
  "vendor": "st",
  "display_name": "STM32F4 Series",
  "description": "High-performance Cortex-M4 MCUs",
  "mcus": [
    {
      "part_number": "STM32F401RET6",
      "core": "Cortex-M4F",
      "max_freq_mhz": 84,
      "flash_kb": 512,
      "ram_kb": 96,
      "package": "LQFP64",
      "peripherals": {
        "uart": 3,
        "spi": 4,
        "i2c": 3,
        "adc": 1,
        "timers": 11,
        "usb": 1
      },
      "features": ["FPU", "USB_OTG"],
      "datasheet_url": "https://st.com/resource/en/datasheet/stm32f401re.pdf",
      "reference_url": "https://st.com/resource/en/reference_manual/dm00096844.pdf",
      "boards": ["nucleo_f401re"]
    }
  ]
}
```

---

## 7. CLI Command Reference (Complete)

### Discovery Commands
```bash
alloy list mcus                           # List all MCUs
alloy list mcus --vendor st               # Filter by vendor
alloy list mcus --family stm32f4          # Filter by family
alloy list mcus --min-flash 512K          # Minimum flash size
alloy list mcus --with-peripheral USB     # Must have USB

alloy list boards                         # List supported boards
alloy list boards --vendor st             # ST boards only

alloy list vendors                        # List MCU vendors
alloy list families --vendor st           # ST families

alloy list peripherals                    # Implemented peripherals
alloy list peripherals --board nucleo_f401re  # For specific board

alloy list examples                       # Available examples
alloy list examples --tag uart            # UART examples
```

### Show Commands
```bash
alloy show mcu STM32F401RE                # MCU detailed info
alloy show mcu STM32F401RE --specs        # Technical specs only
alloy show mcu STM32F401RE --peripherals  # Peripherals list

alloy show board nucleo_f401re            # Board info
alloy show board nucleo_f401re --pinout   # Pinout diagram

alloy show pinout nucleo_f401re           # Interactive pinout
alloy show pinout nucleo_f401re --export pinout.svg  # Export

alloy show peripheral uart                # UART API reference
alloy show peripheral spi --examples      # SPI examples
```

### Search Commands
```bash
alloy search mcu "USB + 512KB flash"      # Find MCUs by features
alloy search mcu "cortex-m4"              # By core

alloy search pin "UART TX"                # Find UART TX pins
alloy search pin "UART TX" --board nucleo_f401re
alloy search pin "I2C SDA"                # I2C pins

alloy search board "stm32f4"              # Find STM32F4 boards
alloy search board "nucleo"               # All Nucleo boards
```

### Project Initialization
```bash
alloy init                                # Interactive wizard
alloy init --board nucleo_f401re          # Quick init
alloy init --template blinky              # From template
alloy init --template rtos                # RTOS template
alloy init --build-system meson           # Use Meson instead of CMake
```

### Configuration
```bash
alloy config show                         # Show current config
alloy config board nucleo_f401re          # Set target board
alloy config peripheral add               # Add peripheral (interactive)
alloy config peripheral add uart          # Add UART (interactive)
alloy config peripheral list              # List configured peripherals
alloy config peripheral remove uart1      # Remove peripheral
alloy config validate                     # Validate configuration
```

### Code Generation
```bash
alloy codegen generate                    # Generate all code
alloy codegen generate --pins             # Only pin definitions
alloy codegen generate --startup          # Only startup code
alloy codegen generate --vendor st        # Only ST MCUs
alloy codegen status                      # Show generation status
alloy codegen clean                       # Clean generated files
```

### Build Commands
```bash
alloy build configure                     # Configure build system
alloy build compile                       # Compile project
alloy build compile --verbose             # Verbose output
alloy build flash                         # Flash to board
alloy build flash --verify                # Flash with verification
alloy build size                          # Show binary size
alloy build size --detailed               # Detailed memory map
alloy build clean                         # Clean build artifacts
```

### Documentation
```bash
alloy docs api uart                       # Open UART API docs
alloy docs datasheet STM32F401RE          # Open datasheet (browser)
alloy docs reference stm32f4              # Reference manual
alloy docs examples                       # Browse examples
alloy docs examples uart-echo             # Open example code
```

### Utilities
```bash
alloy validate                            # Validate project
alloy troubleshoot                        # Common issues help
alloy version                             # Show version
alloy update                              # Update Alloy framework
```

---

## 8. User Experience Comparison

### Current Workflow (Manual)

**Create new project** (10+ steps, 30 minutes):
```bash
# 1. Create directories manually
mkdir my-project
cd my-project
mkdir src include boards cmake

# 2. Copy board files manually
cp -r ~/alloy/boards/nucleo_f401re boards/

# 3. Create CMakeLists.txt manually (50+ lines)
vim CMakeLists.txt  # Copy from example, modify...

# 4. Create main.cpp manually
vim src/main.cpp

# 5. Configure build manually
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
      -DALLOY_BOARD=nucleo_f401re \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# 6. Generate code manually
cd ../tools/codegen
python3 codegen.py generate --vendor st

# 7. Build manually
cd ../../build
make -j8

# 8. Flash manually
openocd -f interface/stlink.cfg \
        -f target/stm32f4x.cfg \
        -c "program my-project.elf verify reset exit"

# 9. Open serial monitor manually
screen /dev/ttyACM0 115200
```

**Time**: 30+ minutes (if you know what you're doing)
**Errors**: High (typos in paths, wrong config, missing flags)
**Learning curve**: Steep (need to understand CMake, OpenOCD, linker scripts)

### Proposed Workflow (CLI-Driven)

**Create new project** (1 step, 2 minutes):
```bash
$ alloy init

# Interactive wizard (as shown earlier)
# Answer 5 questions, done.

$ cd my-robot-controller
$ alloy build compile
$ alloy build flash

# Done!
```

**Time**: 2 minutes
**Errors**: Minimal (validation at each step)
**Learning curve**: None (guided wizard)

### Comparison Table

| Task | Current | Proposed | Time Saved |
|------|---------|----------|------------|
| **Create project** | 30 min | 2 min | 28 min (93%) |
| **Add UART** | 15 min (manual coding) | 1 min (interactive) | 14 min (93%) |
| **Find UART pins** | 5 min (datasheet) | 10 sec (search command) | 4.8 min (96%) |
| **Check MCU specs** | 2 min (Google) | 5 sec (show command) | 1.9 min (95%) |
| **Flash firmware** | 2 min (OpenOCD) | 10 sec (build flash) | 1.8 min (90%) |
| **Validate config** | 10 min (trial & error) | 5 sec (validate command) | 9.9 min (99%) |

**Total time for typical project setup**:
- **Current**: ~1 hour
- **Proposed**: ~5 minutes
- **Savings**: **92%** time reduction

---

## 9. Implementation Priority

### Must Have (Phase 1-2) - 4 weeks

1. ✅ **Enhanced discovery** (`list mcus`, `show mcu`, `search pin`)
2. ✅ **Interactive init** (`alloy init` wizard)
3. ✅ **Build integration** (`alloy build` commands)
4. ✅ **Basic validation** (`alloy config validate`)

**ROI**: High (solves 80% of pain points)
**Complexity**: Medium (leverages existing code)

### Should Have (Phase 3-4) - 2 weeks

1. ✅ **Pinout display** (ASCII art pinout)
2. ✅ **Documentation integration** (`alloy docs` commands)
3. ✅ **Templates** (project templates)
4. ✅ **Smart recommendations** (pin conflict detection)

**ROI**: Medium-High (significantly improves UX)
**Complexity**: Medium

### Nice to Have (Phase 5-6) - 2 weeks

1. ⭐ **Interactive pinout explorer** (arrow keys, highlighting)
2. ⭐ **Meson support** (alternative build system)
3. ⭐ **Web UI** (browser-based configuration)
4. ⭐ **VS Code extension** (IDE integration)

**ROI**: Medium (advanced users benefit)
**Complexity**: High

---

## 10. Conclusion & Recommendation

### Summary

**Transform Alloy CLI from basic code generator to comprehensive embedded development assistant.**

**Key Improvements**:
1. 📋 **Discovery** - Instant access to MCU/board/peripheral information
2. 🚀 **Initialization** - Interactive wizard for zero-friction project setup
3. 🔨 **Build** - One-command build/flash workflow
4. 📚 **Documentation** - Integrated datasheets and API reference
5. ✅ **Validation** - Automatic configuration checking

**Build System**: Keep CMake (industry standard) + add Meson (optional, experimental)

**Technology**: Rich + Typer (beautiful terminal output, modern CLI framework)

**Effort**: 8 weeks (176 hours) for full implementation

**Impact**:
- **10x faster** project setup (1 hour → 5 minutes)
- **Zero learning curve** (guided wizard)
- **Professional tool** (rivals modm but simpler)
- **Better DX** (delightful developer experience)

### Recommendation

**Start with Phase 1-2** (Must Have features):
- Enhanced discovery commands
- Interactive project initialization
- Build integration
- Basic validation

**Timeline**: 4 weeks for MVP
**ROI**: Immediate (addresses top user pain points)

**Then add Phase 3-4** (Should Have):
- Pinout visualization
- Documentation integration
- Templates

**Timeline**: +2 weeks
**Total**: 6 weeks for production-ready CLI

**Meson support**: Add later as optional feature (experimental)

---

**Next Steps**:
1. Review and approve this proposal
2. Start with Phase 1 implementation (enhanced discovery)
3. Gather user feedback
4. Iterate and improve

**The goal is to make Alloy the easiest embedded framework to use, without sacrificing power or flexibility.**
