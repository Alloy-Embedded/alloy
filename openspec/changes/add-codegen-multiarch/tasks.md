## 1. Generator Architecture

- [ ] 1.1 Design database schema for multi-architecture support
- [ ] 1.2 Add 'architecture' field (arm-cm0, arm-cm3, arm-cm4, rl78, xtensa)
- [ ] 1.3 Create architecture-specific template selection logic
- [ ] 1.4 Add support for different startup code formats (C vs C++, ASM)

## 2. ARM SVD Support

- [ ] 2.1 Create SVD parser in `tools/codegen/svd_parser.py`
- [ ] 2.2 Download STM32F103 SVD from ARM CMSIS-SVD repo
- [ ] 2.3 Parse peripherals, registers, and interrupts from SVD
- [ ] 2.4 Generate JSON database from SVD
- [ ] 2.5 Test with STM32F103xB SVD file

## 3. RL78 Database

- [ ] 3.1 Create `tools/codegen/database/families/rl78.json`
- [ ] 3.2 Manually define RL78 peripherals (no SVD available)
- [ ] 3.3 Define port registers (P, PM, PU, etc.)
- [ ] 3.4 Define SAU registers for UART
- [ ] 3.5 Define interrupt vector table for RL78

## 4. ESP32 Database

- [ ] 4.1 Create `tools/codegen/database/families/esp32.json`
- [ ] 4.2 Extract GPIO register definitions from ESP-IDF
- [ ] 4.3 Extract UART register definitions
- [ ] 4.4 Define interrupt vector table for ESP32

## 5. Startup Code Templates

- [ ] 5.1 Create `tools/codegen/templates/startup/cortex_m0.c.j2`
- [ ] 5.2 Create `tools/codegen/templates/startup/cortex_m3.c.j2`
- [ ] 5.3 Create `tools/codegen/templates/startup/rl78.c.j2`
- [ ] 5.4 Create `tools/codegen/templates/startup/esp32.c.j2`
- [ ] 5.5 Test generation for each architecture

## 6. Linker Script Templates

- [ ] 6.1 Create `tools/codegen/templates/linker/cortex_m.ld.j2`
- [ ] 6.2 Create `tools/codegen/templates/linker/rl78.ld.j2`
- [ ] 6.3 Create `tools/codegen/templates/linker/esp32.ld.j2`
- [ ] 6.4 Parameterize memory regions (Flash/RAM sizes)

## 7. CMake Integration

- [ ] 7.1 Update `cmake/codegen.cmake` to detect architecture
- [ ] 7.2 Pass architecture to generator.py
- [ ] 7.3 Select correct template based on architecture
- [ ] 7.4 Test code generation for all 3 architectures
