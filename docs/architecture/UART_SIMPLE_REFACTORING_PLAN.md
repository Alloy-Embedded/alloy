# UartSimple Refactoring Plan - Adapted for Current Architecture

**Date**: 2025-11-19
**Phase**: 1.3 - Refactor UartSimple
**Status**: Analysis & Planning

---

## Current Architecture Analysis

### What We Found

The current `uart_simple.hpp` uses a **factory pattern**, not instance methods:

```cpp
// Current pattern: Static factory methods
template <PeripheralId PeriphId, typename HardwarePolicy>
class Uart {
    // Returns configuration object
    template <typename TxPin, typename RxPin>
    static auto quick_setup(BaudRate baud);
};

// Configuration holds state and has methods
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig {
    void write_byte(u8 byte) const;
    void write(const char* str) const;
    // No send(), receive(), flush() yet
};
```

**Key Observations**:
1. `Uart` class is just a namespace for static factory methods
2. `SimpleUartConfig` is the actual instance type with methods
3. Only TX-only methods exist (`write`, `write_byte`)
4. No `send()`, `receive()`, `flush()` methods yet (they'll come from UartBase)

### Implication

The CRTP pattern should be applied to `SimpleUartConfig`, NOT to `Uart`:

```cpp
// CORRECT approach:
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig : public UartBase<SimpleUartConfig<...>> {
    // Inherits send(), receive(), flush() from UartBase
    // Keeps write() convenience method
};
```

---

## Refactoring Strategy

### Phase 1: Make SimpleUartConfig inherit from UartBase

```cpp
template <typename TxPin, typename RxPin, typename HardwarePolicy>
struct SimpleUartConfig : public UartBase<SimpleUartConfig<TxPin, RxPin, HardwarePolicy>> {
    using Base = UartBase<SimpleUartConfig<TxPin, RxPin, HardwarePolicy>>;
    friend Base;

    // Configuration state (unchanged)
    PeripheralId peripheral;
    BaudRate baudrate;
    u8 data_bits;
    UartParity parity;
    u8 stop_bits;
    bool flow_control;

    // Constructor (unchanged)
    constexpr SimpleUartConfig(...) { ... }

    // Inherit UartBase interface
    using Base::send;           // NEW - from UartBase
    using Base::receive;        // NEW - from UartBase
    using Base::flush;          // NEW - from UartBase
    using Base::send_buffer;    // NEW - from UartBase
    using Base::receive_buffer; // NEW - from UartBase
    using Base::available;      // NEW - from UartBase

    // Keep existing methods (for compatibility)
    void write(const char* str) const {
        // Can now delegate to Base::write() or keep implementation
        Base::write(str).expect("Write failed");
    }

    void write_byte(u8 byte) const {
        // Can delegate to Base::send()
        Base::send(static_cast<char>(byte)).expect("Send failed");
    }

private:
    // Implementation methods (called by UartBase via CRTP)
    [[nodiscard]] Result<void, ErrorCode> send_impl(char c) noexcept {
        // Wait for TX ready
        while (!HardwarePolicy::is_tx_ready()) { }
        HardwarePolicy::write_byte(static_cast<u8>(c));
        return Ok();
    }

    [[nodiscard]] Result<char, ErrorCode> receive_impl() noexcept {
        // Wait for RX ready
        while (!HardwarePolicy::is_rx_ready()) { }
        return Ok(static_cast<char>(HardwarePolicy::read_byte()));
    }

    [[nodiscard]] Result<size_t, ErrorCode> send_buffer_impl(
        const char* buffer,
        size_t length
    ) noexcept {
        for (size_t i = 0; i < length; ++i) {
            auto result = send_impl(buffer[i]);
            if (result.is_err()) {
                return Ok(i); // Return bytes sent before error
            }
        }
        return Ok(length);
    }

    [[nodiscard]] Result<size_t, ErrorCode> receive_buffer_impl(
        char* buffer,
        size_t length
    ) noexcept {
        for (size_t i = 0; i < length; ++i) {
            auto result = receive_impl();
            if (result.is_err()) {
                return Ok(i);
            }
            buffer[i] = result.unwrap();
        }
        return Ok(length);
    }

    [[nodiscard]] Result<void, ErrorCode> flush_impl() noexcept {
        // Wait for transmission complete
        while (!HardwarePolicy::is_tx_complete()) { }
        return Ok();
    }

    [[nodiscard]] size_t available_impl() const noexcept {
        return HardwarePolicy::rx_available();
    }

    [[nodiscard]] Result<void, ErrorCode> set_baud_rate_impl(BaudRate baud) noexcept {
        HardwarePolicy::set_baudrate(baud.value());
        baudrate = baud;
        return Ok();
    }
};
```

### Phase 2: Update SimpleUartConfigTxOnly

Similarly, make TX-only version inherit from UartBase:

```cpp
template <typename TxPin, typename HardwarePolicy>
struct SimpleUartConfigTxOnly : public UartBase<SimpleUartConfigTxOnly<TxPin, HardwarePolicy>> {
    // Only implements send-related methods
    // receive_impl() returns error
};
```

---

## Benefits of This Approach

### 1. **Backward Compatibility**
- Existing `write()` and `write_byte()` methods remain
- No breaking changes to user code
- Factory pattern (`quick_setup()`) unchanged

### 2. **New Capabilities**
- Adds `send()`, `receive()`, `flush()` from UartBase
- Adds `send_buffer()`, `receive_buffer()`
- Adds `available()` for non-blocking checks
- All with Result<T, E> error handling

### 3. **Code Reuse**
- Implementation shared via CRTP
- Future UartFluent and UartExpert will share same impl
- Bug fixes propagate automatically

### 4. **Zero Overhead**
- Empty base optimization (sizeof(UartBase) == 1)
- All methods inline
- Same binary size as current code

---

## Migration Checklist

- [ ] Add `#include "hal/api/uart_base.hpp"` to uart_simple.hpp
- [ ] Make `SimpleUartConfig` inherit from `UartBase`
- [ ] Add `friend Base` declaration
- [ ] Implement `*_impl()` methods (send, receive, flush, etc.)
- [ ] Add `using Base::*` declarations for inherited methods
- [ ] Update `write()` to use `Base::write()` or delegate to `send()`
- [ ] Make `SimpleUartConfigTxOnly` inherit from `UartBase`
- [ ] Add HardwarePolicy methods needed (is_rx_ready, read_byte, etc.)
- [ ] Test compilation on all platforms
- [ ] Measure code size

---

## HardwarePolicy Requirements

The refactoring assumes HardwarePolicy provides:

```cpp
class HardwarePolicy {
    static bool is_tx_ready();
    static bool is_rx_ready();
    static bool is_tx_complete();
    static void write_byte(u8 byte);
    static u8 read_byte();
    static size_t rx_available();
    static void set_baudrate(uint32_t baud);
    // ... existing methods
};
```

If these don't exist, we'll need to add them to the platform-specific policies.

---

## Testing Strategy

### 1. Compilation Test
```bash
# Test that it compiles on all platforms
arm-none-eabi-g++ -c uart_simple.hpp -DSTM32F401 -std=c++23
arm-none-eabi-g++ -c uart_simple.hpp -DSAME70Q21 -std=c++23
```

### 2. Size Test
```bash
# Before refactoring
arm-none-eabi-size uart_simple.o

# After refactoring
arm-none-eabi-size uart_simple.o

# Should be same or smaller!
```

### 3. API Compatibility Test
```cpp
// Existing code should still work
auto uart = Uart<PeripheralId::USART0>::quick_setup<TxPin, RxPin>(BaudRate{115200});
uart.write("Hello");  // Still works

// New methods now available
uart.send('A').expect("Failed");
auto c = uart.receive().expect("Failed");
uart.flush().expect("Failed");
```

---

## Next Steps

1. **Check HardwarePolicy** - Verify required methods exist
2. **Refactor SimpleUartConfig** - Add CRTP inheritance
3. **Implement *_impl methods** - Platform-specific logic
4. **Test compilation** - All platforms
5. **Measure code size** - Verify no bloat
6. **Update documentation** - API now has send/receive/flush

---

**Status**: Plan complete, ready to implement
**Next**: Check HardwarePolicy interface, then refactor SimpleUartConfig
