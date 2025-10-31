# NXP Generated Code

**MCUs Supported**: 12
**Families**: mimxrt1011, mimxrt1021, mimxrt1052, mimxrt1062, mimxrt1064, mk22f51212, mk64f12, mk65f18, mk66f18, mkl26z4, mkw41z4, qn908xc

## Structure

```
nxp/
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
├── mk22f51212/
│   ├── MK22F51212/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mk64f12/
│   ├── MK64F12/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mk65f18/
│   ├── MK65F18/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mk66f18/
│   ├── MK66F18/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mkl26z4/
│   ├── MKL26Z4/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── mkw41z4/
│   ├── MKW41Z4/
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
