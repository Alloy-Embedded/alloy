# Fix ESP32 and SAMD21 Build Issues

## Why

Currently, only 3 out of 5 target boards compile successfully:
- ✅ STM32F103 (Bluepill) - 612 bytes
- ✅ STM32F407VG (Discovery) - 1044 bytes
- ✅ RP2040 (Raspberry Pi Pico) - 832 bytes
- ❌ **ESP32 DevKit** - GPIO peripheral structure incomplete/incorrect
- ❌ **SAMD21 (Arduino Zero)** - Toolchain nosys.specs conflict

### ESP32 Problem
The ESP32 GPIO HAL implementation (`src/hal/espressif/esp32/gpio.hpp`) expects standard GPIO registers (`OUT`, `OUT_W1TS`, `OUT_W1TC`, `ENABLE`, `ENABLE_W1TS`, `ENABLE_W1TC`, `IN`, etc.), but the generated peripheral file (`src/generated/espressif_systems_shanghai_co_ltd/esp32/esp32/peripherals.hpp`) doesn't contain these registers in the correct structure.

**Root cause**: The ESP32 JSON database (`tools/codegen/database/families/espressif_esp32.json`) either:
1. Has incomplete GPIO register definitions, OR
2. Uses a different register naming/structure than expected by the HAL

**Current error**:
```
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'ENABLE_W1TC'
error: 'struct alloy::generated::esp32::gpio::Registers' has no member named 'OUT_W1TS'
```

### SAMD21 Problem
The SAMD21 (Arduino Zero) fails at link time with a nosys.specs conflict when using the xPack ARM toolchain:

**Current error**:
```
arm-none-eabi-g++: fatal error: nosys.specs: attempt to rename spec 'link_gcc_c_sequence'
to already defined spec 'nosys_link_gcc_c_sequence'
```

**Root cause**: The xPack ARM toolchain (v14.2.1) has a known issue where specifying `-specs=nosys.specs` conflicts with internal specs. This affects SAMD21 but not other ARM boards.

## What Changes

### 1. Fix ESP32 GPIO Peripheral Definitions

**Option A: Fix the JSON database** (Preferred)
- Investigate `tools/codegen/database/families/espressif_esp32.json`
- Add/correct GPIO register definitions to match ESP32 TRM (Technical Reference Manual)
- Ensure registers include:
  - `GPIO_OUT_REG` (GPIO0-31 output value)
  - `GPIO_OUT_W1TS_REG` (Write 1 to set)
  - `GPIO_OUT_W1TC_REG` (Write 1 to clear)
  - `GPIO_OUT1_REG` (GPIO32-39 output value)
  - `GPIO_OUT1_W1TS_REG` / `GPIO_OUT1_W1TC_REG`
  - `GPIO_ENABLE_REG` / `GPIO_ENABLE_W1TS_REG` / `GPIO_ENABLE_W1TC_REG`
  - `GPIO_ENABLE1_REG` / `GPIO_ENABLE1_W1TS_REG` / `GPIO_ENABLE1_W1TC_REG`
  - `GPIO_IN_REG` / `GPIO_IN1_REG`
- Regenerate ESP32 peripherals with fixed database
- Verify HAL compiles with corrected peripheral definitions

**Option B: Adapt the HAL to current structure**
- Modify `src/hal/espressif/esp32/gpio.hpp` to work with actual generated structure
- Map HAL expectations to whatever registers are available
- Less preferred as it makes HAL inconsistent with ESP32 documentation

**Decision**: Option A - Fix at the source (JSON database)

### 2. Fix SAMD21 Toolchain Issue

**Option A: Remove `-specs=nosys.specs` for xPack toolchain** (Quick fix)
- Detect xPack toolchain version in CMake
- Conditionally remove `-specs=nosys.specs` for affected versions
- Add comment explaining the workaround

**Option B: Use different specs file**
- Try `-specs=nano.specs` instead
- Provides minimal newlib implementation without syscall stubs

**Option C: Provide custom syscall stubs**
- Create minimal `_exit`, `_sbrk`, `_kill`, `_getpid` stubs
- Don't use `-specs=nosys.specs` at all
- More control but requires maintaining syscall implementations

**Decision**: Start with Option A (quick fix), escalate to Option C if needed

## Impact

### Benefits
- ✅ All 5 target boards compile successfully
- ✅ Validates HAL abstraction across 5 different MCUs/architectures
- ✅ ESP32 GPIO registers match official documentation
- ✅ SAMD21 builds without toolchain hacks
- ✅ Unblocks example development on ESP32 and SAMD21

### Risks
- ⚠️ ESP32 JSON fix might break other ESP32 variants (ESP32-S2, ESP32-C3, etc.)
- ⚠️ SAMD21 fix might be toolchain version-specific
- ⚠️ Regenerating peripherals might introduce other issues

### Mitigation
- Test all ESP32 variants after JSON changes
- Document toolchain version requirements
- Run full test suite after regeneration

## Affected Specs

No new specs - this is a bug fix for existing functionality:
- `add-multi-vendor-clock-boards` (completing remaining tasks)
- `add-esp32-hal` (unblocking implementation)

## Affected Code

### ESP32
- `tools/codegen/database/families/espressif_esp32.json` - Add GPIO registers
- `src/generated/espressif_systems_shanghai_co_ltd/esp32/esp32/peripherals.hpp` - Regenerated
- `src/hal/espressif/esp32/gpio.hpp` - Already correct, should work after regen

### SAMD21
- `cmake/toolchains/arm-none-eabi.cmake` - Conditional specs handling
- `cmake/boards/arduino_zero.cmake` - Board-specific flags if needed
- No source code changes required

## Dependencies

### Required
- Existing code generator (`tools/codegen/generator.py`) - ✅ Working (fixed %s bug)
- ESP32 Technical Reference Manual - For GPIO register reference
- xPack ARM toolchain v14.2.1+ - Already installed

### Blocks
- ESP32 blink example validation
- SAMD21 blink example validation
- Full 5-board test suite completion
- Archiving `add-multi-vendor-clock-boards` change

## Alternatives Considered

### Alternative 1: Mark ESP32/SAMD21 as "not supported"
❌ **Rejected**: These are popular platforms, should be supported

### Alternative 2: Use vendor HALs (ESP-IDF, SAMD21 ASF)
❌ **Rejected**: Defeats purpose of creating minimal HAL abstraction

### Alternative 3: Current approach (Fix underlying issues)
✅ **Selected**: Proper fix at root cause, enables long-term support

## Open Questions

1. **ESP32 GPIO Register Layout**
   - Are there existing ESP32 SVD files we can reference?
   - **Action**: Check ESP-IDF, svd2rust community for ESP32 SVD

2. **SAMD21 Toolchain Compatibility**
   - Which xPack versions are affected?
   - **Action**: Test with multiple xPack versions if available

3. **Other ESP32 Variants**
   - Will GPIO fix apply to ESP32-S2, ESP32-C3, etc.?
   - **Action**: Review JSON for other variants, apply similar fixes

## Success Criteria

- [ ] ESP32 GPIO registers defined correctly in JSON database
- [ ] ESP32 peripherals regenerated with complete GPIO structure
- [ ] ESP32 blink example compiles without errors
- [ ] ESP32 blink example produces correct binary size
- [ ] SAMD21 linker completes without nosys.specs error
- [ ] SAMD21 blink example compiles and links successfully
- [ ] SAMD21 blink example produces correct binary size
- [ ] All 5 boards show in build test summary as successful
- [ ] Zero compilation warnings on all boards
- [ ] Updated documentation reflects any toolchain requirements

## Timeline

**Estimated effort**: 1-2 days

### Day 1: Investigation and ESP32 Fix
- [ ] 1-2h: Research ESP32 GPIO registers (TRM, ESP-IDF headers, SVD files)
- [ ] 1-2h: Update `espressif_esp32.json` with correct GPIO registers
- [ ] 30m: Regenerate ESP32 peripherals
- [ ] 30m: Test ESP32 compilation
- [ ] 1h: Debug any remaining issues

### Day 2: SAMD21 Fix and Validation
- [ ] 1h: Implement SAMD21 toolchain workaround
- [ ] 30m: Test SAMD21 compilation
- [ ] 1h: Run full 5-board test suite
- [ ] 1h: Document fixes and toolchain requirements
- [ ] 30m: Update related OpenSpec tasks

## Related Changes

- `add-multi-vendor-clock-boards` - Parent change (124/133 tasks, needs completion)
- `add-esp32-hal` - Currently blocked by this issue (0/44 tasks)
- `add-codegen-foundation` - Code generator already working (fixed %s tokens)
