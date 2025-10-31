# RENESAS Generated Code

**MCUs Supported**: 11
**Families**: r7fa2a1ab, r7fa2e1a9, r7fa2e2a7, r7fa2l1ab, r7fa4m1ab, r7fa4w1ad, r7fa6e2bb, r7fa6m1ad, r7fa6m2af, r7fa6m3ah, r7fa6t1ad

## Structure

```
renesas/
├── r7fa2a1ab/
│   ├── R7FA2A1AB/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa2e1a9/
│   ├── R7FA2E1A9/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa2e2a7/
│   ├── R7FA2E2A7/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa2l1ab/
│   ├── R7FA2L1AB/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa4m1ab/
│   ├── R7FA4M1AB/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa4w1ad/
│   ├── R7FA4W1AD/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa6e2bb/
│   ├── R7FA6E2BB/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa6m1ad/
│   ├── R7FA6M1AD/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa6m2af/
│   ├── R7FA6M2AF/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa6m3ah/
│   ├── R7FA6M3AH/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── r7fa6t1ad/
│   ├── R7FA6T1AD/
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
