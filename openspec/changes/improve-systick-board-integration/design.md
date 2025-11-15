# SysTick Board Integration Design

## Context

The SysTick timer is a critical component in ARM Cortex-M systems, providing:
- System tick for RTOS scheduling
- Accurate timing and delays
- Performance measurement
- Timeout handling

Currently, each board implements SysTick independently without a standardized pattern, making it difficult to write portable timing code.

### Constraints
- SysTick is a 24-bit down-counter (max value: 16,777,215)
- Clock source is typically CPU clock or CPU clock / 8
- Overflow occurs when counter reaches zero
- Interrupt fires on overflow
- ARM Cortex-M specific (not available on Xtensa/ESP32)

### Stakeholders
- Embedded developers writing timing-sensitive code
- RTOS developers requiring precise tick timing
- Board support package maintainers
- Example code users learning the framework

## Goals / Non-Goals

### Goals
1. **Standardize** SysTick integration pattern across all ARM boards
2. **Provide** compile-time safety for clock configuration
3. **Create** comprehensive examples demonstrating timing patterns
4. **Document** best practices for SysTick usage
5. **Enable** RTOS integration with minimal overhead
6. **Ensure** timing accuracy within ±1% on all boards

### Non-Goals
1. **Not** creating a new abstraction layer (use existing Platform Layer)
2. **Not** supporting non-ARM architectures (ESP32 uses different timer)
3. **Not** implementing software timers (future enhancement)
4. **Not** changing existing HAL APIs (maintain compatibility)

## Decisions

### Decision 1: Standard BoardSysTick Type Pattern

**Choice**: Each board defines a type alias using platform-specific SysTick template:

```cpp
// In board_config.hpp or board.hpp
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;
```

**Rationale**:
- ✅ Type-safe: Clock frequency baked into type
- ✅ Compile-time validation: Invalid frequencies caught early
- ✅ Zero overhead: Template instantiation optimizes to direct register access
- ✅ Portable: Same API across all boards

**Alternatives Considered**:
- ❌ Runtime configuration: Loses compile-time safety
- ❌ Macro-based: Loses type safety and error checking
- ❌ Virtual interface: Adds runtime overhead (vtable)

### Decision 2: Unified Interrupt Handler Pattern

**Choice**: Standardized SysTick_Handler in each board.cpp:

```cpp
extern "C" void SysTick_Handler() {
    board::BoardSysTick::increment_tick();

    #ifdef ALLOY_RTOS_ENABLED
        RTOS::tick();  // Optional: RTOS scheduler tick
    #endif
}
```

**Rationale**:
- ✅ Clear: Single responsibility per handler
- ✅ Extensible: Easy to add RTOS integration
- ✅ Documented: Consistent pattern across boards
- ✅ Testable: Can be mocked/validated

**Alternatives Considered**:
- ❌ Weak symbol override: Less explicit, harder to find
- ❌ Callback registration: Runtime overhead
- ❌ Header-only: Cannot override extern "C" function

### Decision 3: Tick Resolution Configuration

**Choice**: Support multiple tick resolutions via init_ms()/init_us():

```cpp
// 1ms tick (standard for RTOS)
SysTickTimer::init_ms<BoardSysTick>(1);

// 100us tick (high-resolution timing)
SysTickTimer::init_us<BoardSysTick>(100);
```

**Rationale**:
- ✅ Flexible: Choose resolution based on use case
- ✅ Validated: Compile-time checks for valid periods
- ✅ Documented: Clear API showing units
- ✅ Efficient: Only one resolution active at a time

**Trade-offs**:
- 1ms tick: Lower interrupt rate, ±1ms accuracy
- 100us tick: Higher interrupt rate, ±100us accuracy
- Both: Validated at compile-time against 24-bit counter limits

### Decision 4: Example Structure

**Choice**: Create separate examples for different use cases:

```
examples/
├── timing/
│   ├── basic_delays/         # Blocking delays (ms/us)
│   ├── timeout_patterns/     # Non-blocking timeouts
│   └── performance/          # Execution time measurement
└── systick_demo/             # SysTick feature showcase
```

**Rationale**:
- ✅ Focused: Each example teaches one concept
- ✅ Progressive: Start simple, add complexity
- ✅ Reusable: Copy-paste patterns into projects
- ✅ Educational: Comments explain design decisions

**Alternatives Considered**:
- ❌ Single mega-example: Too complex for learning
- ❌ README-only: Code is better than documentation
- ❌ Test code as examples: Tests validate, examples teach

### Decision 5: Compile-Time Validation

**Choice**: Use static_assert for configuration validation:

```cpp
template <u32 CLOCK_HZ>
class SysTick {
    // Validate clock frequency is reasonable
    static_assert(CLOCK_HZ >= 1'000'000,
                  "Clock frequency too low for accurate timing");
    static_assert(CLOCK_HZ <= 400'000'000,
                  "Clock frequency exceeds ARM spec");

    // Validate requested period fits in 24-bit counter
    static constexpr bool is_valid_period_ms(u32 ms) {
        u64 reload = (static_cast<u64>(CLOCK_HZ) / 1000) * ms;
        return reload <= 0xFFFFFF;
    }
};
```

**Rationale**:
- ✅ Safe: Catches errors before deployment
- ✅ Clear: Error messages guide fixes
- ✅ Zero-cost: No runtime checking needed
- ✅ Educational: Shows constraints explicitly

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────┐
│  Application / Examples                          │
│  - Basic delays                                  │
│  - Timeout patterns                              │
│  - Performance measurement                       │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│  HAL API Layer (systick_simple.hpp)             │
│  - init_ms<SysTickImpl>()                       │
│  - delay_ms<SysTickImpl>()                      │
│  - millis<SysTickImpl>()                        │
│  - micros<SysTickImpl>()                        │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│  Board Layer (board.cpp)                        │
│  - using BoardSysTick = SysTick<CLOCK_HZ>       │
│  - SysTick_Handler() { increment_tick(); }      │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│  Platform Layer (systick_platform.hpp)          │
│  - template <u32 CLOCK_HZ> class SysTick        │
│  - Direct register access                       │
│  - Compile-time validation                      │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│  Hardware (ARM Cortex-M SysTick)                │
│  - 24-bit down-counter                          │
│  - Memory-mapped registers                      │
└─────────────────────────────────────────────────┘
```

### Data Flow

**Initialization:**
1. board::init() calls SysTickTimer::init_ms<BoardSysTick>(1)
2. BoardSysTick validates clock frequency and period at compile-time
3. Platform layer calculates reload value and configures registers
4. SysTick interrupt enabled

**Runtime Timing:**
1. Application calls SysTickTimer::millis<BoardSysTick>()
2. API layer delegates to BoardSysTick::millis()
3. Platform layer reads tick counter (volatile read)
4. Return value (zero overhead delegation)

**Interrupt Flow:**
1. SysTick counter reaches zero
2. Hardware triggers SysTick_Handler()
3. Handler calls BoardSysTick::increment_tick()
4. Platform layer increments volatile tick counter
5. Optional: Call RTOS::tick() if enabled

## Risks / Trade-offs

### Risk 1: Timing Accuracy

**Risk**: SysTick accuracy depends on clock stability and interrupt latency.

**Mitigation**:
- Document expected accuracy (±1% typical)
- Measure and report actual accuracy in examples
- Use hardware timers for critical timing (future)
- Validate on all boards with oscilloscope

### Risk 2: Interrupt Overhead

**Risk**: High-frequency ticks (e.g., 100us) increase CPU overhead.

**Mitigation**:
- Default to 1ms tick (standard for RTOS)
- Document interrupt overhead per tick rate
- Measure CPU usage in examples
- Provide tick rate selection guidelines

### Risk 3: Counter Overflow

**Risk**: 64-bit microsecond counter overflows after ~584,542 years, but intermediate calculations can overflow.

**Mitigation**:
- Use u64 for microsecond storage
- Document overflow behavior
- Test overflow handling in examples
- Use difference calculations (handles wraparound)

### Risk 4: RTOS Integration

**Risk**: Different RTOSes may have conflicting SysTick requirements.

**Mitigation**:
- Make RTOS integration optional (#ifdef)
- Document integration patterns
- Provide examples for bare-metal and RTOS
- Keep SysTick handler simple and extensible

## Migration Plan

### Phase 1: Standardize Existing Boards (Week 1)
1. Update all board.cpp files to use standard pattern
2. Verify timing accuracy on each board
3. Document any board-specific quirks
4. **Rollback**: Git revert if timing accuracy degrades

### Phase 2: Create Examples (Week 2)
1. Implement basic_delays example
2. Implement timeout_patterns example
3. Implement performance example
4. Implement systick_demo example
5. **Rollback**: Examples are additive, can be removed individually

### Phase 3: Documentation (Week 3)
1. Write SysTick integration guide
2. Create board porting checklist
3. Document timing best practices
4. Add troubleshooting guide
5. **Rollback**: Documentation is non-breaking

### Phase 4: CI/CD Integration (Week 4)
1. Add examples to build matrix
2. Create hardware-in-loop tests
3. Add timing regression tests
4. **Rollback**: CI changes are isolated

### Backward Compatibility
- ✅ No breaking changes to existing APIs
- ✅ Existing code continues to work
- ✅ New pattern is opt-in via board updates
- ✅ Can coexist with old implementations

## Open Questions

1. **Should we support SysTick clock source selection (CPU vs CPU/8)?**
   - Current: Always use CPU clock
   - Alternative: Add template parameter for clock source
   - Decision: Defer until needed (YAGNI)

2. **Should examples target specific boards or be universal?**
   - Current: CMakeLists.txt supports all boards
   - Alternative: Board-specific examples
   - Decision: Universal examples (more maintainable)

3. **Should we add SysTick calibration support?**
   - Current: Assume clock frequency is accurate
   - Alternative: Auto-calibration using external reference
   - Decision: Defer to future enhancement

4. **How to handle boards without SysTick (ESP32)?**
   - Current: ESP32 uses different timer implementation
   - Alternative: Unified timer abstraction
   - Decision: Keep platform-specific (already resolved by Platform Layer)

## Performance Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| Timing Accuracy | ±1% | Sufficient for most embedded applications |
| Interrupt Latency | <10µs | Typical for ARM Cortex-M @ 100MHz+ |
| API Overhead | 0 cycles | Template instantiation eliminates calls |
| Memory Footprint | 12 bytes | Tick counter (8) + state (4) |
| CPU Usage (1ms tick) | <0.1% | Negligible for most applications |
| CPU Usage (100us tick) | <1% | Acceptable for high-resolution timing |

## Success Criteria

1. ✅ All 5 boards use identical SysTick integration pattern
2. ✅ Timing accuracy within ±1% on all boards (measured)
3. ✅ 4+ comprehensive examples demonstrating patterns
4. ✅ Zero compilation errors across all boards
5. ✅ Complete documentation (integration guide + examples)
6. ✅ CI/CD includes timing validation
7. ✅ Positive feedback from developers on clarity
