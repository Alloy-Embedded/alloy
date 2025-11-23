# API Stability Policy

## Overview

MicroCore follows **Semantic Versioning 2.0** (semver) to communicate API changes clearly to users.

**Current Version:** `0.1.0` (Pre-release)

## Version Format

```
MAJOR.MINOR.PATCH
```

- **MAJOR** - Incremented for breaking changes
- **MINOR** - Incremented for new features (backward compatible)
- **PATCH** - Incremented for bug fixes (backward compatible)

## Stability Guarantees

### During Pre-1.0 (Current: 0.x.y)

⚠️ **No stability guarantees**

- API may change at any time
- Breaking changes allowed in minor versions
- Experimental features may be removed
- Use for evaluation and feedback only

**Upgrade strategy:** Pin to exact version in production

### After 1.0 Release

✅ **Stability guaranteed within major version**

#### Major Version (X.0.0)

**Breaking changes allowed:**
- API signature changes
- Concept requirement changes
- Behavior changes
- Removal of deprecated features

**Example:** `1.x.y → 2.0.0`

**Migration:** Update guide provided for each major release

#### Minor Version (x.Y.0)

**New features added (backward compatible):**
- New classes/functions
- New concept requirements (additive only)
- New platforms/boards
- Performance improvements
- New optional parameters (with defaults)

**Example:** `1.0.0 → 1.1.0`

**Migration:** Drop-in upgrade, no code changes required

#### Patch Version (x.y.Z)

**Bug fixes only:**
- Crash fixes
- Memory leaks
- Incorrect behavior fixes
- Documentation corrections

**Example:** `1.0.0 → 1.0.1`

**Migration:** Drop-in upgrade, strongly recommended

## What's Stable?

### Stable (Never Breaking)

✅ **Core Concepts** (after 1.0)
- `GpioPin`
- `SystemClock`
- `UartPeripheral`
- `SpiPeripheral`
- `I2cPeripheral`

✅ **Result<T> API**
- `.is_ok()`
- `.is_err()`
- `.value()`
- `.error()`

✅ **Board Abstraction**
- `board::init()`
- `board::led::*`
- `board::delay_ms()`

✅ **Platform APIs** (signatures)
- `Gpio<Policy>::set_high()`
- `Uart<Config>::write_byte()`
- `Clock::initialize()`

### Evolving (May Change)

⚠️ **Hardware Policies** (implementation details)
- Register access patterns
- Performance optimizations
- Platform-specific workarounds

⚠️ **Build System**
- CMake variables
- Toolchain files
- Board YAML schema (may extend)

⚠️ **CLI Tools**
- `ucore` command syntax
- Code generation output format

### Experimental (No Guarantees)

🔬 **New Features** (marked as experimental)
- Features in pre-release
- Advanced APIs
- Platform-specific extensions

**Marked as:**
```cpp
namespace ucore::experimental {
    // Experimental API - may change or be removed
}
```

## Deprecation Policy

### Process

1. **Announcement** - Deprecated in version X.Y.0
2. **Transition** - Available but warns for 2 minor versions
3. **Removal** - Removed in next major version

### Example Timeline

```
1.5.0 - Feature announced as deprecated (warning)
1.6.0 - Still available (warning)
1.7.0 - Still available (warning)
2.0.0 - Removed
```

### Deprecation Markers

```cpp
// In code
[[deprecated("Use new_function() instead")]]
void old_function();

// In documentation
/**
 * @deprecated Since 1.5.0, use new_function() instead.
 * Will be removed in 2.0.0.
 */
void old_function();
```

## Version Compatibility Matrix

### Framework Versions

| Version | C++ Std | CMake  | ARM GCC | Status |
|---------|---------|--------|---------|--------|
| 0.1.x   | C++20   | 3.25+  | 10.3+   | Pre-release |
| 1.0.x   | C++20   | 3.25+  | 10.3+   | Planned |
| 2.0.x   | C++23   | 3.27+  | 12.0+   | Future |

### Platform Support

| Platform | Since | Status | Notes |
|----------|-------|--------|-------|
| STM32F4  | 0.1.0 | ✅ Stable | Full support |
| STM32F7  | 0.1.0 | ✅ Stable | Full support |
| STM32G0  | 0.1.0 | ✅ Stable | Full support |
| SAME70   | 0.1.0 | ✅ Stable | Full support |
| Host     | 0.1.0 | ✅ Testing | Unit testing only |
| ESP32    | Future | 🔬 Planned | Not yet available |
| RP2040   | Future | 🔬 Planned | Not yet available |

### Board Support

| Board | Since | Status | Platform |
|-------|-------|--------|----------|
| Nucleo-F401RE | 0.1.0 | ✅ Stable | STM32F4 |
| Nucleo-F722ZE | 0.1.0 | ✅ Stable | STM32F7 |
| Nucleo-G071RB | 0.1.0 | ✅ Stable | STM32G0 |
| Nucleo-G0B1RE | 0.1.0 | ✅ Stable | STM32G0 |
| SAME70 Xplained | 0.1.0 | ✅ Stable | SAME70 |

## Breaking Change Examples

### Major Version Breaking Changes

#### Example 1: Concept Signature Change

**1.x.y:**
```cpp
template <typename T>
concept GpioPin = requires(T) {
    { T::set_high() } -> std::same_as<void>;
};
```

**2.0.0:**
```cpp
template <typename T>
concept GpioPin = requires(T) {
    { T::set_high() } -> std::same_as<Result<void>>;  // Now returns Result
};
```

**Migration:** Update all GpioPin implementations to return `Result<void>`

#### Example 2: API Signature Change

**1.x.y:**
```cpp
void Uart::write_byte(uint8_t data);
```

**2.0.0:**
```cpp
Result<void> Uart::write_byte(uint8_t data);  // Now returns Result
```

**Migration:** Check return value:
```cpp
// Old
uart.write_byte('A');

// New
auto result = uart.write_byte('A');
if (!result.is_ok()) {
    // Handle error
}
```

### Minor Version Additions

#### Example: New Optional Feature

**1.0.0:**
```cpp
void Gpio::configure_output();
```

**1.1.0:**
```cpp
void Gpio::configure_output();
void Gpio::configure_output_speed(Speed speed);  // New, optional
```

**Migration:** None required (backward compatible)

## Upgrade Guide Process

For each major version, we provide:

1. **Changelog** - All changes listed
2. **Migration Guide** - Step-by-step upgrade instructions
3. **Deprecation List** - Removed features
4. **New Features** - Added functionality
5. **Breaking Changes** - Required code changes

Example location: `docs/upgrades/UPGRADE_1.0_to_2.0.md`

## Testing Policy

### Regression Testing

- All tests from previous versions must pass
- New tests added for new features
- Performance regression tests

### Compatibility Testing

- Test on all supported compilers
- Test on all supported platforms
- Test with all supported boards

## Support Policy

### Long-Term Support (LTS)

**Not yet defined** (will be announced with 1.0 release)

Planned:
- 1 year of bugfixes for last major version
- Security patches for 2 years

### Community Support

- GitHub Issues - Bug reports
- GitHub Discussions - Questions and ideas
- Pull Requests - Community contributions

## How to Report Breaking Changes

If you encounter a breaking change:

1. **Check changelog** - Is it documented?
2. **Open issue** - Report unexpected breakage
3. **Include:**
   - Old version number
   - New version number
   - Code that broke
   - Expected vs actual behavior

## Commitment

**Pre-1.0:** No stability guarantees
**Post-1.0:** Strict semantic versioning with clear upgrade paths

We value your time and will communicate breaking changes clearly.

---

**Last Updated:** 2025-01-23
**Next Review:** Before 1.0.0 release
