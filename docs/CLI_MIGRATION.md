# CLI Migration Guide: alloy.py → ucore

This guide explains the migration from the old `alloy.py` CLI to the new unified `ucore` CLI.

## Why the Change?

The old system had multiple confusing ways to build and flash:
- ❌ CMake presets (complex, hard to remember)
- ❌ Direct CMake commands (verbose, board-specific)
- ❌ Make targets (inconsistent naming)
- ❌ Manual flash commands (different per board)

**The new `ucore` CLI provides ONE simple way to do everything:**
- ✅ Simple, consistent commands
- ✅ Works for all boards and examples
- ✅ No need to remember CMake flags or board-specific flash commands

## Command Comparison

### Old Way (Multiple Methods)

#### Method 1: CMake Presets (confusing)
```bash
# Had to learn preset names
./alloy.py build nucleo-f401re-debug
./alloy.py flash nucleo-f401re-release
```

#### Method 2: Direct CMake (verbose)
```bash
# Had to remember all flags
cmake -B build-nucleo_f401re \
      -DMICROCORE_BOARD=nucleo_f401re \
      -DMICROCORE_BUILD_TESTS=OFF \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build-nucleo_f401re --target blink
```

#### Method 3: Manual Flash (board-specific)
```bash
# Different command for each board type
st-flash write build-nucleo_f401re/examples/blink/blink.bin 0x08000000
# or
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program ..."
```

### New Way (One Simple Command)

```bash
# Build any example for any board
./ucore build nucleo_f401re blink

# Build and flash in one command
./ucore flash nucleo_f401re blink
```

**That's it!** No presets, no flags, no board-specific flash commands.

## Migration Examples

### Example 1: Building Blink

**Old:**
```bash
# Using presets
./alloy.py build nucleo-f401re-release

# Or using direct CMake
cmake -B build-nucleo_f401re -DMICROCORE_BOARD=nucleo_f401re ...
cmake --build build-nucleo_f401re --target blink
```

**New:**
```bash
./ucore build nucleo_f401re blink
```

### Example 2: Flashing Firmware

**Old:**
```bash
# Build first
./alloy.py build nucleo-f401re-release

# Then flash manually with board-specific command
st-flash write build/nucleo-f401re-release/examples/blink/blink.bin 0x08000000
```

**New:**
```bash
./ucore flash nucleo_f401re blink
```

### Example 3: Working with RTOS Examples

**Old:**
```bash
# Configure for RTOS example
cmake -B build-nucleo_f401re \
      -DMICROCORE_BOARD=nucleo_f401re \
      -DCMAKE_BUILD_TYPE=Debug \
      ...

# Build specific example
cmake --build build-nucleo_f401re --target rtos_simple_tasks

# Flash manually
st-flash write build-nucleo_f401re/examples/rtos/simple_tasks/rtos_simple_tasks.bin 0x08000000
```

**New:**
```bash
# Build
./ucore build nucleo_f401re rtos/simple_tasks

# Or build and flash
./ucore flash nucleo_f401re rtos/simple_tasks
```

### Example 4: Testing Multiple Boards

**Old:**
```bash
# Needed different presets or commands for each board
./alloy.py build nucleo-f401re-release
./alloy.py build nucleo-g071rb-release
./alloy.py build nucleo-f722ze-release
```

**New:**
```bash
# Same command, just change the board name
./ucore build nucleo_f401re blink
./ucore build nucleo_g071rb blink
./ucore build nucleo_f722ze blink
```

## New Features

### 1. List Available Options

**Discover boards without looking at documentation:**
```bash
./ucore list boards
```

**Discover examples without browsing directories:**
```bash
./ucore list examples
```

### 2. Consistent Command Structure

Every command follows the same pattern:
```bash
./ucore <action> <board> <example>
```

Examples:
- `./ucore build nucleo_f401re blink`
- `./ucore flash nucleo_g071rb uart_logger`
- `./ucore build nucleo_f722ze rtos/simple_tasks`

### 3. Automatic Flash Tool Detection

The CLI automatically:
- Detects which flash tool to use (st-flash, openocd, etc.)
- Uses correct board-specific parameters
- Prompts you to connect the board
- Verifies and resets after flashing

### 4. Clean Builds

```bash
# Clean specific board
./ucore clean nucleo_f401re

# Clean all boards
./ucore clean
```

## FAQ

### Q: Can I still use the old methods?

**A:** Yes, direct CMake commands still work, but the new `ucore` CLI is the recommended way.

### Q: What happened to CMake presets?

**A:** They still exist for advanced users, but the `ucore` CLI provides a simpler interface that doesn't require understanding presets.

### Q: Do I need to update my build scripts?

**A:** Yes, if you have CI/CD or automation scripts using `alloy.py`, update them to use `./ucore` instead.

### Q: Can I build debug versions?

**A:** Yes, use the `--debug` flag:
```bash
./ucore build nucleo_f401re blink --debug
./ucore flash nucleo_f401re blink --debug
```

### Q: Where are the binaries located?

**A:** Same place as before:
```
build-<board>/examples/<example>/<example_name>.[elf|bin|hex]
```

Example:
```
build-nucleo_f401re/examples/blink/blink.elf
build-nucleo_f401re/examples/blink/blink.bin
build-nucleo_f401re/examples/blink/blink.hex
```

### Q: Is the old `alloy.py` still maintained?

**A:** The `alloy.py` file remains for compatibility but is no longer the primary interface. New features will only be added to `ucore`.

## Quick Reference

| Task | Old Command | New Command |
|------|-------------|-------------|
| List boards | _Not available_ | `./ucore list boards` |
| List examples | _Not available_ | `./ucore list examples` |
| Build | `./alloy.py build preset` | `./ucore build board example` |
| Flash | `./alloy.py flash preset` | `./ucore flash board example` |
| Clean | `./alloy.py clean preset` | `./ucore clean board` |
| Clean all | `./alloy.py clean` | `./ucore clean` |

## Getting Help

```bash
# General help
./ucore --help

# Command-specific help
./ucore build --help
./ucore flash --help
```

## See Also

- [QUICKSTART.md](../QUICKSTART.md) - Complete quick start guide
- [README.md](../README.md) - Main project documentation
- [TESTING_BOARDS.md](../TESTING_BOARDS.md) - Hardware testing guide
