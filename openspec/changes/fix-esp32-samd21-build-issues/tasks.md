# Tasks: Fix ESP32 and SAMD21 Build Issues

## Phase 1: Investigation and Root Cause Analysis

- [ ] Research ESP32 GPIO register layout from official TRM
- [ ] Check ESP-IDF headers for GPIO register definitions
- [ ] Search for ESP32 SVD files in community repositories
- [ ] Document expected vs actual GPIO register structure
- [ ] Identify xPack ARM toolchain versions affected by nosys.specs bug
- [ ] Research alternative specs options for SAMD21

## Phase 2: Fix ESP32 GPIO Peripheral Definitions

- [ ] Open and analyze `tools/codegen/database/families/espressif_esp32.json`
- [ ] Locate GPIO peripheral definition in JSON
- [ ] Add missing GPIO registers based on ESP32 TRM:
  - [ ] `GPIO_OUT_REG` (offset 0x04, GPIO0-31 output)
  - [ ] `GPIO_OUT_W1TS_REG` (offset 0x08, write 1 to set)
  - [ ] `GPIO_OUT_W1TC_REG` (offset 0x0C, write 1 to clear)
  - [ ] `GPIO_OUT1_REG` (offset 0x10, GPIO32-39 output)
  - [ ] `GPIO_OUT1_W1TS_REG` (offset 0x14)
  - [ ] `GPIO_OUT1_W1TC_REG` (offset 0x18)
  - [ ] `GPIO_ENABLE_REG` (offset 0x20, GPIO0-31 enable)
  - [ ] `GPIO_ENABLE_W1TS_REG` (offset 0x24)
  - [ ] `GPIO_ENABLE_W1TC_REG` (offset 0x28)
  - [ ] `GPIO_ENABLE1_REG` (offset 0x2C, GPIO32-39 enable)
  - [ ] `GPIO_ENABLE1_W1TS_REG` (offset 0x30)
  - [ ] `GPIO_ENABLE1_W1TC_REG` (offset 0x34)
  - [ ] `GPIO_IN_REG` (offset 0x3C, GPIO0-31 input)
  - [ ] `GPIO_IN1_REG` (offset 0x40, GPIO32-39 input)
- [ ] Verify register offsets match ESP32 TRM Section 4.10
- [ ] Regenerate ESP32 peripherals: `python3 tools/codegen/generator.py --mcu ESP32 --database tools/codegen/database/families/espressif_esp32.json --output src/generated/espressif_systems_shanghai_co_ltd/esp32/esp32`
- [ ] Verify generated `peripherals.hpp` has correct GPIO structure
- [ ] Check that no `%s` tokens remain in generated code

## Phase 3: Test ESP32 Compilation

- [ ] Source ESP-IDF toolchain: `. /Users/lgili/esp/esp-idf/export.sh`
- [ ] Configure ESP32 build: `cmake -DALLOY_BOARD=esp32_devkit -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/xtensa-esp32-elf.cmake`
- [ ] Compile ESP32 blink example: `make blink_esp32`
- [ ] Verify no compilation errors related to GPIO registers
- [ ] Verify no warnings in build output
- [ ] Check binary size is reasonable (< 100KB for blink)
- [ ] Document any remaining issues or limitations

## Phase 4: Fix SAMD21 Toolchain Issue

- [ ] Analyze current CMake toolchain configuration for ARM
- [ ] Detect xPack toolchain version in CMake
- [ ] Implement conditional specs handling:
  - [ ] Option A: Remove `-specs=nosys.specs` for xPack >= 14.x
  - [ ] Option B: Try `-specs=nano.specs` as alternative
  - [ ] Option C: Create custom syscall stubs if needed
- [ ] Update `cmake/toolchains/arm-none-eabi.cmake` with fix
- [ ] Add comments explaining the workaround
- [ ] Test that STM32F103, STM32F407, RP2040 still compile (ensure no regression)

## Phase 5: Test SAMD21 Compilation

- [ ] Configure SAMD21 build: `cmake -DALLOY_BOARD=arduino_zero`
- [ ] Compile SAMD21 blink example: `make blink_arduino_zero`
- [ ] Verify linker completes without nosys.specs error
- [ ] Verify no compilation warnings
- [ ] Check binary size is reasonable (< 10KB for blink)
- [ ] Test that other ARM boards still work after toolchain changes

## Phase 6: Full Validation

- [ ] Run automated test script: `./scripts/test-all-boards.sh`
- [ ] Verify all 5 boards compile successfully:
  - [ ] STM32F103 (Bluepill) - ~612 bytes
  - [ ] STM32F407VG (Discovery) - ~1044 bytes
  - [ ] RP2040 (Raspberry Pi Pico) - ~832 bytes
  - [ ] SAMD21 (Arduino Zero) - Expected < 10KB
  - [ ] ESP32 DevKit - Expected < 100KB
- [ ] Document final binary sizes
- [ ] Verify zero warnings across all builds
- [ ] Run any existing unit tests

## Phase 7: Documentation and Cleanup

- [ ] Update build documentation with toolchain requirements
- [ ] Document ESP32 IDF sourcing requirement
- [ ] Document xPack ARM toolchain version notes
- [ ] Add troubleshooting section for common build issues
- [ ] Update `add-multi-vendor-clock-boards` tasks.md (mark remaining tasks complete)
- [ ] Update `add-esp32-hal` tasks.md (unblock ESP32 implementation)
- [ ] Run `openspec validate fix-esp32-samd21-build-issues --strict`
- [ ] Fix any validation errors

## Phase 8: Verification and Sign-off

- [ ] Review all changes with `git diff`
- [ ] Ensure no unintended changes were introduced
- [ ] Verify regenerated files are correct
- [ ] Test clean build from scratch (remove build/ directory)
- [ ] Get approval for proposal
- [ ] Ready for implementation completion

---

**Total Tasks**: 64
**Estimated Time**: 1-2 days
**Dependencies**: ESP-IDF toolchain, xPack ARM toolchain, working code generator
**Blocks**: Full 5-board validation, example development on ESP32/SAMD21
