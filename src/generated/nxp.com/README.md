# NXP.COM Generated Code

**MCUs Supported**: 6
**Families**: mimxrt1011, mimxrt1021, mimxrt1052, mimxrt1062, mimxrt1064, qn908xc

## Structure

```
nxp.com/
├── mimxrt1011/
│   ├── MIMXRT1011/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mimxrt1021/
│   ├── MIMXRT1021/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mimxrt1052/
│   ├── MIMXRT1052/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mimxrt1062/
│   ├── MIMXRT1062/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mimxrt1064/
│   ├── MIMXRT1064/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── qn908xc/
│   ├── QN908XC/
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
