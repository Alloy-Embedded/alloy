# Flashing Guide

This guide explains how to upload (flash) compiled firmware to development boards.

## Quick Reference

| Board | Method | Tool | Command |
|-------|--------|------|---------|
| Blue Pill (STM32F103) | ST-Link / Serial | OpenOCD / stm32flash | `make flash` or manual |
| STM32F4 Discovery | ST-Link (on-board) | OpenOCD / STM32CubeProgrammer | `make flash` or manual |
| Raspberry Pi Pico | USB Mass Storage | Drag-and-drop | Copy .uf2 file |
| Arduino Zero | USB Serial (bossac) | bossac / Arduino IDE | `make flash` or manual |
| ESP32 DevKit | USB Serial | esptool.py | `make flash` or manual |

## Prerequisites

### Flash Tools Installation

#### OpenOCD (for STM32 with ST-Link)

```bash
# Ubuntu/Debian
sudo apt install openocd

# macOS
brew install openocd

# Verify
openocd --version
```

#### stm32flash (for STM32 via serial)

```bash
# Ubuntu/Debian
sudo apt install stm32flash

# macOS
brew install stm32flash
```

#### esptool (for ESP32)

```bash
# Python pip (recommended)
pip install esptool

# Or via ESP-IDF (already included)
. ~/esp/esp-idf/export.sh

# Verify
esptool.py --version
```

#### bossac (for Arduino Zero)

```bash
# Ubuntu/Debian
sudo apt install bossac

# macOS
brew install bossac

# Or use Arduino IDE built-in bossac
```

## Board-Specific Instructions

### 1. Blue Pill (STM32F103C8)

#### Method A: Using ST-Link V2 (Recommended)

**Hardware Setup**:
1. Connect ST-Link V2 to Blue Pill:
   - ST-Link `SWDIO` → Blue Pill `SWDIO`
   - ST-Link `SWCLK` → Blue Pill `SWCLK`
   - ST-Link `GND` → Blue Pill `GND`
   - ST-Link `3.3V` → Blue Pill `3.3V` (optional, for power)

**Flash via OpenOCD**:
```bash
# Build first
cmake -DALLOY_BOARD=bluepill ..
make blink_stm32f103

# Flash using OpenOCD
openocd -f interface/stlink.cfg \
        -f target/stm32f1x.cfg \
        -c "program blink_stm32f103.elf verify reset exit"
```

**Flash using CMake target**:
```bash
make flash_blink_stm32f103
```

#### Method B: Using USB-Serial Adapter

**Hardware Setup**:
1. Connect USB-Serial adapter to Blue Pill:
   - TX → A10 (RX1)
   - RX → A9 (TX1)
   - GND → GND
2. Set BOOT0 jumper to 1 (boot from System Memory)
3. Press reset button

**Flash**:
```bash
stm32flash -w blink_stm32f103.bin -v -g 0x0 /dev/ttyUSB0
```

4. Set BOOT0 back to 0 (boot from Flash)
5. Press reset

---

### 2. STM32F4 Discovery (STM32F407VG)

The STM32F4 Discovery has an **on-board ST-Link/V2** debugger - no external hardware needed!

**Hardware Setup**:
1. Connect board to computer via USB (mini-USB on ST-Link side)
2. No jumpers needed - ready to flash

**Flash via OpenOCD**:
```bash
# Build first
cmake -DALLOY_BOARD=stm32f407vg ..
make blink_stm32f407

# Flash
openocd -f board/stm32f4discovery.cfg \
        -c "program blink_stm32f407.elf verify reset exit"
```

**Flash using CMake target**:
```bash
make flash_blink_stm32f407
```

**Alternative - ST Flash tool**:
```bash
# Install st-flash (from stlink tools)
sudo apt install stlink-tools

# Flash
st-flash write blink_stm32f407.bin 0x08000000
```

---

### 3. Raspberry Pi Pico (RP2040)

**Method A: USB Mass Storage (Easiest!)**

1. Hold BOOTSEL button while connecting Pico to USB
2. Pico appears as USB drive "RPI-RP2"
3. Drag-and-drop `.uf2` file to the drive
4. Pico automatically reboots and runs your code

```bash
# Build with UF2 output
cmake -DALLOY_BOARD=rp_pico ..
make blink_rp_pico

# Copy to Pico (macOS example)
cp blink_rp_pico.uf2 /Volumes/RPI-RP2/

# Linux example
cp blink_rp_pico.uf2 /media/$USER/RPI-RP2/
```

**Method B: Using picotool**

```bash
# Install picotool
git clone https://github.com/raspberrypi/picotool.git
cd picotool
mkdir build && cd build
cmake ..
make
sudo make install

# Flash
picotool load blink_rp_pico.elf
picotool reboot
```

**Method C: Using SWD with probe**

```bash
openocd -f interface/cmsis-dap.cfg \
        -f target/rp2040.cfg \
        -c "program blink_rp_pico.elf verify reset exit"
```

---

### 4. Arduino Zero (ATSAMD21G18)

**Method A: Using bossac (Native USB)**

1. Double-press reset button to enter bootloader mode
2. Board appears as USB serial device

```bash
# Build first
cmake -DALLOY_BOARD=arduino_zero ..
make blink_arduino_zero

# Flash (Linux)
bossac -p /dev/ttyACM0 -e -w -v -R blink_arduino_zero.bin

# Flash (macOS)
bossac -p /dev/cu.usbmodem* -e -w -v -R blink_arduino_zero.bin
```

**Method B: Using Arduino IDE**

1. Open Arduino IDE
2. Tools → Board → Arduino Zero (Native USB Port)
3. Sketch → Upload Using Programmer
4. Point to your .bin file

**Method C: Using CMSIS-DAP**

```bash
openocd -f interface/cmsis-dap.cfg \
        -f target/at91samdXX.cfg \
        -c "program blink_arduino_zero.elf verify reset exit"
```

---

### 5. ESP32 DevKit

**Using esptool.py**:

```bash
# Build first (with ESP-IDF sourced)
. ~/esp/esp-idf/export.sh
cmake -DALLOY_BOARD=esp32_devkit \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/xtensa-esp32-elf.cmake ..
make blink_esp32

# Find USB port
# Linux: usually /dev/ttyUSB0 or /dev/ttyACM0
# macOS: usually /dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART

# Flash
esptool.py --chip esp32 \
           --port /dev/ttyUSB0 \
           --baud 115200 \
           write_flash -z 0x1000 blink_esp32.bin

# Or use CMake target
make flash_blink_esp32
```

**Monitor serial output**:
```bash
# Using esptool
esptool.py --port /dev/ttyUSB0 monitor

# Using screen
screen /dev/ttyUSB0 115200

# Using minicom
minicom -D /dev/ttyUSB0 -b 115200
```

## CMake Flash Targets

Most boards have automatic flash targets created by the build system:

```bash
# List available targets
make help | grep flash

# Flash specific example
make flash_blink_<board_id>

# Example
make flash_blink_stm32f103
```

## Troubleshooting

### "Error: No device found"

**ST-Link**:
- Check USB connection
- Try different USB port/cable
- Check ST-Link firmware (update if needed)
- Linux: Add udev rules for ST-Link

```bash
# Add udev rules (Linux)
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```

### "Permission denied" on /dev/ttyUSB0

**Linux**:
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### ESP32 fails to enter bootloader

**Solution**:
1. Hold BOOT button
2. Press and release RESET button
3. Release BOOT button
4. Try flashing again

### RP2040 not appearing as USB drive

**Solution**:
1. Disconnect USB
2. Hold BOOTSEL button
3. Connect USB while holding BOOTSEL
4. Release BOOTSEL after 1 second
5. Drive should appear

### "Verification failed" errors

**Causes**:
- Bad USB cable
- Insufficient power supply
- Flash memory corruption

**Solutions**:
- Try different USB cable
- Use external power supply
- Erase flash completely first:
  ```bash
  # STM32 via OpenOCD
  openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
          -c "init" -c "reset halt" -c "flash erase_sector 0 0 last" -c "exit"

  # ESP32
  esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
  ```

## Verifying Flash

After flashing, verify your code is running:

### Visual Verification
- **LED should blink** at 1 Hz (0.5s ON, 0.5s OFF)
- Check the correct LED for your board:
  - Blue Pill: PC13 (on-board LED, active LOW)
  - STM32F4 Discovery: PD12/13/14/15 (4 colored LEDs)
  - Raspberry Pi Pico: GPIO25 (green LED, active HIGH)
  - Arduino Zero: GPIO13/PA17 (orange LED, active HIGH)
  - ESP32 DevKit: GPIO2 (blue LED, active HIGH - may vary by board)

### Serial Output (ESP32 only)
```bash
# ESP32 prints startup messages
esptool.py --port /dev/ttyUSB0 monitor
```

### Debugger Verification
```bash
# STM32 - check PC register
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "init" -c "reset halt" -c "reg pc" -c "exit"
# Should show PC at application start address (usually 0x08000000 + offset)
```

## Next Steps

- **Debug your application**: See [Debugging Guide](debugging.md) (coming soon)
- **Monitor serial output**: See [Serial Communication Guide](serial.md) (coming soon)
- **Measure performance**: See [Profiling Guide](profiling.md) (coming soon)

## See Also

- [Building Guide](building_for_boards.md) - How to compile firmware
- [Troubleshooting Guide](troubleshooting.md) - Common issues
- [Boards Documentation](boards.md) - Board specifications and pinouts
