# Alloy Implementation Roadmap

**Status**: Specifications complete for Phase 0 and Phase 1
**Total Changes**: 14 change proposals
**Total Tasks**: 257 implementation tasks

## Overview

This document provides a comprehensive roadmap for implementing the Alloy embedded framework from foundation to production-ready Phase 1.

---

## Phase 0: Foundation (Host-Only Development)

**Goal**: Validate architecture and API design without requiring hardware

### 📋 Change Proposals (9 changes, 134 tasks)

| # | Change ID | Capability | Tasks | Dependencies | Priority |
|---|-----------|------------|-------|--------------|----------|
| 1 | `add-project-structure` | Directory structure + CMake | 27 | None | 🔴 Critical |
| 2 | `add-core-error-handling` | Result<T,E> + ErrorCode | 13 | #1 | 🔴 Critical |
| 3 | `add-gpio-interface` | GPIO concepts | 9 | #1, #2 | 🟡 High |
| 4 | `add-gpio-host-impl` | GPIO host mock | 16 | #2, #3 | 🟡 High |
| 5 | `add-blinky-example` | Blinky example | 12 | #3, #4 | 🟢 Medium |
| 6 | `add-uart-interface` | UART concepts | 10 | #1, #2 | 🟡 High |
| 7 | `add-uart-host-impl` | UART host (stdin/stdout) | 12 | #2, #6 | 🟡 High |
| 8 | `add-uart-echo-example` | UART echo example | 10 | #6, #7 | 🟢 Medium |
| 9 | `add-testing-infrastructure` | Google Test + CI | 15 | #2, #4 | 🟡 High |

### Implementation Order

**Wave 1: Foundation** (Critical path - must be first)
```
1. add-project-structure (27 tasks)
   └─> Creates entire directory structure and CMake foundation

2. add-core-error-handling (13 tasks)
   └─> Result<T,E> used by all HAL APIs
```

**Wave 2: GPIO Stack** (Can parallelize)
```
3. add-gpio-interface (9 tasks)
   └─> GPIO concepts and types

4. add-gpio-host-impl (16 tasks)
   └─> Host GPIO implementation

5. add-blinky-example (12 tasks)
   └─> First end-to-end validation
```

**Wave 3: UART Stack** (Can parallelize with Wave 2)
```
6. add-uart-interface (10 tasks)
   └─> UART concepts and types

7. add-uart-host-impl (12 tasks)
   └─> Host UART implementation

8. add-uart-echo-example (10 tasks)
   └─> UART validation
```

**Wave 4: Testing** (After Wave 2 or 3)
```
9. add-testing-infrastructure (15 tasks)
   └─> Google Test + unit tests for Result and GPIO
```

### Success Criteria

- ✅ All examples compile for host target
- ✅ Blinky prints GPIO toggles to console
- ✅ UART echo works with stdin/stdout
- ✅ Unit tests pass (Google Test)
- ✅ CMake configuration is clean and documented

---

## Phase 1: Real Hardware Support

**Goal**: Deploy to actual MCUs across 3 different architectures

### 📋 Change Proposals (5 changes, 123 tasks)

| # | Change ID | Capability | Tasks | Target MCU | Priority |
|---|-----------|------------|-------|------------|----------|
| 10 | `add-rl78-hal` | Renesas RL78 HAL | 32 | RL78 (CF_RL78) | 🔴 Critical |
| 11 | `add-bluepill-hal` | STM32F103 HAL | 32 | STM32F103C8T6 | 🔴 Critical |
| 12 | `add-esp32-hal` | ESP32 HAL | 33 | ESP32 (Xtensa) | 🔴 Critical |
| 13 | `add-codegen-multiarch` | Multi-arch code generator | 26 | All | 🟡 High |
| 14 | `add-i2c-spi-interfaces` | I2C/SPI interfaces | 10 | All | 🟢 Medium |

### Architecture Comparison

| Feature | RL78 | STM32F103 (BluePill) | ESP32 |
|---------|------|----------------------|-------|
| **Architecture** | 16-bit CISC | ARM Cortex-M3 | Xtensa LX6 / RISC-V |
| **Toolchain** | GNURL78 / CC-RL | arm-none-eabi-gcc | xtensa-esp32-elf-gcc |
| **Flash** | 64KB (typical) | 64KB (C8) / 128KB (CB) | 4MB |
| **RAM** | 4KB | 20KB | 520KB |
| **Clock** | 32MHz max | 72MHz max | 240MHz max |
| **GPIO** | Port-based | Pin-based (CRL/CRH) | GPIO Matrix |
| **UART** | SAU (Serial Array Unit) | USART1/2/3 | UART0/1/2 |
| **CMSIS** | ❌ Custom headers | ✅ ST CMSIS | ❌ ESP-IDF headers |
| **FreeRTOS** | ❌ Bare-metal only | ❌ Bare-metal (optional later) | ⚠️ Recommended |

### Implementation Order

**Wave 1: Code Generator** (Foundation for all MCUs)
```
13. add-codegen-multiarch (26 tasks)
    └─> SVD parser + multi-arch templates
```

**Wave 2: MCU HALs** (Can parallelize - independent)
```
10. add-rl78-hal (32 tasks)
    └─> RL78 GPIO + UART for CF_RL78

11. add-bluepill-hal (32 tasks)
    └─> STM32F103 GPIO + UART for BluePill

12. add-esp32-hal (33 tasks)
    └─> ESP32 GPIO + UART for ESP32-DevKitC
```

**Wave 3: Advanced Peripherals** (After at least one MCU working)
```
14. add-i2c-spi-interfaces (10 tasks)
    └─> I2C and SPI concepts
```

### Hardware Requirements

**For RL78 (CF_RL78)**:
- Renesas E2 Lite debugger
- CF_RL78 board
- GNURL78 or CC-RL compiler

**For BluePill (STM32F103)**:
- ST-Link V2 programmer
- BluePill board (STM32F103C8T6)
- arm-none-eabi-gcc 11+

**For ESP32**:
- ESP32-DevKitC
- USB cable (esptool.py for flashing)
- xtensa-esp32-elf-gcc or ESP-IDF

### Success Criteria

- ✅ Blinky runs on all 3 MCUs
- ✅ UART echo works on all 3 MCUs
- ✅ Code generator handles all 3 architectures
- ✅ Each MCU has unique board definition
- ✅ All examples can be flashed to hardware
- ✅ Documentation covers flash procedures

---

## Complete Task Breakdown

### Phase 0: 134 Tasks

```
Project Structure     : 27 tasks
Error Handling        : 13 tasks
GPIO Interface        :  9 tasks
GPIO Host Impl        : 16 tasks
Blinky Example        : 12 tasks
UART Interface        : 10 tasks
UART Host Impl        : 12 tasks
UART Echo Example     : 10 tasks
Testing Infrastructure: 15 tasks
```

### Phase 1: 123 Tasks

```
RL78 HAL              : 32 tasks
BluePill HAL          : 32 tasks
ESP32 HAL             : 33 tasks
Code Generator        : 26 tasks
I2C/SPI Interfaces    : 10 tasks
```

**Grand Total: 257 tasks**

---

## Validation Commands

### View all changes
```bash
openspec list
```

### View specific change details
```bash
openspec show add-project-structure
openspec show add-rl78-hal
```

### Validate a change
```bash
openspec validate add-project-structure --strict
```

### Check implementation progress
```bash
# Tasks are in: openspec/changes/<change-id>/tasks.md
cat openspec/changes/add-project-structure/tasks.md
```

---

## Implementation Workflow

### For each change:

1. **Review**
   ```bash
   openspec show <change-id>
   ```

2. **Implement**
   - Follow `tasks.md` checklist
   - Mark tasks as `[x]` when complete
   - Commit regularly

3. **Test**
   - Build and run examples
   - Run unit tests
   - Validate on hardware (Phase 1)

4. **Archive** (when fully deployed)
   ```bash
   openspec archive <change-id> --yes
   ```

---

## Critical Path Analysis

### Blocking Dependencies

**Must complete before ANY Phase 1:**
- ✅ Phase 0 items #1-2 (project structure + error handling)
- ✅ Phase 0 items #3-4 (GPIO interface + host impl)
- ✅ Phase 0 items #6-7 (UART interface + host impl)

**Must complete before EACH MCU:**
- ✅ Phase 1 item #13 (code generator)

**Can proceed independently:**
- Each MCU HAL (#10, #11, #12) can be done in any order
- I2C/SPI interfaces (#14) can be done anytime after Phase 0

---

## Timeline Estimates

### Phase 0 (Conservative)
- **1 developer full-time**: 4-6 weeks
- **2 developers**: 3-4 weeks
- **Part-time**: 8-12 weeks

### Phase 1 (Per MCU)
- **Code generator**: 2 weeks
- **RL78 HAL**: 3-4 weeks (hardest - least documentation)
- **BluePill HAL**: 2-3 weeks (easiest - ARM + good docs)
- **ESP32 HAL**: 3-4 weeks (complex - FreeRTOS integration)

**Total Phase 1**: 8-12 weeks (if parallelized across devs)

---

## Next Steps

1. **Review specs**: Read through each `proposal.md` to understand scope
2. **Start Phase 0**: Begin with `add-project-structure`
3. **Parallel development**: Multiple devs can work on different waves
4. **Continuous validation**: Run `openspec validate --strict` regularly
5. **Document as you go**: Update tasks.md and add learnings to ADRs

---

**Last Updated**: 2025-10-29
**Status**: Ready for implementation
**Total Specs**: 14 change proposals (all validated ✅)
