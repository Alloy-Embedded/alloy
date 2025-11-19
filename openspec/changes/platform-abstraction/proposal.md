# Platform Abstraction Layer

## Why

Alloy currently supports only SAME70 (ARM Cortex-M7), limiting its portability and testability. Adding platform abstraction will enable:

**Multi-Platform Support**:
- **ARM Cortex-M**: SAME70, STM32F4, STM32H7, nRF52, etc.
- **ESP32**: ESP32, ESP32-C3, ESP32-S3 (FreeRTOS + ESP-IDF)
- **Linux/POSIX**: Enable host-based testing without hardware
- **Future**: RISC-V, Windows (simulation/testing)

**Key Benefits**:
1. **Testability**: Run HAL tests on Linux (no hardware, no flash cycle)
2. **Portability**: Write once, run on any supported platform
3. **Flexibility**: Easy to add new MCUs and architectures
4. **CI/CD**: Automated testing in Docker/GitHub Actions
5. **Zero Overhead**: Template-based (no virtual functions, compile-time resolution)

After analyzing LUG's platform abstraction (documented in `docs/LUG_PLATFORM_ARCHITECTURE_ANALYSIS.md`), we identified a proven multi-platform approach but **rejected virtual functions entirely**.

**Critical Design Decision**:
- ❌ **NO virtual functions** - Unacceptable overhead for embedded (vtable lookup, no inlining, larger binary)
- ✅ **Templates only** - Zero runtime overhead, compile-time resolution, full inlining
- ✅ **CRTP pattern** - Static polymorphism when needed
- ✅ **Concepts (C++20)** - Compile-time interface checking (optional, fallback to static_assert)

## What Changes

This change introduces a **3-layer platform abstraction system** (simplified from LUG's 4 layers):

### Architecture Overview (Template-Based, Zero Virtual Functions)

```
┌────────────────────────────────────────────────────────────┐
│  Application Layer (User Code)                             │
│  - Uses board configuration                                │
│  - Template-based, platform-agnostic                       │
│  - auto uart = board::uart0; (compile-time resolution)    │
└───────────────────────┬────────────────────────────────────┘
                        │
┌───────────────────────▼────────────────────────────────────┐
│  Board Layer (Type Aliases & Configuration)                │
│  - boards/same70_xplained/board.hpp                       │
│  - Type aliases: using uart0 = platform::same70::Uart<0>  │
│  - Compile-time pin mappings                               │
└───────────────────────┬────────────────────────────────────┘
                        │
┌───────────────────────▼────────────────────────────────────┐
│  Platform Layer (Template Implementations)                 │
│  - platform/same70/uart.hpp    (templates)                │
│  - platform/linux/uart.hpp     (templates)                │
│  - platform/esp32/uart.hpp     (templates)                │
│  - NO virtual functions, 100% compile-time                 │
└───────────────────────┬────────────────────────────────────┘
                        │
┌───────────────────────▼────────────────────────────────────┐
│  Concept Layer (Compile-Time Contracts) - OPTIONAL         │
│  - concepts/uart_concept.hpp (C++20)                       │
│  - OR static_assert checks (C++17 fallback)                │
│  - Validates interface at compile-time                     │
│  - NO runtime cost, NO vtables                             │
└────────────────────────────────────────────────────────────┘
```

### Implementation Phases

**Phase 1: Concept Layer (Week 1-2)**
- Create `hal/concepts/` with C++20 concepts OR C++17 static_assert checks
- UartConcept, GpioConcept, I2cConcept, SpiConcept
- Compile-time interface validation (no runtime cost)
- Use existing Result<T, E> for error handling

**Phase 2: SAME70 Platform Templates (Week 3-4)**
- Refactor existing SAME70 code to template-based implementation
- `hal/platform/same70/uart.hpp` as template class (NO virtual functions)
- Template-based peripheral addressing: `Uart<BASE_ADDR, IRQ_ID>`
- Board configuration with type aliases

**Phase 3: Linux Platform Templates (Week 5-6)**
- Implement `hal/platform/linux/` using templates + POSIX (termios, sysfs)
- Enable host-based HAL testing (no hardware needed)
- CI/CD integration

**Phase 4: ESP32 Platform Templates (Week 7-8)**
- Implement `hal/platform/esp32/` as templates wrapping ESP-IDF HAL
- FreeRTOS integration
- ESP32 DevKit board configuration

### Key Features

1. **100% Template-Based**: ZERO virtual functions, ZERO vtables, ZERO runtime polymorphism
2. **Compile-Time Platform Selection**: CMake variable `ALLOY_PLATFORM` selects implementation
3. **Absolute Zero Runtime Overhead**: Everything resolved at compile-time, full inlining
4. **Static Polymorphism**: CRTP pattern when inheritance-like behavior needed
5. **Concepts (C++20) or static_assert (C++17)**: Compile-time interface validation
6. **Direct Instantiation**: No registries, no shared_ptr overhead
7. **Flat Directory Structure**: 3 levels max

### Template-Based Design Example

```cpp
// Platform-specific UART template (NO virtual functions)
namespace alloy::hal::same70 {

template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class Uart {
public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    // All methods are NON-virtual, fully inlinable
    Result<void> open() {
        // Direct register access, no indirection
        PMC->PMC_PCER0 = (1 << irq_id);
        HW->UART_CR = UART_CR_TXEN | UART_CR_RXEN;
        m_opened = true;
        return Ok();
    }

    Result<void> close() {
        if (!m_opened) return Err(std::errc::operation_not_permitted);
        HW->UART_CR = UART_CR_TXDIS | UART_CR_RXDIS;
        m_opened = false;
        return Ok();
    }

    Result<size_t> write(const uint8_t* data, size_t size) {
        if (!m_opened) return Err(std::errc::operation_not_permitted);
        for (size_t i = 0; i < size; ++i) {
            while (!(HW->UART_SR & UART_SR_TXRDY));
            HW->UART_THR = data[i];
        }
        return Ok(size);
    }

private:
    static constexpr volatile Uart_Registers* HW =
        reinterpret_cast<volatile Uart_Registers*>(BASE_ADDR);
    bool m_opened = false;
};

// Type aliases for specific instances (compile-time)
using Uart0 = Uart<0x400E0800, ID_UART0>;
using Uart1 = Uart<0x400E0A00, ID_UART1>;

} // namespace alloy::hal::same70

// Board configuration (type aliases)
namespace alloy::boards::same70_xplained {
    using uart0 = hal::same70::Uart0;
    using uart1 = hal::same70::Uart1;
}

// User code (platform-agnostic via templates)
auto uart = board::uart0{};  // Resolved at compile-time
uart.open();                  // Inlined, no virtual call
uart.write(data, size);       // Direct register access
uart.close();                 // Zero overhead
```

### Concept Validation (C++20)

```cpp
// Concept defines interface at compile-time (NO vtable)
template <typename T>
concept UartConcept = requires(T uart, const uint8_t* data, size_t size) {
    { uart.open() } -> std::same_as<Result<void>>;
    { uart.close() } -> std::same_as<Result<void>>;
    { uart.write(data, size) } -> std::same_as<Result<size_t>>;
    { uart.read(data, size) } -> std::same_as<Result<size_t>>;
};

// Usage: compiler validates interface at compile-time
template <UartConcept TUart>
void send_message(TUart& uart, const char* msg) {
    uart.open();
    uart.write(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
    uart.close();
}

// Alternative: C++17 static_assert fallback
template <typename T>
constexpr bool is_uart_v = requires(T uart) { /* same checks */ };

template <typename TUart>
void send_message(TUart& uart, const char* msg) {
    static_assert(is_uart_v<TUart>, "TUart must satisfy UartConcept");
    // ... same code
}
```

### Directory Structure

```
src/
├── hal/
│   ├── interfaces/              # Platform-agnostic interfaces (NEW)
│   │   ├── uart_interface.hpp  # IUart pure virtual base
│   │   ├── gpio_interface.hpp  # IGpio pure virtual base
│   │   ├── i2c_interface.hpp   # II2c pure virtual base
│   │   └── spi_interface.hpp   # ISpi pure virtual base
│   │
│   └── platform/                # Platform-specific implementations (NEW)
│       ├── same70/              # ARM Cortex-M7 (MODIFIED from existing)
│       │   ├── uart.hpp
│       │   ├── uart.cpp
│       │   ├── gpio.hpp
│       │   └── platform.cmake
│       │
│       ├── linux/               # POSIX (NEW - for testing)
│       │   ├── uart.hpp
│       │   ├── uart.cpp
│       │   ├── gpio.hpp
│       │   └── platform.cmake
│       │
│       ├── esp32/               # ESP-IDF (NEW)
│       │   ├── uart.hpp
│       │   └── platform.cmake
│       │
│       └── stm32f4/             # ARM Cortex-M4 (NEW - future)
│
├── boards/                      # Board configurations (MODIFIED)
│   ├── same70_xplained/
│   │   ├── board.hpp            # Direct device instantiation
│   │   └── board.cpp            # Board init
│   │
│   ├── linux_host/              # NEW - for testing
│   └── esp32_devkit/            # NEW
│
cmake/
├── platforms/                   # NEW
│   ├── same70.cmake
│   ├── linux.cmake
│   └── esp32.cmake
│
└── toolchains/                  # EXISTING, modified
    ├── arm-none-eabi.cmake
    └── esp32.cmake
```

### Breaking Changes

**BREAKING - API Changes**:
- Existing SAME70 implementations will be refactored to inherit from interfaces
- Direct register access wrapped in interface implementations
- Board initialization changes from global singletons to explicit instances

**Migration Strategy**:
- Phase 1-2: Create interfaces, refactor SAME70 (2 weeks)
- Phase 3: Add deprecation warnings to old direct-access code (2 weeks)
- Phase 4: Remove old APIs in next major version (after 3 months)

**Non-Breaking Additions**:
- Linux platform (new, testing only)
- ESP32 platform (new)
- Platform selection via CMake (opt-in)

## Impact

### Affected Specifications

**Modified**:
- `hal-uart-interface` - Add IUart abstract base class
- `hal-gpio-interface` - Add IGpio abstract base class
- `hal-i2c-spi` - Add II2c, ISpi abstract base classes

**New**:
- `platform-interface-layer` - Pure virtual interfaces with NVI pattern
- `platform-same70` - SAME70 implementations of interfaces
- `platform-linux` - Linux/POSIX implementations (testing)
- `platform-esp32` - ESP32/ESP-IDF implementations
- `platform-build-system` - CMake-based platform selection

### Affected Code Areas

**Core Infrastructure** (New):
- `src/hal/interfaces/uart_interface.hpp` - IUart base class
- `src/hal/interfaces/gpio_interface.hpp` - IGpio base class
- `src/hal/interfaces/i2c_interface.hpp` - II2c base class
- `src/hal/interfaces/spi_interface.hpp` - ISpi base class

**SAME70 Platform** (Modified from existing):
- `src/hal/platform/same70/uart.hpp` - Refactored to implement IUart
- `src/hal/platform/same70/gpio.hpp` - Refactored to implement IGpio
- `src/hal/platform/same70/i2c.hpp` - Refactored to implement II2c
- `src/hal/platform/same70/spi.hpp` - Refactored to implement ISpi

**Linux Platform** (New):
- `src/hal/platform/linux/uart.hpp` - POSIX termios implementation
- `src/hal/platform/linux/gpio.hpp` - sysfs or simulated GPIO

**ESP32 Platform** (New):
- `src/hal/platform/esp32/uart.hpp` - ESP-IDF UART wrapper
- `src/hal/platform/esp32/gpio.hpp` - ESP-IDF GPIO wrapper

**Board Configurations** (Modified):
- `boards/same70_xplained/board.hpp` - Direct device instantiation
- `boards/linux_host/board.hpp` - Linux testing configuration (new)
- `boards/esp32_devkit/board.hpp` - ESP32 DevKit configuration (new)

**Build System** (New/Modified):
- `cmake/platforms/same70.cmake` - Platform-specific settings
- `cmake/platforms/linux.cmake` - Linux platform settings
- `cmake/platforms/esp32.cmake` - ESP32 platform settings
- Root `CMakeLists.txt` - Add ALLOY_PLATFORM variable

**Documentation** (New):
- `docs/PLATFORM_ABSTRACTION.md` - Architecture overview
- `docs/ADDING_PLATFORMS.md` - Step-by-step porting guide
- `docs/TESTING_ON_LINUX.md` - How to run HAL tests on host

**Examples** (Modified/New):
- `examples/same70_blink/` - Updated to use new architecture
- `examples/linux_uart_test/` - Host-based UART testing (new)
- `examples/multi_platform/` - Cross-platform example (new)

### Performance Impact

**Zero Runtime Overhead**:
- ✅ Compile-time platform selection (only selected code compiled)
- ✅ Virtual functions only for initialization (hot path is non-virtual or inlined)
- ✅ Template-based peripheral addressing (no indirection)
- ✅ Direct device instantiation (no shared_ptr, no registry lookup)

**Build Time**:
- ⚠️ Slightly longer compile times (~10%) due to additional templates
- ✅ Clean builds faster (only one platform compiled)

**Binary Size**:
- ✅ Same or smaller (only one platform included)
- ✅ Shared code centralized (validation, error handling)

### Compatibility

**Backwards Compatible**:
- Old SAME70 code will be refactored but API stays similar
- Migration helpers provided
- Deprecation warnings for 3 months before removal

**Forward Compatible**:
- Easy to add new platforms (STM32, nRF52, RISC-V)
- Easy to add new boards for existing platforms
- Extensible interface design

**Toolchain**:
- Requires: C++17 (already required)
- No new external dependencies
- Same toolchains (arm-none-eabi-gcc, native gcc, xtensa-esp32-gcc)

### Risk Mitigation

1. **Phased rollout**: Interface → SAME70 → Linux → ESP32 (incremental)
2. **SAME70 first**: Refactor existing platform before adding new ones
3. **Linux testing**: Catch bugs early without hardware
4. **Comprehensive tests**: Unit tests for each platform
5. **Documentation**: Detailed porting guide for adding platforms
6. **Validation**: Strict OpenSpec validation before implementation

### Success Metrics

- ✅ All existing SAME70 tests pass with new architecture
- ✅ New Linux platform enables host-based HAL testing (>90% coverage)
- ✅ Cross-platform example runs on 3+ platforms unchanged
- ✅ Binary size unchanged or smaller for SAME70
- ✅ Documentation covers platform porting (step-by-step guide)
- ✅ CI/CD runs HAL tests on Linux automatically

## Dependencies

- Requires: Existing HAL code (uart, gpio, i2c, spi)
- Blocks: None (additive, can coexist with old code during migration)
- Enables: Multi-platform support, host-based testing, future RISC-V/STM32 ports

## Alternatives Considered

1. **Keep Status Quo (SAME70 Only)**:
   - ❌ No host-based testing (slow development cycle)
   - ❌ Locked to single MCU family
   - ❌ Misses opportunity to learn from LUG's proven patterns

2. **Full LUG Port** (with registries, Device Manager, etc.):
   - ❌ Too complex (+70% code overhead)
   - ❌ See `docs/DEVICE_MANAGER_ANALYSIS.md` for why we rejected this
   - ✅ Cherry-picking platform abstraction only is better

3. **Zephyr/Mbed OS**:
   - ❌ Large external dependency (full RTOS)
   - ❌ Heavyweight for simple projects
   - ❌ Less control over implementation
   - ✅ Alloy custom HAL is more flexible

4. **No Interfaces (Direct Platform #ifdef)**:
   - ❌ Validation logic duplicated in each platform
   - ❌ No compile-time checks
   - ❌ Hard to test (can't mock interfaces)
   - ✅ Interface layer is superior

## Implementation Timeline

- **Week 1-2**: Interface layer (IUart, IGpio, II2c, ISpi) + NVI pattern
- **Week 3-4**: Refactor SAME70 to implement interfaces
- **Week 5-6**: Linux platform implementation + host-based tests
- **Week 7-8**: ESP32 platform implementation
- **Week 9-10**: Documentation, examples, migration guide
- **Total**: 10 weeks (2.5 months)

No migration period needed for new platforms (Linux, ESP32). SAME70 migration happens during Week 3-4.

## References

- LUG Platform Architecture Analysis: `docs/LUG_PLATFORM_ARCHITECTURE_ANALYSIS.md`
- LUG Source Code: `/Users/lgili/Documents/01 - Codes/01 - Github/lug/lugpe-mcal-arm-f292a8612044/lib/platform/`
- Existing Alloy HAL: `src/hal/vendors/atmel/same70/`
- Related OpenSpec: `adopt-lug-patterns` (Template Peripherals, RAII, Circular Buffer)
