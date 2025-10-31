# Alloy Generated MCU Support

**Total MCUs**: 3
**Vendors**: 2

## Supported MCUs by Vendor


### EXAMPLE VENDOR (1 MCUs)

**EXAMPLE**: EXAMPLE_MCU


### ST (2 MCUs)

**STM32F1**: STM32F103C8, STM32F103CB


## Directory Structure

```
src/generated/
├── {vendor}/
│   ├── {family}/
│   │   ├── {mcu}/
│   │   │   ├── startup.cpp
│   │   │   └── peripherals.hpp
│   │   └── ...
│   └── README.md
└── INDEX.md (this file)
```

## Using Generated Code

### In CMakeLists.txt

```cmake
# Set your MCU
set(ALLOY_MCU "STM32F103C8")

# Generated code is in src/generated/
set(ALLOY_GENERATED_DIR "${CMAKE_SOURCE_DIR}/src/generated/st/stm32f1/stm32f103c8")

# Add startup code
target_sources(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}/startup.cpp
)

# Include peripheral definitions
target_include_directories(my_firmware PRIVATE
    ${ALLOY_GENERATED_DIR}
)
```

### In C++ Code

```cpp
#include "peripherals.hpp"

using namespace alloy::generated::stm32f103c8;

// Use GPIO
auto* gpioc = gpio::GPIOC;
gpioc->BSRR = (1U << 13);  // Set PC13

// Use USART
auto* uart = usart::USART1;
uart->DR = 'A';  // Send character
```

## Regenerating Code

To regenerate all MCU code:

```bash
cd tools/codegen

# Sync SVD files
python3 sync_svd.py --update

# Parse all SVDs
python3 parse_all_svds.py

# Generate all code
python3 generate_all.py --all
```

## Last Generated

{Path(__file__).stem}: Run `python3 generate_all.py --all` to update
