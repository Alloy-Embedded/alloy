# ARM_LTD Generated Code

**MCUs Supported**: 1
**Families**: cmsdk_cm3

## Structure

```
arm_ltd/
├── cmsdk_cm3/
│   ├── CMSDK_CM3/
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
