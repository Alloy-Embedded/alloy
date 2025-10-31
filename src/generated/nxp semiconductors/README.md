# NXP SEMICONDUCTORS Generated Code

**MCUs Supported**: 6
**Families**: mk22f51212, mk64f12, mk65f18, mk66f18, mkl26z4, mkw41z4

## Structure

```
nxp semiconductors/
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
