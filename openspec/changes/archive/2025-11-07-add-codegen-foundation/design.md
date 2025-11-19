## Architectural Overview

### System Components

```
┌─────────────────────────────────────────────────────────┐
│  ARM CMSIS-Pack Repository (GitHub)                     │
│  https://github.com/cmsis-packs                         │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  tools/codegen/sync_svd.py                              │
│  - Clones/updates CMSIS repos                           │
│  - Organizes by vendor: STM32/, nRF/, LPC/, etc        │
│  - Symlinks to tools/codegen/database/svd/             │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  tools/codegen/database/svd/                            │
│  ├── STM32/                                             │
│  │   ├── STM32F103.svd                                  │
│  │   ├── STM32F446.svd                                  │
│  │   └── ...                                            │
│  ├── nRF/                                               │
│  └── ...                                                │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  tools/codegen/svd_parser.py                            │
│  - Parses SVD XML → JSON                                │
│  - Normalizes vendor differences                        │
│  - Validates required fields                            │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  tools/codegen/database/families/                       │
│  ├── stm32f1xx.json   (multiple MCUs)                   │
│  ├── stm32f4xx.json                                     │
│  └── nrf52.json                                         │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  User CMakeLists.txt                                    │
│  set(ALLOY_MCU "STM32F103C8")                           │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  cmake/codegen.cmake                                    │
│  - Detects MCU from ALLOY_MCU                           │
│  - Checks if code generation needed                     │
│  - Invokes generator.py if needed                       │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  tools/codegen/generator.py                             │
│  - Loads JSON database                                  │
│  - Renders Jinja2 templates                             │
│  - Writes to build/generated/{MCU}/                     │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  build/generated/STM32F103C8/                           │
│  ├── startup.cpp        (Reset handler, .data/.bss)     │
│  ├── vectors.cpp        (Interrupt vector table)        │
│  ├── registers.hpp      (Peripheral structs)            │
│  └── stm32f103c8.ld     (Linker script)                 │
└────────────────┬────────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────────┐
│  CMake Build                                            │
│  - Compiles generated + HAL + user code                 │
│  - Links with generated linker script                   │
└─────────────────────────────────────────────────────────┘
```

### Design Decisions

#### 1. SVD Source Strategy

**Decision:** Clone CMSIS-Pack repos as git submodules, symlink SVDs to organized structure

**Alternatives considered:**
| Approach | Pros | Cons | Decision |
|----------|------|------|----------|
| Download individual SVDs | Simple, small repo | Manual updates, no version control | ❌ Rejected |
| Full CMSIS clone | Official source | Huge repo (~1GB+), slow | ⚠️ Submodule only |
| Vendor websites | Most up-to-date | Inconsistent formats, manual | ❌ Rejected |
| **Git submodule + organize** | Version controlled, automated sync | Initial setup complexity | ✅ **Chosen** |

**Implementation:**
```bash
# .gitmodules
[submodule "tools/codegen/upstream/cmsis-svd"]
    path = tools/codegen/upstream/cmsis-svd
    url = https://github.com/posborne/cmsis-svd.git
    shallow = true

[submodule "tools/codegen/upstream/cmsis-pack-eclipse"]
    path = tools/codegen/upstream/cmsis-pack-eclipse
    url = https://github.com/ARM-software/CMSIS_5.git
    shallow = true
```

Then `sync_svd.py` creates organized symlink structure:
```
database/svd/
├── STM32/ -> ../../upstream/cmsis-svd/data/STMicro/
├── nRF/ -> ../../upstream/cmsis-svd/data/Nordic/
└── ...
```

#### 2. JSON Database Format

**Decision:** Use normalized JSON intermediate format, not SVD directly

**Rationale:**
- SVD XML is complex and inconsistent across vendors
- JSON easier to parse in Python/CMake
- Allows manual databases for non-SVD MCUs (RL78, ESP32)
- Enables vendor-specific normalization

**Schema example:**
```json
{
  "family": "STM32F1",
  "architecture": "arm-cortex-m3",
  "vendor": "ST",
  "mcus": {
    "STM32F103C8": {
      "flash": {
        "size_kb": 64,
        "base_address": "0x08000000",
        "page_size_kb": 1
      },
      "ram": {
        "size_kb": 20,
        "base_address": "0x20000000"
      },
      "peripherals": {
        "GPIO": {
          "instances": [
            {"name": "GPIOA", "base": "0x40010800"},
            {"name": "GPIOB", "base": "0x40010C00"},
            {"name": "GPIOC", "base": "0x40011000"}
          ],
          "registers": {
            "CRL":  {"offset": "0x00", "size": 32},
            "CRH":  {"offset": "0x04", "size": 32},
            "IDR":  {"offset": "0x08", "size": 32},
            "ODR":  {"offset": "0x0C", "size": 32},
            "BSRR": {"offset": "0x10", "size": 32},
            "BRR":  {"offset": "0x14", "size": 32}
          }
        }
      },
      "interrupts": {
        "count": 68,
        "vectors": [
          {"number": 0, "name": "Initial_SP"},
          {"number": 1, "name": "Reset_Handler"},
          {"number": 2, "name": "NMI_Handler"},
          {"number": 3, "name": "HardFault_Handler"},
          {"number": 11, "name": "SVC_Handler"},
          {"number": 14, "name": "PendSV_Handler"},
          {"number": 15, "name": "SysTick_Handler"},
          {"number": 16, "name": "WWDG_IRQHandler"},
          {"number": 23, "name": "EXTI0_IRQHandler"}
        ]
      }
    }
  }
}
```

#### 3. Template Technology

**Decision:** Jinja2 templates for code generation

**Alternatives:**
| Technology | Pros | Cons | Decision |
|------------|------|------|----------|
| String interpolation | Simple | Unreadable for complex code | ❌ |
| Mako | Powerful | Less common, Python-specific | ❌ |
| **Jinja2** | Industry standard, readable, powerful | Requires Python | ✅ **Chosen** |
| m4 macros | C-friendly | Cryptic syntax | ❌ |

**Template structure:**
```
tools/codegen/templates/
├── common/
│   ├── header.j2           # Standard file header
│   └── macros.j2           # Reusable macros
│
├── startup/
│   └── cortex_m_startup.cpp.j2
│
├── registers/
│   └── peripheral_struct.hpp.j2
│
└── linker/
    └── cortex_m.ld.j2
```

**Example template snippet:**
```jinja2
{# templates/startup/cortex_m_startup.cpp.j2 #}
/// Auto-generated startup code for {{ mcu.name }}
/// DO NOT EDIT - Generated from {{ source_file }}

extern "C" [[noreturn]] void Reset_Handler() {
    // Copy .data from Flash to RAM
    uint32_t* src = &_sidata;
    uint32_t* dest = &_sdata;
    while (dest < &_edata) {
        *dest++ = *src++;
    }

    // Zero .bss
    dest = &_sbss;
    while (dest < &_ebss) {
        *dest++ = 0;
    }

    // Call main
    main();

    while (true) { __asm__("wfi"); }
}
```

#### 4. CMake Integration

**Decision:** Transparent automatic generation during CMake configure

**Flow:**
1. User sets `ALLOY_MCU` in CMakeLists.txt
2. CMake loads `cmake/codegen.cmake`
3. Check if `build/generated/${ALLOY_MCU}/.generated` marker exists
4. If not, or if database is newer, run `generator.py`
5. Add generated files to build
6. Create marker file

**Key functions:**
```cmake
function(alloy_generate_code)
    # Check if generation needed
    if(NOT EXISTS ${MARKER_FILE} OR
       ${DATABASE_FILE} IS_NEWER_THAN ${MARKER_FILE})

        execute_process(
            COMMAND ${Python3_EXECUTABLE}
                ${GENERATOR_SCRIPT}
                --mcu ${ALLOY_MCU}
                --output ${OUTPUT_DIR}
            RESULT_VARIABLE result
        )

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Code generation failed")
        endif()

        file(TOUCH ${MARKER_FILE})
    endif()
endfunction()
```

#### 5. Directory Organization

```
tools/codegen/
├── sync_svd.py              # SVD downloader/sync script
├── svd_parser.py            # SVD XML → JSON converter
├── generator.py             # Main code generator
├── validate_database.py     # Database schema validator
│
├── database/
│   ├── svd/                 # Organized symlinks to upstream SVDs
│   │   ├── STM32/           # -> ../../upstream/cmsis-svd/data/STMicro/
│   │   ├── nRF/             # -> ../../upstream/cmsis-svd/data/Nordic/
│   │   └── ...
│   │
│   └── families/            # Normalized JSON databases
│       ├── stm32f1xx.json   # STM32F1 family
│       ├── stm32f4xx.json
│       └── nrf52.json
│
├── templates/               # Jinja2 templates
│   ├── common/
│   ├── startup/
│   ├── registers/
│   └── linker/
│
├── upstream/                # Git submodules (gitignored binaries)
│   └── cmsis-svd/           # Git submodule
│
└── tests/                   # Generator tests
    ├── test_parser.py
    ├── test_generator.py
    └── fixtures/
        └── test_svd.xml
```

### Performance Considerations

**Code generation time budget:**
- Parse SVD: < 2 seconds
- Generate all files: < 1 second
- Total CMake overhead: < 5 seconds (first time)
- Incremental (no changes): < 100ms (marker check)

**Optimization strategies:**
- Cache parsed JSON (don't re-parse SVD every time)
- Parallel template rendering (if needed later)
- Incremental generation (only changed MCUs)

### Testing Strategy

**Unit tests:**
- `test_svd_parser.py` - Validate parsing with fixture SVD
- `test_generator.py` - Test template rendering
- `test_database_schema.py` - Validate JSON structure

**Integration tests:**
- Generate code for STM32F103
- Compile generated code
- Verify symbols in output binary
- Compare generated vs reference code

**Validation:**
```bash
# Validate all databases
python tools/codegen/validate_database.py database/families/*.json

# Test generation for all MCUs
python tools/codegen/test_all_mcus.py

# Verify generated code compiles
cmake -B build -DALLOY_MCU=STM32F103C8
cmake --build build
```

### Future Extensibility

**Designed for:**
- Multiple architectures (ARM, RL78, Xtensa)
- Multiple output languages (C++, C, Rust?)
- Custom generators (peripheral drivers, HAL)
- IDE integration (VSCode extension?)

**Not in MVP:**
- Clock configuration generator (Phase 2)
- Pin mux generator (Phase 2)
- DMA configuration (Phase 2)
- Custom peripheral drivers (Phase 3)
