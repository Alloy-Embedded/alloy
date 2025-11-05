# ATMEL Generated Code

**MCUs Supported**: 3
**Families**: at91sam9n12, atsam3s8c, atsama5d35

## Structure

```
atmel/
├── at91sam9n12/
│   ├── AT91SAM9N12/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── atsam3s8c/
│   ├── ATSAM3S8C/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── atsama5d35/
│   ├── ATSAMA5D35/
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
