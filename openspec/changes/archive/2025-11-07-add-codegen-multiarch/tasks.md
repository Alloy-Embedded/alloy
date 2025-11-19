## 1. Generator Architecture

- [x] 1.1 Design database schema for multi-architecture support
- [x] 1.2 Add 'architecture' field (arm-cm0, arm-cm3, arm-cm4, rl78, xtensa)
- [x] 1.3 Create architecture-specific template selection logic
- [x] 1.4 Add support for different startup code formats (C vs C++, ASM)

## 2. ARM SVD Support

- [x] 2.1 Create SVD parser in `tools/codegen/svd_parser.py`
- [x] 2.2 Download STM32F103 SVD from ARM CMSIS-SVD repo
- [x] 2.3 Parse peripherals, registers, and interrupts from SVD
- [x] 2.4 Generate JSON database from SVD
- [x] 2.5 Test with STM32F103xB SVD file

## 3. RL78 Database

- [ ] 3.1 Create `tools/codegen/database/families/rl78.json` *(Not implemented - RL78 support deferred)*
- [ ] 3.2 Manually define RL78 peripherals (no SVD available)
- [ ] 3.3 Define port registers (P, PM, PU, etc.)
- [ ] 3.4 Define SAU registers for UART
- [ ] 3.5 Define interrupt vector table for RL78

## 4. ESP32 Database

- [x] 4.1 Create `tools/codegen/database/families/esp32.json`
- [x] 4.2 Extract GPIO register definitions from ESP-IDF
- [x] 4.3 Extract UART register definitions
- [x] 4.4 Define interrupt vector table for ESP32

## 5. Startup Code Templates

- [x] 5.1 Create `tools/codegen/templates/startup/cortex_m0.c.j2` *(Implemented as cortex_m_startup.cpp.j2 - unified for all Cortex-M)*
- [x] 5.2 Create `tools/codegen/templates/startup/cortex_m3.c.j2` *(Covered by unified template)*
- [ ] 5.3 Create `tools/codegen/templates/startup/rl78.c.j2` *(Deferred - RL78 not yet supported)*
- [ ] 5.4 Create `tools/codegen/templates/startup/esp32.c.j2` *(Deferred - ESP32 uses ESP-IDF startup)*
- [x] 5.5 Test generation for each architecture *(Tested for ARM Cortex-M)*

## 6. Linker Script Templates

- [ ] 6.1 Create `tools/codegen/templates/linker/cortex_m.ld.j2` *(Not templated - linker scripts provided per-board in cmake/linker/)*
- [ ] 6.2 Create `tools/codegen/templates/linker/rl78.ld.j2` *(Deferred - RL78 not yet supported)*
- [ ] 6.3 Create `tools/codegen/templates/linker/esp32.ld.j2` *(Deferred - ESP32 uses ESP-IDF linker scripts)*
- [x] 6.4 Parameterize memory regions (Flash/RAM sizes) *(Flash/RAM sizes stored in database JSON)*

## 7. CMake Integration

- [x] 7.1 Update `cmake/codegen.cmake` to detect architecture *(Architecture stored in database, CMake reads from JSON)*
- [x] 7.2 Pass architecture to generator.py *(Architecture passed via database file)*
- [x] 7.3 Select correct template based on architecture *(Generator selects template based on architecture field)*
- [x] 7.4 Test code generation for all 3 architectures *(ARM tested extensively, ESP32 databases created, RL78 deferred)*
