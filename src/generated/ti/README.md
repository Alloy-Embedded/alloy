# TI Generated Code

**MCUs Supported**: 8
**Families**: tm4c1231d5pz, tm4c1231h6pz, tm4c1232e6pm, tm4c1236e6pm, tm4c1237d5pz, tm4c1237h6pz, tm4c123fe6pm, tm4c123gh6pm

## Structure

```
ti/
├── tm4c1231d5pz/
│   ├── TM4C1231D5PZ/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c1231h6pz/
│   ├── TM4C1231H6PZ/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c1232e6pm/
│   ├── TM4C1232E6PM/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c1236e6pm/
│   ├── TM4C1236E6PM/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c1237d5pz/
│   ├── TM4C1237D5PZ/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c1237h6pz/
│   ├── TM4C1237H6PZ/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c123fe6pm/
│   ├── TM4C123FE6PM/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── tm4c123gh6pm/
│   ├── TM4C123GH6PM/
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
