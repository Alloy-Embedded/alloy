# Tasks: ARM Cortex-M System Initialization

## Phase 1: Core Common (All ARM Cortex-M)
**Estimated Time**: 2 hours

- [ ] Create `src/startup/arm_cortex_m/core_common.hpp`
  - [ ] Define SCB registers and CPACR bits
  - [ ] Define NVIC registers
  - [ ] Define SysTick registers
  - [ ] Add DSB/ISB/DMB/WFI/WFE barrier functions

- [ ] Create `src/startup/arm_cortex_m/nvic.hpp`
  - [ ] Implement `enable_irq()`, `disable_irq()`
  - [ ] Implement `set_priority()`, `get_priority()`
  - [ ] Implement `set_pending()`, `clear_pending()`
  - [ ] Implement `is_active()`

- [ ] Create `src/startup/arm_cortex_m/systick.hpp`
  - [ ] Implement `configure()` with ticks and interrupt enable
  - [ ] Implement `disable()`
  - [ ] Implement `get_value()`, `has_counted_to_zero()`

## Phase 2: Cortex-M4 Features (FPU)
**Estimated Time**: 3 hours

- [ ] Create `src/startup/arm_cortex_m4/fpu_m4.hpp`
  - [ ] Implement `enable_fpu()` - Set CPACR, DSB, ISB
  - [ ] Implement `disable_fpu()`
  - [ ] Implement `is_fpu_enabled()`
  - [ ] Define FPCCR registers
  - [ ] Implement `configure_lazy_stacking()`

- [ ] Create `src/startup/arm_cortex_m4/cortex_m4_init.hpp`
  - [ ] Implement `init()` - Call enable_fpu() and configure_lazy_stacking()
  - [ ] Add compile-time checks for __FPU_PRESENT

- [ ] Update CMake flags for STM32F4 boards
  - [ ] Add `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`
  - [ ] Define `__FPU_PRESENT=1 __FPU_USED=1`

## Phase 3: Cortex-M7 Features (FPU + Cache)
**Estimated Time**: 4 hours

- [ ] Create `src/startup/arm_cortex_m7/fpu_m7.hpp`
  - [ ] Copy from M4 (same implementation for M7)
  - [ ] Update comments for double precision support

- [ ] Create `src/startup/arm_cortex_m7/cache_m7.hpp`
  - [ ] Define Cache registers (ICIALLU, DCISW, etc)
  - [ ] Implement `invalidate_icache()`, `enable_icache()`, `disable_icache()`
  - [ ] Implement `invalidate_dcache()`, `enable_dcache()`, `disable_dcache()`
  - [ ] Implement `clean_dcache()` for DMA

- [ ] Create `src/startup/arm_cortex_m7/cortex_m7_init.hpp`
  - [ ] Implement `init()` - FPU + I-Cache + D-Cache
  - [ ] Add compile-time checks for __ICACHE_PRESENT, __DCACHE_PRESENT

- [ ] Update CMake flags for STM32F7/SAME70 boards
  - [ ] Add `-mfpu=fpv5-d16 -mfloat-abi=hard`
  - [ ] Define `__FPU_PRESENT=1 __FPU_USED=1 __ICACHE_PRESENT=1 __DCACHE_PRESENT=1`

## Phase 4: Vendor Integration (ST STM32)
**Estimated Time**: 5 hours

- [ ] Create `src/hal/vendors/st/stm32f1/system_stm32f1.hpp`
  - [ ] SystemInit() function
  - [ ] Call clock configuration
  - [ ] Call cortex_m3::init() (no FPU)

- [ ] Create `src/hal/vendors/st/stm32f4/system_stm32f4.hpp`
  - [ ] SystemInit() function
  - [ ] Configure voltage scaling (PWR_CR VOS bits)
  - [ ] Call clock configuration
  - [ ] Call cortex_m4::init() (enable FPU)

- [ ] Create `src/hal/vendors/st/stm32f7/system_stm32f7.hpp`
  - [ ] SystemInit() function
  - [ ] Configure voltage scaling + overdrive
  - [ ] Call clock configuration
  - [ ] Call cortex_m7::init() (enable FPU + Cache)

- [ ] Create `src/hal/vendors/atmel/same70/system_same70.hpp`
  - [ ] SystemInit() function
  - [ ] Configure PMC, EFC
  - [ ] Call clock configuration
  - [ ] Call cortex_m7::init() (enable FPU + Cache)

## Phase 5: Update Board Startup Files
**Estimated Time**: 2 hours

- [ ] Update `boards/stm32f103c8/startup.cpp`
  - [ ] Include `hal/vendors/st/stm32f1/system_stm32f1.hpp`
  - [ ] Call `SystemInit()` in Reset_Handler

- [ ] Update `boards/stm32f407vg/startup.cpp`
  - [ ] Include `hal/vendors/st/stm32f4/system_stm32f4.hpp`
  - [ ] Call `SystemInit()` in Reset_Handler

- [ ] Update `boards/atmel_same70_xpld/startup.cpp`
  - [ ] Include `hal/vendors/atmel/same70/system_same70.hpp`
  - [ ] Call `SystemInit()` in Reset_Handler

## Phase 6: Testing & Validation
**Estimated Time**: 3 hours

- [ ] Test STM32F4 FPU enabled
  - [ ] Write test with floating point math
  - [ ] Check disassembly uses VFPU instructions (not software emulation)
  - [ ] Benchmark performance vs software float

- [ ] Test STM32F7 FPU + Cache enabled
  - [ ] Verify CCR register bits set
  - [ ] Benchmark with/without cache
  - [ ] Test DMA + D-Cache interaction

- [ ] Test SAME70 FPU + Cache enabled
  - [ ] Same tests as STM32F7

- [ ] Verify backward compatibility
  - [ ] Existing examples still build and run
  - [ ] No performance regressions

## Total Estimated Time: ~19 hours

## Success Criteria

- ✅ FPU enabled on all M4F/M7 boards (STM32F4, STM32F7, SAME70)
- ✅ Cache enabled on all M7 boards (STM32F7, SAME70)
- ✅ Floating point operations 10-100x faster (hardware vs software)
- ✅ Code execution 2-3x faster with I-Cache on M7
- ✅ All existing examples compile and run
- ✅ No breaking changes to existing code
- ✅ Clear documentation in each header file

## Dependencies

- ✅ Clock configuration (already implemented)
- ✅ startup_common.hpp (already exists)
- ⏳ CMake updates (Phase 2-3)
- ⏳ Board startup updates (Phase 5)

## Risks

**Risk**: Cache + DMA interaction issues on M7
**Mitigation**: Provide `clean_dcache()` function, document DMA usage

**Risk**: Breaking existing code
**Mitigation**: All changes are additive, existing startup still works

**Risk**: Wrong FPU flags cause hard fault
**Mitigation**: Compile-time checks with `__FPU_PRESENT`, clear error messages
