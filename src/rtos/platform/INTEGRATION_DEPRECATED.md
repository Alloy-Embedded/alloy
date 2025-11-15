# Platform-Specific SysTick Integration Files - DEPRECATED

## Status: OBSOLETE (Phase 4)

The files in this directory (`arm_systick_integration.cpp`, `xtensa_systick_integration.cpp`) are **DEPRECATED** as of Phase 4 and should not be used in new projects.

---

## Why Deprecated?

### Old Approach (Pre-Phase 4):
- Platform-specific SysTick integration files
- Heavy use of `#ifdef` for different MCU families
- SysTick_Handler defined in RTOS platform layer
- Separation between board code and RTOS integration
- No Result<> error handling

### New Approach (Phase 4):
- Unified SysTick_Handler in each `boards/*/board.cpp`
- Consistent pattern across all boards
- Uses Result<T, E> for error handling
- Board owns SysTick_Handler (clearer responsibility)
- RTOS is agnostic to board implementation

---

## Migration Guide

### Old Code (DEPRECATED):
```cpp
// In src/rtos/platform/arm_systick_integration.cpp
extern "C" void SysTick_Handler() {
    #if defined(STM32F4)
        stm32f4_systick_handler();
    #elif defined(STM32F7)
        stm32f7_systick_handler();
    #endif

    alloy::rtos::RTOS::tick();  // No error handling
}
```

### New Code (Phase 4):
```cpp
// In boards/*/board.cpp
extern "C" void SysTick_Handler() {
    // Update HAL tick (always)
    board::BoardSysTick::increment_tick();

    // Forward to RTOS (if enabled)
    #ifdef ALLOY_RTOS_ENABLED
        alloy::rtos::RTOS::tick().unwrap();  // Result<> error handling
    #endif
}
```

---

## Benefits of New Approach

1. **Simpler Architecture**:
   - Board owns its interrupt handlers
   - RTOS doesn't know about specific boards
   - Clear separation of concerns

2. **No Platform #ifdefs in RTOS**:
   - RTOS code is platform-agnostic
   - Board-specific code stays in board layer
   - Easier to add new boards

3. **Result<> Integration**:
   - tick() returns `Result<void, RTOSError>`
   - Errors can be handled (or unwrapped in ISR)
   - Consistent with Phase 1 error handling

4. **Unified Pattern**:
   - All 5 boards use identical SysTick_Handler structure
   - Easy to understand and maintain
   - Copy-paste friendly for new boards

---

## Removal Plan

These files will be removed in a future commit after verification that all boards work correctly with the new pattern.

**Files to Remove**:
- `src/rtos/platform/arm_systick_integration.cpp`
- `src/rtos/platform/xtensa_systick_integration.cpp`

**Timeline**: After Phase 8 (testing validation)

---

## Current Status (Phase 4)

All 5 boards updated to new pattern:
- ✅ `boards/nucleo_f401re/board.cpp`
- ✅ `boards/nucleo_f722ze/board.cpp`
- ✅ `boards/nucleo_g071rb/board.cpp`
- ✅ `boards/nucleo_g0b1re/board.cpp`
- ✅ `boards/same70_xplained/board.cpp`

Legacy integration files:
- ⚠️ `src/rtos/platform/arm_systick_integration.cpp` - DEPRECATED
- ⚠️ `src/rtos/platform/xtensa_systick_integration.cpp` - DEPRECATED

---

## For New Board Ports

When adding a new board, use this SysTick_Handler pattern in `boards/your_board/board.cpp`:

```cpp
/// SysTick Interrupt Handler
///
/// Called every 1ms by hardware SysTick timer.
/// Updates HAL tick counter and forwards to RTOS scheduler if enabled.
extern "C" void SysTick_Handler() {
    // Update HAL tick (always - required for HAL timing functions)
    board::BoardSysTick::increment_tick();

    // Forward to RTOS scheduler (if enabled at compile time)
    #ifdef ALLOY_RTOS_ENABLED
        // RTOS::tick() returns Result<void, RTOSError>
        // In ISR context, we can't handle errors gracefully, so we unwrap
        // If tick fails, it indicates a serious system error
        alloy::rtos::RTOS::tick().unwrap();
    #endif
}
```

**Do NOT** use the old platform-specific integration files.

---

## See Also

- Phase 1: Result<T,E> Integration - `docs/PHASE1_COMPLETION_SUMMARY.md`
- Phase 4: Unified SysTick Integration - `docs/PHASE4_COMPLETION_SUMMARY.md`
- RTOS Tick API: `src/rtos/rtos.hpp` - `RTOS::tick()`
