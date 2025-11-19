# Change Proposal: Memory Analysis and Optimization Tools

## Metadata

- **Change ID**: `add-memory-analysis`
- **Status**: Proposed
- **Priority**: High
- **Phase**: 0 (Foundation)
- **Created**: 2025-10-30
- **Author**: Alloy Team

## Summary

Implement memory footprint analysis tools and compilation flags to support low-memory MCUs (8KB-16KB RAM). This is essential infrastructure for ensuring Alloy meets its design goal of functioning well on memory-constrained microcontrollers like the Renesas RL78 (8KB-16KB RAM).

## Motivation

### Problem

Many embedded MCUs have extremely limited RAM:
- Renesas RL78: 8KB-16KB typical
- STM32F103C6: 10KB RAM
- ATmega328P: 2KB RAM

Without careful design and tooling to measure/optimize memory usage, a C++ framework can easily consume all available RAM with overhead alone, making real applications impossible.

## Related Work

- **Dependencies**: `add-project-structure` (CMake configuration must exist)
- **Related Specs**: None yet (but future HAL specs will reference memory budgets)
- **ADR**: ADR-013 (Low-Memory Support)

## Proposed Solution

### Architecture Changes

1. **CMake Memory Analysis Targets**: Add custom targets to generate memory usage reports
2. **Minimal Build Mode**: Add `ALLOY_MINIMAL_BUILD` option for size optimization
3. **Python Memory Analyzer**: Tool to parse linker map files and generate reports
4. **Static Assertions**: Template validation for zero-overhead guarantees
5. **Memory Budget Documentation**: Template for documenting module footprints

### Design Decisions

**Memory Budget Targets**:
- Tiny (2-8KB RAM): < 512 bytes Alloy overhead
- Small (8-32KB RAM): < 2KB Alloy overhead
- Medium (32-128KB RAM): < 8KB Alloy overhead
- Large (128+KB RAM): < 16KB Alloy overhead

**Key Strategies** (from ADR-013):
- Zero-cost abstractions (templates + constexpr)
- Template bloat control (extract non-templated base classes)
- Compile-time configuration (enable only needed peripherals)
- Static buffer sizing
- Stack usage awareness

### API Examples

#### CMake Usage

```cmake
# User's CMakeLists.txt
option(ALLOY_MINIMAL_BUILD "Optimize for size" OFF)

# After building:
cmake --build build --target memory-report
```

#### Memory Report Output

```
===========================================
Alloy Memory Usage Report
===========================================
Target MCU: RL78/G13 (16KB RAM, 64KB Flash)

Flash Usage:
-----------
Total:      12,432 / 65,536 bytes (19.0%)
  .text:    11,024 bytes (code)
  .rodata:   1,408 bytes (const data)

RAM Usage:
----------
Total:      1,856 / 16,384 bytes (11.3%)
  .data:      512 bytes (initialized data)
  .bss:     1,344 bytes (zero-initialized)

Top RAM Consumers:
------------------
1. uart_rx_buffer     : 256 bytes
2. i2c_buffer         : 128 bytes
3. main_stack         : 512 bytes
4. Alloy HAL overhead : 192 bytes
5. Application data   : 768 bytes

Memory Budget Check:
--------------------
✅ Target: < 2KB Alloy overhead (Small MCU)
✅ Actual: 192 bytes (well within budget)

Recommendations:
----------------
- Consider reducing uart_rx_buffer to 128 bytes (-128 bytes)
- Stack usage is reasonable for this application
```

## Implementation Plan

See `tasks.md` for detailed breakdown.

## Testing Strategy

1. **Compile-time tests**: Static assertions validate zero-overhead types
2. **Build tests**: Verify memory-report target works across all toolchains
3. **Integration tests**: Build minimal example for each MCU class, verify budget met
4. **Regression tests**: CI tracks memory footprint over time, fails if overhead increases

## Migration Path

N/A - This is new infrastructure, no migration needed.

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Parser breaks with different linker formats | High | Test with arm-none-eabi-gcc, rl78-elf-gcc, xtensa toolchains |
| Overhead calculations inaccurate | Medium | Validate against manual nm/objdump analysis |
| False positives in CI | Low | Set reasonable thresholds with margin |

## Success Criteria

- [ ] `memory-report` target works for host, ARM, RL78 toolchains
- [ ] Python analyzer correctly parses linker maps from all 3 toolchains
- [ ] `ALLOY_MINIMAL_BUILD=ON` reduces footprint by at least 15%
- [ ] CI tracks memory usage per commit
- [ ] Documented memory budget for each module in Phase 0
- [ ] Example blinky app uses < 1KB RAM on RL78 (8KB target)

## Alternatives Considered

1. **No tooling, manual analysis**: Too error-prone, will not scale
2. **Third-party tools (e.g., Puncover)**: Good, but adds dependency and learning curve
3. **IDE-based analysis**: Not portable, different for each IDE

## Open Questions

- [ ] Should we integrate with GitHub Actions to comment memory usage on PRs?
- [ ] Should memory budgets be enforced (hard CI failure) or warnings?
- [ ] What threshold for "memory regression" should trigger CI failure? (+5%? +10%?)

## References

- ADR-013: Support for Low-Memory MCUs (8KB-16KB RAM)
- architecture.md: Section 12 (Memory-Constrained Design)
- Renesas RL78 Datasheet (target platform)
