# Proposal: ARM Cortex-M System Initialization & Configuration Framework

## Summary
Create a comprehensive, layered system initialization framework for ARM Cortex-M microcontrollers. This framework provides FPU enablement, cache configuration, MPU setup, and vendor-specific system initialization while maximizing code reuse across different ARM cores (M0/M3/M4/M7/M33) and vendors (ST, Microchip, NXP, etc).

## Motivation
Currently, the Alloy framework lacks proper system initialization beyond basic startup code:

### What We Have ✅
- ✅ `startup_common.hpp` - Runtime initialization (.data, .bss, C++ constructors)
- ✅ `startup.cpp` - Board-specific vector tables and Reset_Handler
- ✅ Linker scripts (`.ld`) - Memory layout
- ✅ Clock configuration - **Just implemented!**

### What's Missing ❌
- ❌ **FPU Configuration** - No hardware floating point enablement (M4F/M7 run in software mode)
- ❌ **Cache Configuration** - No I-Cache/D-Cache setup (M7 performance loss)
- ❌ **MPU Configuration** - No memory protection (security risk)
- ❌ **SystemInit()** - No vendor-specific initialization
- ❌ **Power/Voltage Scaling** - No optimization for performance vs power
- ❌ **Debug Configuration** - No debug interface setup
- ❌ **Layered Architecture** - Code duplicated across vendors

This leads to:
1. **Performance Loss** - FPU disabled = 10-100x slower floating point
2. **Memory Waste** - Cache disabled = slower code execution on M7
3. **Security Risk** - No MPU = no memory protection
4. **Code Duplication** - Same FPU code repeated for each vendor
5. **Hard to Maintain** - Changes require updates in multiple places

## Goals

### 1. **Layered Architecture** (Maximum Code Reuse)
Create a 3-layer system:
- **Layer 1**: Common ARM Cortex-M code (all cores)
- **Layer 2**: Core-specific code (M0/M3/M4/M7/M33)
- **Layer 3**: Vendor-specific code (ST/Microchip/NXP/etc)

### 2. **Feature Detection** (Compile-Time)
Use feature macros to detect capabilities:
```cpp
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    // Enable FPU
#endif
```

### 3. **Type-Safe Configuration** (Template-Based)
```cpp
// Configure M7 with FPU + Cache
using SystemInit = ArmCortexM7Init<
    EnableFpu::Yes,
    EnableCache::Yes,
    EnableMpu::No
>;
SystemInit::configure();
```

### 4. **Vendor Independence** (Abstract Interfaces)
Vendor code only handles vendor-specific peripherals (RCC, PMC, etc).
ARM core features (FPU, Cache, MPU) are handled by common code.

## Design

### Directory Structure
```
src/startup/
├── arm_cortex_m/                      # Layer 1: Common to ALL ARM Cortex-M
│   ├── core_common.hpp                # SCB, NVIC, SysTick (all cores)
│   ├── nvic.hpp                       # Interrupt controller
│   ├── systick.hpp                    # System tick timer
│   └── scb.hpp                        # System Control Block
│
├── arm_cortex_m0/                     # Layer 2: Cortex-M0/M0+ Specific
│   ├── cortex_m0_init.hpp             # M0 initialization
│   └── mpu_m0.hpp                     # MPU (if present)
│
├── arm_cortex_m3/                     # Layer 2: Cortex-M3 Specific
│   ├── cortex_m3_init.hpp             # M3 initialization
│   └── mpu_m3.hpp                     # MPU (8 regions)
│
├── arm_cortex_m4/                     # Layer 2: Cortex-M4/M4F Specific
│   ├── cortex_m4_init.hpp             # M4 initialization
│   ├── fpu_m4.hpp                     # FPU single precision
│   ├── dsp_m4.hpp                     # DSP instructions
│   └── mpu_m4.hpp                     # MPU (8 regions)
│
├── arm_cortex_m7/                     # Layer 2: Cortex-M7 Specific
│   ├── cortex_m7_init.hpp             # M7 initialization
│   ├── fpu_m7.hpp                     # FPU single/double precision
│   ├── cache_m7.hpp                   # I-Cache + D-Cache
│   ├── mpu_m7.hpp                     # MPU (16 regions)
│   └── tcm_m7.hpp                     # Tightly Coupled Memory
│
└── arm_cortex_m33/                    # Layer 2: Cortex-M33 Specific
    ├── cortex_m33_init.hpp            # M33 initialization
    ├── fpu_m33.hpp                    # FPU (optional)
    ├── mpu_m33.hpp                    # MPU (8/16 regions)
    ├── trustzone_m33.hpp              # TrustZone-M
    └── sau_m33.hpp                    # Security Attribution Unit

src/hal/vendors/
├── st/
│   ├── stm32f1/                       # Layer 3: ST STM32F1 (M3)
│   │   ├── system_stm32f1.hpp         # RCC, Flash, PWR
│   │   └── clock.hpp                  # ✅ Already implemented
│   ├── stm32f4/                       # Layer 3: ST STM32F4 (M4F)
│   │   ├── system_stm32f4.hpp         # RCC, Flash, PWR, Voltage Scaling
│   │   └── clock.hpp                  # ✅ Already implemented
│   └── stm32f7/                       # Layer 3: ST STM32F7 (M7)
│       ├── system_stm32f7.hpp         # RCC, Flash, PWR, Overdrive
│       └── clock.hpp                  # ✅ Already implemented
│
├── atmel/
│   └── same70/                        # Layer 3: Microchip SAME70 (M7)
│       ├── system_same70.hpp          # PMC, EFC, SUPC
│       └── clock.hpp                  # ✅ Already implemented
│
└── raspberrypi/
    └── rp2040/                        # Layer 3: RP2040 (M0+)
        ├── system_rp2040.hpp          # XOSC, PLL, RESETS
        └── clock.hpp                  # ✅ Already implemented
```

### Feature Matrix
| Feature | M0/M0+ | M3 | M4 | M4F | M7 | M33 | M55 |
|---------|--------|----|----|-----|----|----|-----|
| NVIC | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common |
| SysTick | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common | ✅ Common |
| MPU | Optional | ✅ M3 (8) | ✅ M3 (8) | ✅ M3 (8) | ✅ M7 (16) | ✅ M33 | ✅ M55 |
| FPU | ❌ | ❌ | ❌ | ✅ M4F | ✅ M7 | ✅ M33 | ✅ M55 |
| Cache | ❌ | ❌ | ❌ | ❌ | ✅ M7 | ❌ | ✅ M55 |
| DSP | ❌ | ❌ | ✅ M4 | ✅ M4 | ✅ M7 | ✅ M33 | ✅ M55 |
| TrustZone | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ M33 | ✅ M55 |
| Clock | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor |
| Flash | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor |
| Power | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor | 🟠 Vendor |

## Implementation Strategy

### Phase 1: Core Common (All ARM Cortex-M)
**Files**: `src/startup/arm_cortex_m/`
- ✅ Already have `startup_common.hpp` (data/bss/constructors)
- ➕ Add `core_common.hpp` (SCB, NVIC, SysTick registers)
- ➕ Add `nvic.hpp` (interrupt configuration)
- ➕ Add `systick.hpp` (system tick configuration)

### Phase 2: Cortex-M4 Features (FPU)
**Files**: `src/startup/arm_cortex_m4/`
- ➕ `fpu_m4.hpp` - Enable single precision FPU
- ➕ `mpu_m4.hpp` - Configure 8-region MPU
- ➕ `cortex_m4_init.hpp` - M4 initialization

### Phase 3: Cortex-M7 Features (FPU + Cache)
**Files**: `src/startup/arm_cortex_m7/`
- ➕ `fpu_m7.hpp` - Enable single/double precision FPU
- ➕ `cache_m7.hpp` - Enable I-Cache + D-Cache
- ➕ `mpu_m7.hpp` - Configure 16-region MPU
- ➕ `cortex_m7_init.hpp` - M7 initialization

### Phase 4: Vendor Integration (ST STM32F4/F7, SAME70)
**Files**: `src/hal/vendors/{vendor}/{family}/system_*.hpp`
- ➕ STM32F4: `system_stm32f4.hpp` (RCC, Flash, PWR, Voltage Scaling)
- ➕ STM32F7: `system_stm32f7.hpp` (RCC, Flash, PWR, Overdrive)
- ➕ SAME70: `system_same70.hpp` (PMC, EFC, SUPC)

### Phase 5: Update Board Startup Files
**Files**: `boards/{board}/startup.cpp`
- Update to call new `SystemInit()` function
- Enable FPU/Cache/MPU as appropriate for core

## Benefits

### Performance
- **10-100x faster** floating point math (FPU enabled)
- **2-3x faster** code execution on M7 (cache enabled)
- **Deterministic timing** with proper cache configuration

### Code Reuse
- **80% less duplication** - FPU code shared across all M4F/M7 devices
- **Easy to add new vendors** - Just implement vendor-specific parts
- **Consistent API** - Same configuration across all devices

### Security
- **Memory protection** - MPU prevents buffer overflows
- **Stack/heap separation** - MPU enforces boundaries
- **Code region protection** - Prevent code injection

### Developer Experience
- **Type-safe configuration** - Compile-time feature detection
- **Self-documenting** - Clear layering shows what's common vs specific
- **Easy to understand** - Obvious where to add new features

## Success Criteria

1. ✅ **FPU enabled** on all M4F/M7 boards
2. ✅ **Cache enabled** on all M7 boards (STM32F7, SAME70)
3. ✅ **Code reuse > 80%** - Most code shared across vendors
4. ✅ **No performance regression** - Existing code runs same or faster
5. ✅ **Compile-time feature detection** - No runtime overhead
6. ✅ **Documentation** - Each layer documented with examples

## Non-Goals
- ❌ Runtime feature detection (compile-time only)
- ❌ Support for non-ARM architectures (keep focused)
- ❌ Bootloader/secure boot (separate proposal)
- ❌ Power management APIs (separate proposal)

## Risks & Mitigation

### Risk 1: Breaking Existing Code
**Mitigation**: All changes are additive. Existing startup code continues to work.

### Risk 2: Increased Complexity
**Mitigation**: Clear layering makes it obvious what goes where. Examples provided.

### Risk 3: Vendor Differences
**Mitigation**: Layer 3 (vendor) handles all vendor-specific behavior.

## Timeline
- Phase 1 (Core Common): 2 hours
- Phase 2 (M4 Features): 3 hours
- Phase 3 (M7 Features): 4 hours
- Phase 4 (Vendor Integration): 5 hours
- Phase 5 (Board Updates): 2 hours
**Total**: ~16 hours

## Related Work
- ✅ Clock configuration (just completed!)
- 🔄 RTOS support (benefits from proper FPU/Cache setup)
- 🔄 Memory analysis (benefits from MPU configuration)
