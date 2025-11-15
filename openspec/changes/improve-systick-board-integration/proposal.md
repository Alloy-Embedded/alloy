# Improve SysTick Board Integration

## Why

Currently, SysTick timer integration across boards is **inconsistent** and lacks comprehensive examples demonstrating compile-time safe timing capabilities. Each board implements SysTick differently, and there are no examples showcasing:

- Accurate microsecond/millisecond timing
- Timeout handling patterns
- Performance measurement
- Multiple timing modes (blocking delays, non-blocking timeouts)
- Integration with RTOS tick

This inconsistency makes it difficult to write portable timing code and doesn't showcase the compile-time safety philosophy of the project.

## What Changes

1. **Standardize SysTick integration** across all boards (F401RE, F722ZE, G071RB, G0B1RE, SAME70)
   - Consistent BoardSysTick type definition
   - Unified interrupt handler pattern
   - Compile-time clock frequency validation

2. **Create comprehensive timing examples** demonstrating:
   - Basic delays (ms/us)
   - Non-blocking timeouts
   - Performance measurement
   - RTOS tick integration
   - Overflow handling

3. **Add compile-time safety checks**:
   - SysTick clock frequency validation
   - Period calculation verification
   - Overflow detection at compile-time

4. **Document SysTick patterns** for new board ports

## Impact

### Affected Specs
- `board-systick` (NEW) - SysTick integration requirements
- `examples-timing` (NEW) - Timing examples and patterns
- `board-support` (MODIFIED) - Add SysTick initialization requirement

### Affected Code
- `boards/nucleo_f401re/` - Standardize implementation
- `boards/nucleo_f722ze/` - Standardize implementation
- `boards/nucleo_g071rb/` - Standardize implementation
- `boards/nucleo_g0b1re/` - Standardize implementation
- `boards/same70_xplained/` - Standardize implementation
- `examples/timing/` (NEW) - Comprehensive timing examples
- `examples/systick_demo/` (NEW) - SysTick feature demonstration

### Benefits
- ✅ **Consistent** SysTick integration across all boards
- ✅ **Type-safe** timing APIs with compile-time validation
- ✅ **Educational** examples showing best practices
- ✅ **Portable** code that works on any board
- ✅ **Zero-overhead** with compile-time configuration
- ✅ **RTOS-ready** integration patterns

### Breaking Changes
None - this is an additive change that improves existing implementations.
