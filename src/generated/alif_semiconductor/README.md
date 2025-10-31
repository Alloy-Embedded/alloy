# ALIF_SEMICONDUCTOR Generated Code

**MCUs Supported**: 6
**Families**: ae101f4071542lh_cm55_he_view, ae302f40c1537le_cm55_he_view, ae302f80c1557le_cm55_hp_view, ae302f80f55d5ae_cm55_he_view, ae512f80f5582as_cm55_he_view, ae722f80f55d5ls_cm55_he_view

## Structure

```
alif_semiconductor/
├── ae101f4071542lh_cm55_he_view/
│   ├── AE101F4071542LH_CM55_HE_VIEW/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── ae302f40c1537le_cm55_he_view/
│   ├── AE302F40C1537LE_CM55_HE_VIEW/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── ae302f80c1557le_cm55_hp_view/
│   ├── AE302F80C1557LE_CM55_HP_VIEW/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── ae302f80f55d5ae_cm55_he_view/
│   ├── AE302F80F55D5AE_CM55_HE_VIEW/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── ae512f80f5582as_cm55_he_view/
│   ├── AE512F80F5582AS_CM55_HE_VIEW/
│   │   ├── startup.cpp
│   │   └── peripherals.hpp
├── ae722f80f55d5ls_cm55_he_view/
│   ├── AE722F80F55D5LS_CM55_HE_VIEW/
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
