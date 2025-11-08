# Spec: hal-template-peripherals

## Overview
Template-based peripheral implementations providing zero-overhead compile-time peripheral addressing and configuration, eliminating runtime indirection.

## Requirements

### Functional Requirements

**REQ-TP-001**: The system SHALL provide template-based GPIO implementation
- **Rationale**: Compile-time pin configuration eliminates runtime overhead
- **Implementation**: `GpioPin<PORT_BASE, PIN_NUM>` template class

**REQ-TP-002**: The system SHALL provide template-based I2C implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `I2c<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-003**: The system SHALL provide template-based SPI implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `Spi<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-004**: The system SHALL provide template-based Timer implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `Timer<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-005**: The system SHALL provide template-based PWM implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `Pwm<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-006**: The system SHALL provide template-based ADC implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `Adc<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-007**: The system SHALL provide template-based DMA implementation
- **Rationale**: Compile-time peripheral addressing for zero-overhead access
- **Implementation**: `Dma<BASE_ADDR, IRQ_ID>` template class

**REQ-TP-008**: The system SHALL provide type aliases for common peripherals
- **Rationale**: Convenient access without template syntax
- **Implementation**: `using Uart0 = Uart<UART0_BASE, UART0_IRQ>`

### Non-Functional Requirements

**REQ-TP-NF-001**: The implementation SHALL produce identical assembly to direct register access
- **Rationale**: Zero-overhead abstraction principle
- **Implementation**: Fully inlined methods with constexpr where possible

**REQ-TP-NF-002**: The implementation SHALL resolve all addresses at compile-time
- **Rationale**: No runtime address calculation overhead
- **Implementation**: Template parameters for base addresses

**REQ-TP-NF-003**: The implementation SHALL use type-safe bitfield operations
- **Rationale**: Prevents register manipulation errors
- **Implementation**: `BitField<Pos, Width>` template with namespaced constants

**REQ-TP-NF-004**: The implementation SHALL support multiple platforms
- **Rationale**: Platform abstraction without overhead
- **Implementation**: Platform-specific template specializations

## Implementation

### Platforms
Currently implemented for:
- ✅ SAME70 (Atmel/Microchip ARM Cortex-M7)
- ✅ STM32F4 (STMicroelectronics ARM Cortex-M4)

### Files (SAME70 Example)
- `src/hal/platform/same70/gpio.hpp` - Template GPIO
- `src/hal/platform/same70/i2c.hpp` - Template I2C
- `src/hal/platform/same70/spi.hpp` - Template SPI
- `src/hal/platform/same70/timer.hpp` - Template Timer
- `src/hal/platform/same70/pwm.hpp` - Template PWM
- `src/hal/platform/same70/adc.hpp` - Template ADC
- `src/hal/platform/same70/dma.hpp` - Template DMA

### API Surface (Example: I2C)
```cpp
template <uint32_t BASE_ADDR, uint32_t IRQ_ID>
class I2c {
public:
    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t irq_id = IRQ_ID;

    Result<void, ErrorCode> open();
    Result<void, ErrorCode> close();
    Result<void, ErrorCode> setSpeed(I2cSpeed speed);
    Result<size_t, ErrorCode> write(uint8_t addr, const uint8_t* data, size_t size);
    Result<size_t, ErrorCode> read(uint8_t addr, uint8_t* data, size_t size);
};

// Type aliases for convenience
using I2c0 = I2c<TWIHS0_BASE, ID_TWIHS0>;
using I2c1 = I2c<TWIHS1_BASE, ID_TWIHS1>;
```

### Usage Examples
```cpp
// Direct template usage
I2c<TWIHS0_BASE, ID_TWIHS0> i2c;
i2c.open();

// Type alias usage (preferred)
I2c0 i2c;
i2c.open();

// GPIO example
GpioPin<PIOA_BASE, 8> led;
led.setMode(GpioMode::Output);
led.set();

// Compile-time constants
static_assert(I2c0::base_address == TWIHS0_BASE);
static_assert(I2c0::irq_id == ID_TWIHS0);
```

### Benefits Over Runtime Configuration
- **Code size**: 5-10% reduction (no vtables, no runtime dispatch)
- **Performance**: Zero runtime overhead (all addresses compile-time)
- **Type safety**: Compile-time peripheral validation
- **Debugging**: Better compiler error messages

## Testing
Tested on:
- SAME70 Xplained board
- STM32F4 Discovery board
- Build tests verify zero-overhead assembly generation

## Dependencies
- `core/error.hpp` - Error codes
- `core/result.hpp` - Result<T, E> type
- `hal/utils/bitfield.hpp` - Type-safe bitfields
- Platform-specific register definitions

## References
- LUG Framework template peripheral pattern
- Implementation: `src/hal/platform/*/`
- Previous improvements: PMC clock fixes, bitfield standardization
