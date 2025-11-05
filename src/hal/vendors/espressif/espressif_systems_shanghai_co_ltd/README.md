# ESPRESSIF_SYSTEMS_SHANGHAI_CO_LTD Generated Code

**MCUs Supported**: 8
**Families**: esp32, esp32_c2, esp32_c3, esp32_c6_lp, esp32_h2, esp32_p4, esp32_s2, esp32_s3

## Structure

```
espressif_systems_shanghai_co_ltd/
├── esp32/
│   ├── ESP32/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_c2/
│   ├── ESP32_C2/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_c3/
│   ├── ESP32_C3/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_c6_lp/
│   ├── ESP32_C6_LP/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_h2/
│   ├── ESP32_H2/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_p4/
│   ├── ESP32_P4/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_s2/
│   ├── ESP32_S2/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── esp32_s3/
│   ├── ESP32_S3/
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
