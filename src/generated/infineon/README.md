# INFINEON Generated Code

**MCUs Supported**: 17
**Families**: imc300a, imm100a, tle984x, tle9868, tle986x, tle987x, xmc1100, xmc1200, xmc1300, xmc1400, xmc4100, xmc4200, xmc4300, xmc4400, xmc4500, xmc4700, xmc4800

## Structure

```
infineon/
├── imc300a/
│   ├── IMC300A/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── imm100a/
│   ├── IMM100A/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tle984x/
│   ├── TLE984X/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tle9868/
│   ├── TLE9868/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tle986x/
│   ├── TLE986X/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tle987x/
│   ├── TLE987X/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc1100/
│   ├── XMC1100/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc1200/
│   ├── XMC1200/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc1300/
│   ├── XMC1300/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc1400/
│   ├── XMC1400/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4100/
│   ├── XMC4100/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4200/
│   ├── XMC4200/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4300/
│   ├── XMC4300/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4400/
│   ├── XMC4400/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4500/
│   ├── XMC4500/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4700/
│   ├── XMC4700/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── xmc4800/
│   ├── XMC4800/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
```

## Files Generated

Each MCU directory contains:

- **startup.cpp** - Startup code with:
  - Reset handler (.data/.bss initialization)
  - Vector table (all interrupts)
  - Weak default handlers

- **peripherals.hpp** - Peripheral definitions:
  - Memory map (flash/RAM addresses)
  - All peripheral base addresses
  - Register structures
  - Peripheral instances (ready to use)

## Usage

```cpp
// In your CMakeLists.txt
set(ALLOY_MCU "STM32F103C8")  # Or any supported MCU

# Generated code is automatically included
target_sources(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)
```

## Regenerating

To regenerate all code for this vendor:

```bash
cd tools/codegen
python3 generate_all.py --vendor {vendor_name}
```
