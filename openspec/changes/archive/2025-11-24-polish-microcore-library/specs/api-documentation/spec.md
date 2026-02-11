# Spec: API Reference Documentation

## ADDED Requirements

### Requirement: Doxygen-generated API reference
Complete API documentation MUST be automatically generated from code comments.

#### Scenario: Generate documentation
```bash
$ make docs

Running Doxygen...
Generating HTML documentation...
✓ Documentation generated: docs/html/index.html

$ open docs/html/index.html
# Browser opens with searchable API reference
```

**Expected**: Professional documentation website
**Rationale**: Industry-standard API documentation

#### Scenario: Browse GPIO API
```
API Reference > Concepts > GpioPin

template<typename T>
concept GpioPin

Requirements:
  - T::set_high() -> void
  - T::set_low() -> void
  - T::read() -> bool
  - T::toggle() -> void

Example:
  auto led = platform::Gpio::Output<platform::Pin::PA5>();
  led.set_high();  // Turn on
  led.set_low();   // Turn off

See also:
  - GpioOutput (stronger requirements)
  - GpioInput (input-only pin)
  - Examples: blink.cpp, button.cpp
```

**Expected**: Clear documentation with examples
**Rationale**: Reduces learning curve

### Requirement: All concepts documented with examples
Every C++20 concept MUST have usage examples.

#### Scenario: Document SystemClock concept
```cpp
/**
 * @concept SystemClock
 * @brief Represents a system clock configuration
 *
 * A type satisfies SystemClock if it provides compile-time access to
 * clock frequencies and runtime configuration.
 *
 * @par Example:
 * @code
 * using Clock = SystemClock<84'000'000>; // 84 MHz
 * Clock::configure();
 * uint32_t freq = Clock::frequency();
 * @endcode
 *
 * @par Requirements:
 * - configure() - Configure hardware clocks
 * - frequency() - Get current frequency in Hz
 *
 * @see platform::SystemClockConfig
 * @see board::ClockConfiguration
 */
template<typename T>
concept SystemClock = requires(T clock) {
    { T::configure() } -> std::same_as<void>;
    { T::frequency() } -> std::same_as<uint32_t>;
};
```

**Expected**: Doxygen extracts and formats beautifully
**Rationale**: Self-documenting code

### Requirement: Getting Started tutorial in docs
Step-by-step guide MUST exist for new users.

#### Scenario: New user follows tutorial
```markdown
# Getting Started with MicroCore

## Installation
1. Install ARM GCC toolchain
2. Install flash tools
3. Clone MicroCore

## First Blink
1. List boards: `./ucore list boards`
2. Build: `./ucore build nucleo_f401re blink`
3. Flash: `./ucore flash nucleo_f401re blink`
4. See LED blink at 1 Hz

## Next Steps
- Read [API Reference](api/index.html)
- Try [UART Example](examples/uart.html)
- Learn [Abstraction Tiers](guides/abstraction-tiers.html)
```

**Expected**: New user productive in 15 minutes
**Rationale**: First impressions matter

### Requirement: API stability policy documented
Versioning and compatibility MUST be guarantees clearly stated.

#### Scenario: Check API compatibility
```markdown
# API Stability Policy

## Semantic Versioning
MicroCore follows semver: MAJOR.MINOR.PATCH

- MAJOR: Breaking API changes
- MINOR: New features, backward compatible
- PATCH: Bug fixes only

## Stability Guarantees

### Stable APIs (v1.0+)
- Core concepts (GpioPin, SystemClock, UartConfig)
- Platform APIs (Gpio, Uart, SPI, I2C)
- Board abstractions (led, button, console)

**Promise**: No breaking changes within major version

### Experimental APIs
- Marked with `@experimental` in docs
- May change without notice
- Become stable after 2 minor releases

### Deprecated APIs
- Marked with `[[deprecated]]` attribute
- Removed after 2 major versions
- Migration guide provided
```

**Expected**: Users know what to expect
**Rationale**: Trust and predictability

## MODIFIED Requirements

### Requirement: Integrate docs into build system
Extend existing build targets MUST include documentation.

#### Scenario: CI builds documentation
```yaml
# .github/workflows/ci.yml
- name: Build documentation
  run: |
    make docs
    make docs-check  # Verify no warnings
```

**Expected**: Documentation always up-to-date
**Rationale**: Prevent doc rot

## REMOVED Requirements

None. This adds new documentation capabilities.
