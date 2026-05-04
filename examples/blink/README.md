# blink — Universal LED Blink

Blinks the onboard LED on every supported Alloy board.  
One source file, zero board-specific code in `main.cpp`.

## Supported boards

| Board | MCU | Arch | LED | Flash tool |
|---|---|---|---|---|
| `same70_xplained` | ATSAME70Q21B | Cortex-M7 | PC8 (active-LOW) | OpenOCD |
| `nucleo_g071rb` | STM32G071RBT6 | Cortex-M0+ | PA5 | OpenOCD / STM32CubeProg |
| `nucleo_g0b1re` | STM32G0B1RET6 | Cortex-M0+ | PA5 | OpenOCD / STM32CubeProg |
| `nucleo_f401re` | STM32F401RET6 | Cortex-M4 | PA5 | OpenOCD / STM32CubeProg |
| `raspberry_pi_pico` | RP2040 | Cortex-M0+ | GP25 | UF2 drag-and-drop |
| `esp32_devkit` | ESP32 | Xtensa LX6 | GPIO2 | esptool |
| `esp32c3_devkitm` | ESP32-C3 | RISC-V RV32 | GPIO8 | esptool |
| `esp32s3_devkitc` | ESP32-S3 | Xtensa LX7 | GPIO8 | esptool |
| `esp_wrover_kit` | ESP32 | Xtensa LX6 | GPIO2 (green ch.) | esptool |
| `avr128da32_curiosity_nano` | AVR128DA32 | AVR | PC6 (active-LOW) | avrdude |

## Prerequisites

- **ARM boards** — `arm-none-eabi-gcc`, `openocd`, CMake ≥ 3.25, Ninja
- **ESP32 boards** — Espressif toolchain + `esptool.py`
- **AVR** — `avr-gcc`, `avrdude`
- `alloyctl` — `python3 scripts/alloyctl.py` (no install needed)

## Build & flash

### alloyctl (recommended)

One command builds and flashes in a single step:

```bash
python3 scripts/alloyctl.py flash --board <board> --target blink --build-first
```

Examples:

```bash
# SAME70 Xplained Ultra
python3 scripts/alloyctl.py flash --board same70_xplained --target blink --build-first

# Nucleo-G071RB
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink --build-first

# Nucleo-F401RE
python3 scripts/alloyctl.py flash --board nucleo_f401re --target blink --build-first

# Raspberry Pi Pico  (builds the UF2; drag to RPI-RP2 drive)
python3 scripts/alloyctl.py build --board raspberry_pi_pico --target blink

# ESP32-DevKit
python3 scripts/alloyctl.py flash --board esp32_devkit --target blink --build-first

# ESP32-C3-DevKitM
python3 scripts/alloyctl.py flash --board esp32c3_devkitm --target blink --build-first
```

#### Build only (no hardware needed)

```bash
python3 scripts/alloyctl.py build --board nucleo_g071rb --target blink
# ELF/HEX/BIN land in build/hw/g071/examples/blink/
```

#### Flash only (already built)

```bash
python3 scripts/alloyctl.py flash --board same70_xplained --target blink
```

#### Use STM32CubeProgrammer instead of OpenOCD (STM32 boards)

```bash
python3 scripts/alloyctl.py flash --board nucleo_g071rb --target blink \
    --build-first --flash-backend stm32cube
```

### Raw CMake (any board)

```bash
# Configure
cmake -S . -B build/blink \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DALLOY_BOARD=nucleo_g071rb \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
    -DALLOY_BUILD_EXAMPLES=ON

# Build
cmake --build build/blink --target blink -j8

# Flash (OpenOCD example)
openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
    -c "program build/blink/examples/blink/blink.elf verify reset exit"
```

## Expected output

The onboard LED toggles every ~500 ms (busy-wait delay, no SysTick yet).

```
LED ON  →  500 ms  →  LED OFF  →  500 ms  →  repeat
```

## How it works

`main.cpp` is 100 % portable:

```cpp
int main() {
    board::init();          // clock + GPIO setup (board-specific)
    while (true) {
        board::led::toggle();
        busy_delay();
    }
}
```

The build system defines `ALLOY_BOARD_<NAME>` from `--board <name>`, which
selects the correct `boards/<board>/board.hpp` via `#elif` chains.
`board::init()` is implemented in `boards/<board>/board.cpp` and compiled in
by the example's `CMakeLists.txt`.

## Adding a new board

1. Create `boards/<new_board>/board.hpp` and `board.cpp` implementing:
   - `board::init()` — clock + LED GPIO init
   - `board::led::toggle()` — toggle the LED
2. Add an `#elif defined(ALLOY_BOARD_<NEW_BOARD_UPPER>)` branch in `main.cpp`.
3. Add a `board.json` in `boards/<new_board>/` so alloyctl auto-discovers it.
