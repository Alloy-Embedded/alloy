# Design Document: Adopt LUG Framework Patterns

## Context

The Alloy HAL framework currently lacks several production-proven patterns for embedded systems development. After analyzing the legacy LUG framework (15+ commercial products, 50,000+ field hours, <0.1% critical bug rate), we identified key patterns that can significantly improve Alloy's robustness, performance, and developer experience.

### Background
- **Current State**: Basic HAL with raw error codes, manual resource management
- **Problem**: Type safety issues, potential resource conflicts, runtime overhead
- **Opportunity**: Adopt battle-tested patterns from LUG framework
- **Constraints**: Must maintain backwards compatibility during migration, C++17 only

### Stakeholders
- **HAL Developers**: Need type-safe, efficient abstractions
- **Application Developers**: Need clean, intuitive APIs
- **Maintainers**: Need clear patterns and documentation
- **End Users**: Need reliable, bug-free embedded systems

## Goals / Non-Goals

### Goals
- ✅ Introduce Result<T> for type-safe error handling
- ✅ Implement Device Management for resource ownership tracking
- ✅ Create template-based peripherals for zero-overhead abstraction
- ✅ Add RAII wrappers for automatic resource cleanup
- ✅ Provide CircularBuffer for efficient embedded containers
- ✅ Maintain backwards compatibility during migration period
- ✅ Comprehensive documentation and migration guides
- ✅ Zero runtime overhead for new patterns

### Non-Goals
- ❌ Complete rewrite of Alloy (cherry-pick patterns only)
- ❌ Port entire LUG framework (too disruptive)
- ❌ Add external dependencies (keep Alloy standalone)
- ❌ Support C++14 or earlier (requires C++17 features)
- ❌ Implement advanced features immediately (DMA, interrupts) - those come later

## Architecture Overview

### Pattern Dependency Graph

```
┌─────────────────────┐
│   Result<T>         │  ← Foundation (no dependencies)
└──────────┬──────────┘
           │
           ↓
┌─────────────────────┐
│ Device Management   │  ← Uses Result<T> for acquire/release
└──────────┬──────────┘
           │
           ↓
┌─────────────────────┐
│ RAII Wrappers       │  ← Uses Device Management
└──────────┬──────────┘
           │
           ↓
┌─────────────────────┐
│ Template Peripherals│  ← Inherits Device, uses Result<T>
└─────────────────────┘

┌─────────────────────┐
│ Circular Buffer     │  ← Independent (no dependencies)
└─────────────────────┘
```

### Module Structure

```
src/core/
├── result.hpp              # Result<T> implementation
├── device.hpp              # Device base class, SharingPolicy
├── device_registry.hpp     # GlobalRegistry pattern
├── scoped_device.hpp       # RAII wrappers
└── circular_buffer.hpp     # Ring buffer

src/hal/interfaces/
├── uart_interface.hpp      # Updated with Result<T>
├── gpio_interface.hpp      # Updated with Result<T>
├── i2c_interface.hpp       # Updated with Result<T>, ScopedI2c
└── spi_interface.hpp       # Updated with Result<T>, ScopedSpi

src/hal/vendors/atmel/same70/
├── uart.hpp                # Template-based: Uart<BASE, CH>
├── gpio.hpp                # Template-based: Gpio<BASE, PIN>
├── i2c.hpp                 # Template-based: I2c<BASE>
└── spi.hpp                 # Template-based: Spi<BASE>

boards/atmel_same70_xpld/
├── board.hpp               # Board configuration
├── device_ids.hpp          # Device enumerations
└── device_registry.cpp     # Registry initialization
```

## Decisions

### Decision 1: Result<T> over std::expected

**What**: Implement custom Result<T> instead of using C++23 std::expected

**Why**:
- ✅ C++17 compatible (std::expected requires C++23)
- ✅ Tailored to embedded constraints (no exceptions, minimal overhead)
- ✅ Can optimize for our specific use cases
- ✅ No external dependencies

**Alternatives Considered**:
1. **std::expected (C++23)**: Not available in C++17
2. **External library (e.g., tl::expected)**: Adds dependency
3. **std::optional + error_code pair**: Verbose, not ergonomic

**Trade-offs**:
- ⚠️ Custom code to maintain
- ✅ Full control over implementation
- ✅ Can add Alloy-specific features

### Decision 2: Template-Based Peripherals

**What**: Use templates with compile-time constants for peripheral base addresses and channels

**Why**:
- ✅ Zero runtime overhead (constants resolved at compile-time)
- ✅ Type safety (can't confuse UART0 with UART1)
- ✅ Code reuse without performance penalty
- ✅ Enables compiler optimizations

**Implementation**:
```cpp
template <uint32_t BASE_ADDR, uint32_t CHANNEL>
class UartX : public IUart {
    static constexpr volatile Uart* HW =
        reinterpret_cast<volatile Uart*>(BASE_ADDR);

    Result<uint8_t> read() override {
        // Compiler resolves BASE_ADDR and CHANNEL at compile-time
        return HW->read(CHANNEL);
    }
};

// Instantiations (zero overhead)
using Uart0 = UartX<UART0_BASE, 0>;
using Uart1 = UartX<UART1_BASE, 0>;
```

**Alternatives Considered**:
1. **Runtime base address**: Overhead of pointer indirection
2. **Macros**: Not type-safe, hard to debug
3. **Manual duplication**: Code duplication, maintenance burden

### Decision 3: SharingPolicy Enum

**What**: Enum class for device sharing policy (Single vs Shared)

**Why**:
- ✅ Explicit resource sharing semantics
- ✅ Compile-time policy specification
- ✅ Runtime enforcement with reference counting
- ✅ Prevents resource conflicts

**Implementation**:
```cpp
enum class SharingPolicy {
    eSingle,   // Exclusive access (e.g., UART)
    eShared    // Shared access (e.g., GPIO pin for reading)
};

class Device {
    SharingPolicy m_policy;
    size_t m_owners_count{0};

    std::error_code acquire() {
        if (m_policy == SharingPolicy::eSingle && m_owners_count > 0) {
            return make_error_code(std::errc::device_or_resource_busy);
        }
        ++m_owners_count;
        return {};
    }
};
```

**Alternatives Considered**:
1. **No ownership tracking**: Current approach, leads to conflicts
2. **Mutex-based locking**: Runtime overhead, not suitable for all cases
3. **Compile-time only**: Can't detect runtime conflicts

### Decision 4: RAII with Smart Pointers

**What**: Use std::shared_ptr with custom deleters for RAII resource management

**Why**:
- ✅ Automatic cleanup (exception-safe even without exceptions)
- ✅ Reference counting built-in
- ✅ Works with existing C++ patterns
- ✅ Composable with move semantics

**Implementation**:
```cpp
template <typename DeviceType>
class ScopedDevice {
    std::shared_ptr<DeviceType> m_device;

public:
    explicit ScopedDevice(DeviceId id) {
        m_device = getDevice<DeviceType>(id);
    }

    ~ScopedDevice() {
        if (m_device) {
            returnDevice(std::move(m_device));
        }
    }

    // No copy, only move
    ScopedDevice(const ScopedDevice&) = delete;
    ScopedDevice(ScopedDevice&&) = default;

    DeviceType* operator->() { return m_device.get(); }
};
```

**Alternatives Considered**:
1. **Manual cleanup**: Error-prone, especially with early returns
2. **Unique pointers**: Doesn't support shared ownership
3. **Raw pointers**: No automatic cleanup

### Decision 5: Circular Buffer with Fixed Size

**What**: Template-based circular buffer with compile-time size

**Why**:
- ✅ No dynamic allocation (embedded-friendly)
- ✅ Predictable memory usage
- ✅ Cache-friendly (contiguous memory)
- ✅ Fast (no reallocation)

**Implementation**:
```cpp
template<typename T, size_t N>
class CircularBuffer {
    T m_buffer[N];
    size_t m_head{0};
    size_t m_tail{0};
    size_t m_count{0};

    void push(T value) {
        if (full()) pop();
        m_buffer[m_head] = value;
        m_head = (m_head + 1) % N;
        ++m_count;
    }
};
```

**Alternatives Considered**:
1. **std::vector**: Dynamic allocation, not suitable for embedded
2. **std::deque**: Complex, dynamic allocation
3. **std::array + manual index**: Reinventing the wheel

### Decision 6: Non-Virtual Interface (NVI) Pattern

**What**: Public non-virtual methods that call private virtual methods

**Why**:
- ✅ Validation at interface level (don't duplicate in each driver)
- ✅ Template Method pattern (algorithm in base, details in derived)
- ✅ Smaller vtables (fewer virtual methods)
- ✅ Cleaner driver implementations

**Implementation**:
```cpp
class IUart : public Device {
public:
    // Public non-virtual interface
    Result<uint8_t> read() {
        if (!isOpened()) {
            return make_error_code(std::errc::operation_not_permitted);
        }
        return drvRead();  // Call virtual implementation
    }

private:
    // Virtual implementation (drivers override these)
    virtual Result<uint8_t> drvRead() = 0;
};
```

**Alternatives Considered**:
1. **All virtual**: Validation duplicated in each driver
2. **All non-virtual**: Can't override behavior
3. **Mixed without pattern**: Inconsistent, confusing

## Implementation Details

### Phase 1: Core Infrastructure (Week 1-2)

#### Result<T> Implementation

**File**: `src/core/result.hpp`

**Key Features**:
```cpp
template <typename T>
class Result {
    std::optional<T> m_value;
    std::error_code m_error;

public:
    // Construction
    Result(T value) : m_value(std::move(value)) {}
    Result(std::error_code ec) : m_error(ec) {}

    // Boolean conversion
    explicit operator bool() const { return m_value.has_value(); }

    // Value access
    T& value() { return *m_value; }
    T operator*() const { return value(); }

    // Error access
    std::error_code error() const { return m_error; }

    // Structured binding support (C++17)
    template <size_t I>
    decltype(auto) get() const {
        if constexpr (I == 0) return m_value;
        else if constexpr (I == 1) return m_error;
    }
};
```

**Usage Examples**:
```cpp
// Simple check
Result<uint8_t> result = uart.read();
if (result) {
    uint8_t byte = *result;
    process(byte);
}

// Structured binding (C++17)
auto [value, error] = uart.read();
if (value) {
    process(*value);
} else {
    log_error(error);
}

// Error propagation
Result<Data> read_sensor() {
    auto [value, error] = i2c.read();
    if (!value) return error;
    return parse(*value);
}
```

#### Device Management Implementation

**File**: `src/core/device.hpp`

**Key Classes**:
```cpp
enum class SharingPolicy { eSingle, eShared };

class Device {
protected:
    SharingPolicy m_policy;
    size_t m_owners_count{0};
    bool m_opened{false};

public:
    explicit Device(SharingPolicy policy) : m_policy(policy) {}
    virtual ~Device() = default;

    std::error_code acquire();
    std::error_code release();

    bool isOpened() const { return m_opened; }
    size_t ownersCount() const { return m_owners_count; }
};
```

**File**: `src/core/device_registry.hpp`

```cpp
template <typename ObjectType, typename IdType>
class GlobalRegistry {
    static inline std::map<IdType, std::shared_ptr<ObjectType>> m_instances;

public:
    static void init(std::vector<std::pair<IdType, std::shared_ptr<ObjectType>>> instances);
    static std::shared_ptr<ObjectType> get(const IdType& id);
};

// Board-specific device IDs
enum class Same70DeviceId {
    eUart0, eUart1, eUart2,
    eLedGreen, eLedRed,
    eI2c0, eI2c1,
    eSpi0
};

// Factory function
template <typename T>
std::shared_ptr<T> getDevice(Same70DeviceId id);
```

### Phase 2: HAL Updates (Week 3-4)

#### Template-Based UART

**File**: `src/hal/vendors/atmel/same70/uart.hpp`

```cpp
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class UartX : public IUart {
    static constexpr volatile Uart* HW =
        reinterpret_cast<volatile Uart*>(BASE_ADDR);

public:
    UartX() : IUart(SharingPolicy::eSingle) {}

    Result<void> drvOpen(uint32_t baudrate) override {
        // Configure using compile-time BASE_ADDR
        HW->UART_BRGR = calculate_baud(baudrate);
        HW->UART_MR = UART_MR_PAR_NO | UART_MR_CHMODE_NORMAL;
        HW->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
        return {};
    }

    Result<uint8_t> drvRead() override {
        if (!(HW->UART_SR & UART_SR_RXRDY)) {
            return make_error_code(std::errc::resource_unavailable_try_again);
        }
        return static_cast<uint8_t>(HW->UART_RHR);
    }
};

// Type aliases (zero overhead)
using Uart0 = UartX<0x400E0800, ID_UART0>;
using Uart1 = UartX<0x400E0A00, ID_UART1>;
```

#### RAII Wrappers

**File**: `src/core/scoped_device.hpp`

```cpp
template <typename DeviceType>
class ScopedDevice {
    std::shared_ptr<DeviceType> m_device;

public:
    explicit ScopedDevice(DeviceId id) {
        m_device = getDevice<DeviceType>(id);
    }

    ~ScopedDevice() {
        if (m_device) {
            returnDevice(std::move(m_device));
        }
    }

    // No copy, only move
    ScopedDevice(const ScopedDevice&) = delete;
    ScopedDevice& operator=(const ScopedDevice&) = delete;
    ScopedDevice(ScopedDevice&&) = default;
    ScopedDevice& operator=(ScopedDevice&&) = default;

    DeviceType* operator->() { return m_device.get(); }
    DeviceType& operator*() { return *m_device; }
};

// Bus locking for I2C/SPI
class ScopedI2c {
    std::shared_ptr<II2c> m_i2c;
    bool m_locked{false};

public:
    explicit ScopedI2c(std::shared_ptr<II2c> i2c, uint32_t timeout_ms)
        : m_i2c(i2c) {
        acquire(timeout_ms);
    }

    ~ScopedI2c() { release(); }

    II2c& get() { return *m_i2c; }

private:
    void acquire(uint32_t timeout_ms);
    void release();
};
```

### Phase 3: Memory Optimization (Week 5-6)

#### Circular Buffer

**File**: `src/core/circular_buffer.hpp`

```cpp
template<typename T, size_t N>
class CircularBuffer {
    T m_buffer[N];
    size_t m_head{0};
    size_t m_tail{0};
    size_t m_count{0};

public:
    void push(const T& value) {
        if (full()) pop();
        m_buffer[m_head] = value;
        m_head = (m_head + 1) % N;
        ++m_count;
    }

    T pop() {
        if (empty()) {
            // Handle error (could use Result<T>)
        }
        T value = m_buffer[m_tail];
        m_tail = (m_tail + 1) % N;
        --m_count;
        return value;
    }

    bool empty() const { return m_count == 0; }
    bool full() const { return m_count == N; }
    size_t size() const { return m_count; }
    size_t capacity() const { return N; }

    // Iterator support
    class iterator { /* ... */ };
    iterator begin() { return iterator(this, m_tail); }
    iterator end() { return iterator(this, m_head); }
};
```

## Migration Plan

### Phase 1: Compatibility Layer (Weeks 1-2)

1. **Add new Result-based APIs** alongside existing APIs
2. **Mark old APIs as deprecated** with compiler warnings
3. **Update documentation** with migration examples
4. **Provide conversion helpers** (e.g., `result_to_int()`)

Example:
```cpp
class IUart {
public:
    // New API (preferred)
    Result<uint8_t> read();

    // Old API (deprecated, compatibility)
    [[deprecated("Use read() returning Result<uint8_t>")]]
    int read_legacy(uint8_t* data);
};
```

### Phase 2: Migration Period (Weeks 3-14)

1. **12-week migration window** for users to update code
2. **Compiler warnings** guide users to new APIs
3. **Migration guide** with examples for all common patterns
4. **Office hours** for questions and support

### Phase 3: Cleanup (Week 15+)

1. **Remove deprecated APIs** in next major version
2. **Update all examples** to use new patterns exclusively
3. **Archive migration guide** for reference

### Rollback Plan

If critical issues arise:
1. **Keep compatibility layer** indefinitely
2. **Make new APIs opt-in** via compile flag
3. **Document known issues** and workarounds
4. **Fix issues** before removing old APIs

## Risks / Trade-offs

### Risk 1: Learning Curve
**Risk**: Developers unfamiliar with Result<T> and RAII patterns
**Mitigation**:
- Comprehensive documentation with examples
- Migration guide with before/after code
- Office hours for questions
- Gradual adoption (both APIs available)

### Risk 2: Compilation Time
**Risk**: Template-heavy code may increase compile times
**Mitigation**:
- Use extern templates where appropriate
- Measure compile time before/after
- Optimize if >10% regression
- Consider precompiled headers

### Risk 3: Binary Size
**Risk**: Template instantiations may increase code size
**Mitigation**:
- Measure binary size before/after
- Use `if constexpr` to eliminate dead code
- Link-time optimization (LTO)
- Expect 5-10% *reduction* due to better inlining

### Risk 4: Backwards Compatibility
**Risk**: Breaking changes affect existing projects
**Mitigation**:
- 12-week migration period with both APIs
- Deprecation warnings guide migration
- Automated migration scripts where possible
- Major version bump for removal

### Trade-off 1: Complexity vs Safety
**Trade-off**: More sophisticated patterns = more complexity
**Decision**: Accept complexity for safety and robustness
**Rationale**: Production-proven patterns reduce bugs long-term

### Trade-off 2: Performance vs Abstraction
**Trade-off**: Templates may be harder to debug
**Decision**: Zero-overhead abstractions worth the trade-off
**Rationale**: No runtime cost, benefits outweigh debugging complexity

### Trade-off 3: Custom vs Standard
**Trade-off**: Custom Result<T> vs waiting for C++23 std::expected
**Decision**: Custom implementation for C++17 compatibility
**Rationale**: Can't wait 2-3 years for C++23 adoption

## Performance Analysis

### Result<T> Overhead

**Theoretical**:
- Size: `sizeof(std::optional<T>) + sizeof(std::error_code)` = ~12-16 bytes
- Zero runtime overhead vs raw error codes (compiler optimizes)

**Benchmarks** (Expected):
```
Operation           | Raw int | Result<T> | Overhead
--------------------|---------|-----------|----------
UART read success   | 150ns   | 150ns     | 0%
UART read error     | 50ns    | 50ns      | 0%
Binary size (UART)  | 2.4KB   | 2.4KB     | 0%
```

### Template Peripherals Overhead

**Theoretical**:
- Compile-time: Constants folded, addresses resolved
- Runtime: Zero overhead (same as raw register access)
- Binary size: Potentially smaller (better inlining)

**Benchmarks** (Expected):
```
Approach              | Code Size | Performance
----------------------|-----------|-------------
Raw register access   | 100%      | 100%
Runtime base address  | 105%      | 95%
Template-based        | 98%       | 100%
```

### RAII Wrappers Overhead

**Theoretical**:
- Destructor call is inlined
- Smart pointer has same cost as raw pointer
- Move semantics eliminate copies

**Benchmarks** (Expected):
```
Approach              | Code Size | Performance
----------------------|-----------|-------------
Manual cleanup        | 100%      | 100%
ScopedDevice          | 100%      | 100%
```

### Circular Buffer Overhead

**Theoretical**:
- Fixed size = no allocation
- Modulo operation for wrap-around
- Cache-friendly (contiguous memory)

**Benchmarks** (Expected):
```
Operation           | std::vector | CircularBuffer | Speedup
--------------------|-------------|----------------|--------
Push (no alloc)     | 50ns        | 30ns           | 1.67x
Push (with alloc)   | 500ns       | 30ns           | 16.7x
Pop                 | 40ns        | 25ns           | 1.6x
Memory usage        | Dynamic     | Fixed          | Predictable
```

## Testing Strategy

### Unit Tests

**Core Infrastructure**:
- [ ] `test_result.cpp` - All Result<T> operations and edge cases
- [ ] `test_device.cpp` - Device ownership and sharing
- [ ] `test_device_registry.cpp` - Registry operations
- [ ] `test_scoped_device.cpp` - RAII cleanup behavior
- [ ] `test_circular_buffer.cpp` - Buffer operations

**HAL Integration**:
- [ ] `test_uart_result.cpp` - UART with Result<T>
- [ ] `test_uart_template.cpp` - Template-based UART
- [ ] `test_gpio_template.cpp` - Template-based GPIO
- [ ] `test_scoped_i2c.cpp` - I2C bus locking

### Integration Tests

- [ ] `test_device_conflict.cpp` - Detect resource conflicts
- [ ] `test_migration_compat.cpp` - Old/new API compatibility
- [ ] `test_example_blink.cpp` - Updated blink example
- [ ] `test_example_uart_echo.cpp` - UART echo with new patterns

### Performance Tests

- [ ] Benchmark Result<T> vs raw error codes
- [ ] Benchmark template peripherals vs runtime config
- [ ] Measure binary size before/after
- [ ] Measure compile time before/after

### Validation

- [ ] All tests pass with new APIs
- [ ] All tests pass with old APIs (during migration)
- [ ] No resource leaks (Valgrind, static analysis)
- [ ] Performance regression <1%
- [ ] Binary size reduction 5-10%

## Documentation Requirements

### User-Facing Documentation

1. **Pattern Guide** (`docs/PATTERNS.md`):
   - Overview of each pattern
   - When to use each pattern
   - Code examples
   - Best practices

2. **Migration Guide** (`docs/MIGRATION.md`):
   - Breaking changes summary
   - Before/after examples
   - Common patterns
   - Troubleshooting

3. **API Reference**:
   - Result<T> full API
   - Device Management API
   - RAII wrappers API
   - Circular Buffer API

### Developer Documentation

1. **Architecture Decision Records** (ADRs):
   - Why Result<T> over std::expected
   - Why template-based peripherals
   - Why custom implementations

2. **Implementation Notes**:
   - Template instantiation strategy
   - Binary size optimization techniques
   - Performance tuning tips

## Open Questions

1. **Q**: Should we support C++14?
   **A**: No - Result<T> structured binding requires C++17. Decision: C++17 minimum.

2. **Q**: Should we use exceptions for critical errors?
   **A**: No - embedded systems typically disable exceptions. Decision: Result<T> only.

3. **Q**: Should Device Registry be thread-safe?
   **A**: Deferred - single-threaded for now, can add mutex later if RTOS support needed.

4. **Q**: Should we provide std::expected compatibility?
   **A**: Future - can add conversion utilities when C++23 is available.

5. **Q**: Should template peripherals support runtime configuration?
   **A**: Partial - compile-time for base address, runtime for settings like baudrate.

## Success Criteria

1. ✅ All existing tests pass with new APIs
2. ✅ Migration guide covers 100% of breaking changes
3. ✅ Performance regression <1% (ideally 5-10% improvement)
4. ✅ Binary size reduction 5-10%
5. ✅ Zero resource leaks (static analysis)
6. ✅ Documentation coverage >90%
7. ✅ All examples updated
8. ✅ Positive feedback from early adopters

## Approval Checklist

- [ ] Proposal reviewed and approved
- [ ] Design reviewed and approved
- [ ] Breaking changes impact assessed
- [ ] Migration plan agreed upon
- [ ] Testing strategy approved
- [ ] Documentation requirements clear
- [ ] Performance targets defined
- [ ] Timeline realistic

---

**Document Status**: Draft for Review
**Last Updated**: 2025-01-06
**Author**: Alloy Team (based on LUG framework analysis)
