# Proposal: Add SysTick Timer HAL

## Summary
Implement a global, high-precision system timer (SysTick) interface and implementations for all 5 supported MCU families. This provides a unified, zero-overhead time tracking mechanism that serves as the foundation for the future Alloy RTOS.

## Motivation
Currently, Alloy has delay functions but no global time tracking. This creates several problems:
1. **No way to measure elapsed time** - Users can't timestamp events or measure durations
2. **No foundation for RTOS** - A preemptive scheduler requires precise time tracking
3. **Inconsistent timing APIs** - Each vendor has different timer peripherals and APIs
4. **No timeout mechanisms** - Can't implement timeouts for blocking operations

A unified SysTick interface solves these problems with:
- **Global time counter** - Always available, microsecond precision
- **Zero overhead** - Uses hardware timer, no polling needed
- **Consistent API** - Same interface across all MCU families
- **RTOS ready** - Designed as the heartbeat for future scheduler
- **Compile-time safety** - All configuration validated at compile time

## Goals
1. Define a clean, modern C++20 SysTick interface
2. Implement SysTick for all 5 MCU families (STM32F1, STM32F4, ESP32, RP2040, SAMD21)
3. Provide both namespace-level and object-oriented APIs
4. Auto-initialize during `Board::initialize()` with sane defaults
5. Allow reconfiguration for advanced users
6. Maintain microsecond precision with 32-bit counter (71 minute overflow)
7. Zero memory overhead when not used (compile-time elimination)
8. Thread-safe access to time counter (atomic reads)

## Non-Goals
1. **Not adding callback support** - Callbacks handled by future RTOS, not SysTick
2. **Not implementing multiple timers** - Single global systick only
3. **Not adding delay functions** - Already exist, separate concern
4. **Not supporting 64-bit counters** - 32-bit sufficient, saves memory
5. **Not implementing timer channels** - General-purpose timers are separate

## Success Criteria
- [x] All 5 MCU families have SysTick implementation
- [x] Consistent API: `alloy::systick::micros()` works on all platforms
- [x] Auto-initialized during `Board::initialize()`
- [x] Microsecond resolution with accurate timing
- [x] Zero overhead when not called (compiler eliminates unused code)
- [x] Thread-safe reads (atomic operations)
- [x] All examples compile and run with SysTick available
- [x] Documentation covers all APIs and usage patterns

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Timer peripheral conflicts | High - breaks existing code | Use dedicated SysTick peripheral (ARM) or reserved timer |
| Interrupt overhead | Medium - affects performance | Minimize ISR work, just increment counter |
| Overflow handling | Low - app crashes | Document 71min limit, provide `micros_since()` helper |
| Platform differences | Medium - inconsistent behavior | Hide differences behind interface, validate all platforms |
| Initialization order | Medium - timer not ready | Initialize early in `Board::initialize()`, before peripherals |

## Dependencies
- **Builds on**: `add-multi-vendor-clock-boards` (needs clock frequency for timer config)
- **Required by**: Future `add-alloy-rtos` (scheduler needs systick)
- **Blocks**: None (standalone feature)

## Timeline
- **Design & Spec**: 1 day (this proposal)
- **Implementation**: 2-3 days (interface + 5 MCUs)
- **Testing & Validation**: 1 day
- **Total**: 4-5 days

## Alternatives Considered

### 1. Use Generic Timer Interface Instead of Dedicated SysTick
**Rejected**: Generic timers are for general-purpose timing (PWM, input capture, etc.). SysTick is special:
- Always available (dedicated peripheral on ARM)
- Simpler API (just time tracking)
- Lower overhead (no mode switching, no callbacks)
- Better separation of concerns

### 2. Implement as Part of Clock HAL
**Rejected**: Clock HAL manages frequencies, not time tracking. Separate SysTick:
- Single responsibility principle
- Clock can be reconfigured without affecting time tracking
- Clearer API boundaries

### 3. Use 64-bit Counter to Avoid Overflow
**Rejected**:
- 8 bytes instead of 4 bytes (wastes memory on constrained MCUs)
- Atomic 64-bit reads require expensive locking on 32-bit MCUs
- 71 minutes is sufficient for most use cases
- Users can handle overflow with `micros_since()` helper

### 4. Add Millisecond Counter Too
**Rejected**:
- Adds complexity without significant benefit
- Users can divide by 1000 if needed
- Microsecond precision covers both use cases
- Saves 4 bytes of RAM

## Open Questions
None - all design decisions made based on user feedback.

## References
- ARM Cortex-M SysTick: https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-timer--systick
- ESP32 Timer: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html
- RP2040 Timer: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf (Section 4.6)
- SAMD21 TC: https://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D21_DA1_Family_DataSheet_DS40001882F.pdf
