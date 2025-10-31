# NORDIC SEMICONDUCTOR Generated Code

**MCUs Supported**: 10
**Families**: nrf51, nrf52, nrf52805, nrf52810, nrf52811, nrf52820, nrf52833, nrf52840, nrf5340_network, nrf9160

## Structure

```
nordic semiconductor/
├── nrf51/
│   ├── NRF51/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52/
│   ├── NRF52/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52805/
│   ├── NRF52805/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52810/
│   ├── NRF52810/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52811/
│   ├── NRF52811/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52820/
│   ├── NRF52820/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52833/
│   ├── NRF52833/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf52840/
│   ├── NRF52840/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf5340_network/
│   ├── NRF5340_NETWORK/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── nrf9160/
│   ├── NRF9160/
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
