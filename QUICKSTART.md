# MicroCore Quick Start Guide

Simple, unified commands to build and flash examples on any supported board.

## 🚀 Installation

### 1. Install ARM Toolchain

```bash
# Using xPack (recommended)
./scripts/install-xpack-toolchain.sh

# Or using Homebrew (macOS)
brew install --cask gcc-arm-embedded
```

### 2. Install Flash Tools

For STM32 boards:
```bash
# macOS
brew install stlink

# Linux
sudo apt-get install stlink-tools
```

For SAME70 boards:
```bash
# macOS
brew install openocd

# Linux
sudo apt-get install openocd
```

## 📋 List Available Options

### List all supported boards:
```bash
./ucore list boards
```

**Supported boards:**
- `nucleo_f401re` - STM32F401RE (Cortex-M4, 84 MHz)
- `nucleo_f722ze` - STM32F722ZE (Cortex-M7, 216 MHz)
- `nucleo_g071rb` - STM32G071RB (Cortex-M0+, 64 MHz)
- `nucleo_g0b1re` - STM32G0B1RE (Cortex-M0+, 64 MHz)
- `same70_xplained` - ATSAME70Q21B (Cortex-M7, 300 MHz)

### List all available examples:
```bash
./ucore list examples
```

**Available examples:**
- `blink` - Simple LED blink
- `systick_demo` - SysTick timer demonstration
- `uart_logger` - UART logging example
- `rtos/simple_tasks` - RTOS multi-tasking example
- And more...

## 🔨 Build Examples

Build any example for any board:

```bash
./ucore build <board> <example>
```

**Examples:**

```bash
# Build blink for Nucleo F401RE
./ucore build nucleo_f401re blink

# Build RTOS example for Nucleo G071RB
./ucore build nucleo_g071rb rtos/simple_tasks

# Build UART logger for Nucleo F722ZE
./ucore build nucleo_f722ze uart_logger

# Build debug version (with debug symbols)
./ucore build nucleo_f401re blink --debug
```

**Output:**
- Binaries are in `build-<board>/examples/<example>/`
- Formats: `.elf` (with debug info), `.bin` (raw binary), `.hex` (Intel HEX)

## 📡 Flash to Hardware

Build and flash in one command:

```bash
./ucore flash <board> <example>
```

**Examples:**

```bash
# Flash blink to Nucleo F401RE
./ucore flash nucleo_f401re blink

# Flash RTOS example to Nucleo G071RB
./ucore flash nucleo_g071rb rtos/simple_tasks
```

The CLI will:
1. ✅ Build the example
2. ⏸️  Prompt you to connect your board
3. 📡 Flash via ST-Link/OpenOCD
4. ✅ Verify and reset

## 🧹 Clean Builds

```bash
# Clean specific board
./ucore clean nucleo_f401re

# Clean all boards
./ucore clean
```

## 💡 Common Workflows

### First-time setup:
```bash
# 1. List what's available
./ucore list boards
./ucore list examples

# 2. Build and test locally
./ucore build nucleo_f401re blink

# 3. Flash to hardware
./ucore flash nucleo_f401re blink
```

### Development workflow:
```bash
# Edit code...

# Quick rebuild and flash
./ucore flash nucleo_f401re blink
```

### Test on multiple boards:
```bash
# Build for all boards
./ucore build nucleo_f401re blink
./ucore build nucleo_f722ze blink
./ucore build nucleo_g071rb blink
./ucore build nucleo_g0b1re blink
```

## 🎯 Example: Blink LED

**Goal:** Make the onboard LED blink on Nucleo F401RE

```bash
# 1. Build
./ucore build nucleo_f401re blink

# 2. Connect your board via USB

# 3. Flash
./ucore flash nucleo_f401re blink

# 4. The green LED (LD2) should blink at 1 Hz!
```

## 🐛 Troubleshooting

### Flash tool not found

**Error:** `Flash tool not found: st-flash`

**Solution:**
```bash
# macOS
brew install stlink

# Linux
sudo apt-get install stlink-tools
```

### Board not detected

**Error:** Flash fails with "Cannot find target"

**Solutions:**
- Ensure board is connected via USB
- Check USB cable (some are power-only)
- Try a different USB port
- On Linux: May need `sudo` or udev rules

### Build fails

**Error:** Compilation errors

**Solutions:**
1. Ensure ARM toolchain is installed: `arm-none-eabi-gcc --version`
2. Clean and rebuild: `./ucore clean nucleo_f401re && ./ucore build nucleo_f401re blink`
3. Check example exists: `./ucore list examples`

## 📚 Next Steps

- **Explore examples:** Check out `examples/` directory
- **Read board docs:** See `boards/<board>/README.md`
- **API documentation:** See `docs/API_REFERENCE.md`
- **Add your own example:** Copy an existing example and modify

## 🔗 Resources

- **Main README:** [README.md](README.md)
- **Board testing guide:** [TESTING_BOARDS.md](TESTING_BOARDS.md)
- **Contributing:** [CONTRIBUTING.md](CONTRIBUTING.md)

---

**One command to rule them all:** `./ucore flash <board> <example>` ✨
