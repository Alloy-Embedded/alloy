## Why

With 3 different architectures (RL78, ARM Cortex-M, Xtensa), we need a flexible code generator that can handle different register layouts, startup codes, and linker scripts.

## What Changes

- Enhance code generator to support multiple architectures
- Add SVD parser for ARM-based MCUs (STM32F103)
- Add custom database format for RL78 (no SVD available)
- Add ESP32 database (based on ESP-IDF headers)
- Generate architecture-specific startup code
- Generate architecture-specific linker scripts

## Impact

- Affected specs: codegen-system (new capability)
- Affected code: tools/codegen/generator.py, tools/codegen/database/
- Enables scaling to hundreds of MCUs across different architectures
