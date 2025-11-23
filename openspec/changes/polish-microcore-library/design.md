# Design - Polish MicroCore Library

## Context

MicroCore library achieved B+ (83/100) in comprehensive analysis but requires systematic improvements to reach production-ready A- (90/100) quality. Analysis identified:

- **Critical blockers**: Performance issues (SAME70 25x slower), naming confusion, anti-pattern examples
- **Quality gaps**: No generated code validation, incomplete peripheral implementations
- **Documentation needs**: Missing API reference, unclear abstraction philosophy
- **System weaknesses**: Manual board configuration, no dependency validation

## Goals

1. **Eliminate release blockers** (Phase 1) - Fix critical issues preventing v1.0
2. **Establish quality gates** (Phase 2) - Prevent regressions through validation
3. **Complete platform support** (Phase 3) - UART/SPI/I2C/ADC across all boards
4. **Professional polish** (Phase 4) - Documentation, tooling, benchmarks

## Architectural Approach

### Phased Implementation Strategy

**Philosophy**: Incremental value delivery with clear validation criteria.

Each phase is:
- **Independently valuable** - Can ship after any phase
- **Risk-layered** - Critical fixes first, enhancements later
- **Testable** - Clear success criteria per phase
- **Parallelizable** - Tasks within phase can run concurrently

### Phase Sequencing Rationale

```
Phase 1: Critical Fixes
    ↓ (enables confident development)
Phase 2: Quality & Validation
    ↓ (enables safe peripheral additions)
Phase 3: Platform Completeness
    ↓ (enables professional release)
Phase 4: Professional Tools
```

**Why this order:**
1. Must fix broken things before adding new things
2. Must add validation before expanding implementations
3. Must complete implementations before optimizing tooling
4. Can release after any phase with increasing maturity

### Cross-Cutting Design Decisions

#### 1. Abstraction Tier System

**Problem**: Users have different skill levels and performance needs.

**Solution**: Three-tier API design

```
Simple Tier (Beginner)
  ↓ wraps
Fluent Tier (Application)
  ↓ wraps
Expert Tier (Library/HAL)
  ↓ wraps
Hardware Policy (Zero-overhead)
  ↓ wraps
Generated Registers (SVD)
```

**Trade-offs**:
- ✅ Serves all user types
- ✅ Clear progression path
- ✅ Zero-overhead preserved at all levels
- ❌ More API surface to maintain
- ❌ Documentation complexity

**Decision**: Worth the complexity. Enables library to serve beginners without sacrificing expert performance.

#### 2. Declarative Board Configuration

**Problem**: Adding boards requires C++ expertise, error-prone.

**Solution**: YAML-based board definitions with code generation.

```yaml
# boards/my_board/board.yaml (human-editable)
mcu:
  part_number: STM32F411CE
clock:
  crystal: 25MHz
  target: 100MHz
peripherals:
  led: PA5
```

```cpp
// boards/my_board/board.hpp (generated)
namespace board {
    using led = Gpio::Output<Pin::PA5>;
}
```

**Trade-offs**:
- ✅ Lowers barrier to adding boards
- ✅ Validation before compilation
- ✅ Documentation from schema
- ❌ Build step complexity
- ❌ Two sources of truth (YAML + generated code)

**Decision**: Benefits outweigh costs. Code generation already used for registers, extends pattern.

**Migration strategy**:
1. Keep existing C++ board definitions working
2. Add YAML alongside (Phase 2)
3. Migrate boards incrementally
4. Deprecate C++ boards in future major version

#### 3. Generated Code Validation

**Problem**: SVD→C++ generation not validated, could produce broken code.

**Solution**: Compile-time testing of generated code.

```
SVD File
  ↓ parse
Generated C++ Code
  ↓ compile
Validation Test (CMake target)
  ↓ static_assert
Type Safety Verified
```

**Architecture**:
```cmake
# tools/codegen/tests/CMakeLists.txt
add_executable(validate_stm32f4
    test_registers.cpp
    ../../src/generated/stm32f401/registers.hpp
)
target_compile_options(validate_stm32f4 PRIVATE
    -Wall -Wextra -Werror  # Fail on warnings
)
```

**Trade-offs**:
- ✅ Catches generation bugs early
- ✅ Validates register layout correctness
- ✅ Enforces code style
- ❌ Slower build (one-time per regeneration)
- ❌ Requires ARM toolchain for validation

**Decision**: Essential quality gate. Run validation on `ucore generate`, not every build.

#### 4. Multi-Platform Peripheral Implementation

**Problem**: UART/SPI/I2C incomplete across platforms.

**Solution**: Consistent hardware policy pattern.

**Interface (platform-independent)**:
```cpp
template<typename T>
concept UartPolicy = requires(T uart) {
    { T::configure() } -> std::same_as<void>;
    { T::write_byte(uint8_t{}) } -> std::same_as<void>;
    { T::read_byte() } -> std::same_as<uint8_t>;
};
```

**Implementation (platform-specific)**:
```cpp
// STM32F4
template<UartInstance Instance>
class Stm32f4UartHardwarePolicy {
    static void configure() {
        // STM32F4-specific registers
        USART1->CR1 |= USART_CR1_UE;
    }
};

// SAME70
template<UartInstance Instance>
class Same70UartHardwarePolicy {
    static void configure() {
        // SAME70-specific registers
        UART0->UART_CR = UART_CR_TXEN;
    }
};
```

**Trade-offs**:
- ✅ Zero-overhead abstraction
- ✅ Compile-time validation via concepts
- ✅ Platform-specific optimizations possible
- ❌ Requires deep hardware knowledge
- ❌ Duplication across platforms

**Decision**: Core architectural pattern. Enables portability without performance cost.

**Implementation strategy**:
1. Define concept interface first
2. Implement for one platform (STM32F4)
3. Validate with real hardware
4. Port to other platforms
5. Share tests across platforms

### Testing Strategy

**Hierarchy of validation**:

```
Level 1: Compile-time Tests
  - Concept satisfaction (static_assert)
  - Type correctness
  - Register layout validation

Level 2: Unit Tests (Host platform)
  - Logic correctness
  - Error handling
  - Edge cases

Level 3: Integration Tests (Hardware)
  - GPIO toggle timing
  - UART communication
  - SPI transfers
  - I2C transactions

Level 4: System Tests
  - Example applications
  - Board bring-up validation
  - Performance benchmarks
```

**Phase mapping**:
- Phase 1: Manually tested critical fixes
- Phase 2: Adds Level 1 (codegen validation)
- Phase 3: Adds Level 2 & 3 (host + hardware tests)
- Phase 4: Adds Level 4 (benchmarks, CI)

### Documentation Architecture

**Three-tier documentation system**:

```
Tier 1: Getting Started (tutorials)
  - Installation
  - First blink
  - First UART
  Target: Complete beginners

Tier 2: Guides (how-to)
  - Adding a board
  - Porting to new platform
  - Choosing abstraction tier
  Target: Application developers

Tier 3: API Reference (Doxygen)
  - Concept documentation
  - Class documentation
  - Hardware policy details
  Target: Library developers
```

**Generation pipeline**:
```
Source Code (Doxygen comments)
  ↓
Doxygen → HTML
  ↓
Markdown Guides
  ↓
Static Site (GitHub Pages)
```

**Quality gates**:
- All public APIs must have Doxygen comments
- All concepts must have examples
- All guides validated by test user

## Critical Decisions

### Decision 1: Keep or Remove Result<T> Pattern?

**Analysis identified**: Many functions return `Result<void>` but always `Ok()`.

**Options**:
1. **Remove** - Simplify API where errors impossible
2. **Keep** - Consistency, future extensibility
3. **Audit** - Remove where truly impossible, keep where plausible

**Decision**: **Option 3 - Audit**

**Rationale**: Some operations (GPIO init) truly can't fail. Others (UART init) might fail in future (invalid baud rate). Audit case-by-case.

**Implementation** (Phase 2):
```cpp
// Before: Always returns Ok()
Result<void> Gpio::configure() {
    // ... setup ...
    return Ok();  // Never fails
}

// After: Just do it
void Gpio::configure() {
    // ... setup ...
}

// But keep for UART (could fail)
Result<Uart> Uart::create(BaudRate baud) {
    if (!is_valid_baud(baud)) {
        return Err(ErrorCode::InvalidBaudRate);
    }
    // ... setup ...
    return Ok(uart);
}
```

### Decision 2: YAML vs JSON for Board Config?

**Options**:
1. **YAML** - More readable, comments, less verbose
2. **JSON** - Standard, better tooling, JSON Schema
3. **Both** - YAML for humans, JSON for tools

**Decision**: **YAML with JSON Schema validation**

**Rationale**:
- ✅ YAML more readable for configuration
- ✅ JSON Schema provides validation
- ✅ Easy to convert YAML→JSON for validation
- ✅ Industry standard (GitHub Actions, Docker Compose)

**Implementation**:
```python
# ucore CLI
yaml_config = yaml.load('board.yaml')
json_config = json.dumps(yaml_config)
jsonschema.validate(json_config, schema)
```

### Decision 3: Auto-generate or Manual Board Headers?

**Question**: After YAML board config, still need C++ headers?

**Decision**: **Generate from YAML**

**Rationale**:
- ✅ Single source of truth (YAML)
- ✅ Validation before compilation
- ✅ Faster board addition
- ❌ Requires re-generation on change
- ❌ Can't hand-optimize generated code

**Mitigation**: Allow override for complex boards
```yaml
# board.yaml
code_generation:
  enabled: true
  custom_overrides:
    - custom_init.cpp  # Included after generation
```

### Decision 4: Phase 3 Parallel or Sequential?

**Question**: Can UART, SPI, I2C, ADC be developed in parallel?

**Decision**: **Parallel development with shared testing framework**

**Rationale**:
- ✅ Faster completion (4 peripherals in parallel)
- ✅ Peripherals independent
- ❌ Requires coordination on concept interfaces
- ❌ Testing framework must be ready first

**Implementation order**:
```
Week 1: Define all peripheral concepts (sequential)
Week 2-3: Implement peripherals (parallel)
  - Dev 1: UART
  - Dev 2: SPI
  - Dev 3: I2C
  - Dev 4: ADC
Week 4: Integration testing (sequential)
```

## Migration Path

### For Existing Users

**Phase 1**: Transparent (bug fixes, naming)
- Action: Update git pull
- Breaking: None (namespace already ucore::)

**Phase 2**: Opt-in (board YAML)
- Action: Can migrate board to YAML
- Breaking: None (C++ boards still work)

**Phase 3**: Feature additions
- Action: New peripherals available
- Breaking: None (additive only)

**Phase 4**: Ecosystem improvements
- Action: Better docs, tools
- Breaking: None

### For New Contributors

**Phase 1**: Fix critical issues
- Easier to understand codebase (consistent naming)
- Confidence in hardware (correct clock configs)

**Phase 2**: Quality gates
- Easier to contribute (validation catches errors)
- Lower barrier (YAML boards, not C++)

**Phase 3**: Complete platform
- More areas to contribute
- Better examples to learn from

**Phase 4**: Professional tools
- Clear contribution guides
- Automated testing in CI

## Risks and Mitigations

### Risk 1: Scope Creep

**Risk**: 223 hours of work could expand to 400+ hours.

**Mitigation**:
- Strict phase boundaries
- Can ship after any phase
- Defer non-critical features to next version

### Risk 2: Breaking Changes

**Risk**: YAML board config might break existing projects.

**Mitigation**:
- Keep C++ boards working during transition
- Provide migration script: `./ucore migrate-board <name>`
- Deprecation policy (2 major versions warning)

### Risk 3: Testing Burden

**Risk**: Testing all peripherals on all boards is time-consuming.

**Mitigation**:
- Automated hardware-in-loop testing (Phase 3)
- CI tests compilation for all boards
- Community testing (beta releases)

### Risk 4: Documentation Maintenance

**Risk**: Docs become stale as code evolves.

**Mitigation**:
- Doxygen auto-generation (always in sync)
- CI checks for doc build warnings
- Examples as integration tests

## Success Metrics

### Phase 1 Success
- ✅ All 5 boards build and flash successfully
- ✅ SAME70 LED blink timing verified (1 Hz ±1%)
- ✅ Zero "Alloy" references in docs
- ✅ uart_logger passes code review (no raw registers)

### Phase 2 Success
- ✅ Generated code compiles with -Wall -Werror
- ✅ At least 2 boards migrated to YAML
- ✅ API documentation published and browsable
- ✅ `make docs` runs without warnings

### Phase 3 Success
- ✅ UART example works on all 5 boards
- ✅ SPI example drives real hardware (OLED display)
- ✅ I2C example reads sensor (BME280)
- ✅ ADC example reads voltage
- ✅ 80%+ code coverage

### Phase 4 Success
- ✅ Documentation auto-deploys on commit
- ✅ Board wizard creates working board in <10 minutes
- ✅ Benchmarks show performance vs competitors
- ✅ CI pipeline passes for all platforms

### Overall Success (v1.0 Release)
- ✅ Grade improvement: B+ (83/100) → A- (90/100)
- ✅ Complete platform support (UART/SPI/I2C/ADC)
- ✅ Professional documentation and tooling
- ✅ Community adoption (GitHub stars, forks)
- ✅ Production use cases (at least 3 projects)

## Open Questions

1. **Tier naming**: Simple/Fluent/Expert or Beginner/Standard/Advanced?
   - Defer to Phase 3 user testing

2. **Host platform**: Keep or deprecate?
   - Keep for unit testing (Phase 2)
   - Re-evaluate after hardware test framework (Phase 3)

3. **RTOS integration**: Include in this change or separate?
   - Separate change (already mostly complete)
   - Document integration in Phase 2

4. **ESP32 support**: When to add?
   - After Phase 3 (platform completeness)
   - Requires different architecture (ESP-IDF integration)

5. **Arduino compatibility layer**: Worth it?
   - Defer to community feedback after v1.0
   - Could be separate library (microcore-arduino)
