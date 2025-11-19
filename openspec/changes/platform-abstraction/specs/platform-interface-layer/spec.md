# Platform Template-Based Abstraction Specification

## ADDED Requirements

### Requirement: Zero Virtual Functions - Template-Only Design
The system SHALL use ONLY templates for platform abstraction with ZERO virtual functions to achieve absolute zero runtime overhead.

#### Scenario: No virtual functions in any HAL interface
- **GIVEN** any HAL peripheral implementation (UART, GPIO, I2C, SPI)
- **WHEN** examining class definition
- **THEN** SHALL contain ZERO virtual methods
- **AND** SHALL contain ZERO vtable pointers
- **AND** all methods SHALL be non-virtual and inlinable

#### Scenario: Compile-time polymorphism only
- **GIVEN** multiple platform implementations (SAME70, Linux, ESP32)
- **WHEN** building for specific platform
- **THEN** platform SHALL be selected via CMake at compile-time
- **AND** only selected platform code SHALL be compiled
- **AND** no runtime polymorphism SHALL exist

#### Scenario: Binary contains no vtables
- **GIVEN** compiled binary for SAME70
- **WHEN** analyzing binary symbols
- **THEN** SHALL contain ZERO vtable symbols
- **AND** SHALL contain ZERO typeinfo symbols
- **AND** binary size SHALL be minimized (no vtable overhead)

### Requirement: Template-Based UART Implementation
The system SHALL provide template-based UART implementations with compile-time peripheral addressing for zero-overhead abstraction.

#### Scenario: UART template with BASE_ADDR parameter
- **GIVEN** UART template class
- **WHEN** defining template: `template <uint32_t BASE_ADDR, uint32_t IRQ_ID>`
- **THEN** BASE_ADDR SHALL be compile-time constant
- **AND** compiler SHALL inline all register accesses
- **AND** no runtime address calculation SHALL occur

#### Scenario: Type aliases for UART instances
- **GIVEN** UART template on SAME70
- **WHEN** defining UART instances
- **THEN** SHALL create type aliases: `using Uart0 = Uart<0x400E0800, ID_UART0>`
- **AND** each alias SHALL be unique type
- **AND** compiler SHALL optimize based on specific BASE_ADDR

#### Scenario: UART methods are fully inlined
- **GIVEN** UART template write() method
- **WHEN** compiled with -O2 optimization
- **THEN** method SHALL be inlined at call site
- **AND** generated assembly SHALL be direct register writes
- **AND** no function call overhead SHALL exist

### Requirement: C++20 Concepts for Interface Validation
The system SHALL use C++20 concepts to validate interfaces at compile-time with zero runtime cost.

#### Scenario: UartConcept defines interface contract
- **GIVEN** UartConcept definition
- **WHEN** checking if type satisfies concept
- **THEN** compiler SHALL validate at compile-time
- **AND** SHALL check for open(), close(), write(), read() methods
- **AND** SHALL check correct return types (Result<T>)
- **AND** no runtime cost SHALL be incurred

#### Scenario: Generic code uses concepts
- **GIVEN** generic function: `template <UartConcept T> void send(T& uart)`
- **WHEN** calling with SAME70::Uart0
- **THEN** compiler SHALL verify Uart0 satisfies UartConcept
- **AND** compilation SHALL fail if interface not satisfied
- **AND** error message SHALL be clear and helpful

#### Scenario: Concepts prevent incorrect usage
- **GIVEN** function expecting UartConcept
- **WHEN** passing non-UART type (e.g., GPIO)
- **THEN** compilation SHALL fail immediately
- **AND** error SHALL indicate concept not satisfied
- **AND** SHALL be caught at compile-time (not runtime)

### Requirement: C++17 Fallback with static_assert
The system SHALL provide C++17 fallback using static_assert when C++20 concepts are not available.

#### Scenario: static_assert validates interface in C++17
- **GIVEN** C++17 compiler (no concepts support)
- **WHEN** defining generic UART function
- **THEN** SHALL use static_assert to check interface
- **AND** SHALL validate method existence at compile-time
- **AND** SHALL provide similar safety to concepts

#### Scenario: Type traits check interface
- **GIVEN** `is_uart_v<T>` type trait
- **WHEN** checking if T is UART-like
- **THEN** SHALL use SFINAE or void_t to detect methods
- **AND** SHALL work in C++17 without concepts
- **AND** SHALL provide compile-time validation

### Requirement: Platform Selection via CMake
The system SHALL select platform implementation at compile-time using CMake variables with only selected platform compiled.

#### Scenario: ALLOY_PLATFORM selects implementation
- **GIVEN** CMake configuration
- **WHEN** setting ALLOY_PLATFORM=same70
- **THEN** only SAME70 platform files SHALL be compiled
- **AND** Linux and ESP32 files SHALL be excluded
- **AND** binary SHALL contain only SAME70 code

#### Scenario: Board header includes correct platform
- **GIVEN** board configuration header
- **WHEN** ALLOY_PLATFORM=linux
- **THEN** board.hpp SHALL include platform/linux/uart.hpp
- **AND** type aliases SHALL use linux::Uart
- **AND** platform selection SHALL be compile-time

#### Scenario: No runtime platform detection
- **GIVEN** compiled application
- **WHEN** running on target
- **THEN** SHALL NOT contain code to detect platform at runtime
- **AND** SHALL NOT have #ifdef in user code
- **AND** all platform selection SHALL be at build time

### Requirement: Template-Based GPIO Implementation
The system SHALL provide template-based GPIO with compile-time pin configuration for zero-overhead bit manipulation.

#### Scenario: GPIO template with PORT and PIN parameters
- **GIVEN** GPIO pin template
- **WHEN** defining: `template <uint32_t PORT_BASE, uint8_t PIN_NUM>`
- **THEN** PORT_BASE SHALL be compile-time constant
- **AND** PIN_NUM SHALL be compile-time constant
- **AND** bit mask SHALL be computed at compile-time

#### Scenario: GPIO operations compile to direct register access
- **GIVEN** GPIO pin write() method
- **WHEN** compiled with optimization
- **THEN** SHALL generate single register write instruction
- **AND** SHALL use compile-time computed bit mask
- **AND** no function call overhead SHALL exist

#### Scenario: Multiple GPIO pins are distinct types
- **GIVEN** `GpioPin<PIOC_BASE, 8>` and `GpioPin<PIOC_BASE, 9>`
- **WHEN** compiler processes types
- **THEN** SHALL be two distinct types
- **AND** cannot accidentally pass wrong pin
- **AND** type system SHALL enforce correctness

### Requirement: CRTP for Static Polymorphism
The system SHALL use Curiously Recurring Template Pattern (CRTP) when inheritance-like behavior is needed without virtual functions.

#### Scenario: Base class with CRTP provides common functionality
- **GIVEN** `template <typename Derived> class UartBase`
- **WHEN** derived class inherits: `class Uart0 : public UartBase<Uart0>`
- **THEN** base class SHALL call derived methods statically
- **AND** no virtual function overhead SHALL exist
- **AND** compiler SHALL inline all calls

#### Scenario: CRTP enables code reuse without vtables
- **GIVEN** common validation logic in UartBase
- **WHEN** calling derived class methods
- **THEN** SHALL reuse base class logic
- **AND** SHALL call derived implementation statically
- **AND** performance SHALL be identical to non-inherited code

### Requirement: Compile-Time Interface Checking
The system SHALL validate that all platform implementations satisfy interface requirements at compile-time.

#### Scenario: Missing method detected at compile-time
- **GIVEN** platform UART implementation missing read() method
- **WHEN** compiling with UartConcept check
- **THEN** compilation SHALL fail
- **AND** error message SHALL indicate missing read() method
- **AND** SHALL be detected before any runtime

#### Scenario: Incorrect return type detected at compile-time
- **GIVEN** platform UART write() returning int instead of Result<size_t>
- **WHEN** compiling with concept/static_assert check
- **THEN** compilation SHALL fail
- **AND** error SHALL indicate incorrect return type
- **AND** SHALL prevent runtime errors

#### Scenario: Parameter mismatch detected at compile-time
- **GIVEN** platform UART write() with wrong parameters
- **WHEN** compiling with interface check
- **THEN** compilation SHALL fail
- **AND** SHALL be caught immediately
- **AND** SHALL prevent subtle bugs

### Requirement: Zero Template Instantiation Overhead
The system SHALL ensure template instantiations do not bloat binary size through careful design and compiler optimizations.

#### Scenario: Common code is shared across instantiations
- **GIVEN** Uart<0x400E0800> and Uart<0x400E0A00>
- **WHEN** analyzing generated code
- **THEN** common methods SHALL be instantiated once
- **AND** only address constants SHALL differ
- **AND** binary size SHALL be minimal

#### Scenario: Unused methods are not instantiated
- **GIVEN** UART template with many methods
- **WHEN** using only open(), write(), close()
- **THEN** compiler SHALL NOT instantiate unused methods
- **AND** binary SHALL contain only used code
- **AND** template bloat SHALL be avoided

#### Scenario: Compiler optimizes away template overhead
- **GIVEN** template-based UART
- **WHEN** comparing binary size to manual register access
- **THEN** sizes SHALL be identical or template version smaller
- **AND** compiler SHALL eliminate abstraction overhead
- **AND** no penalty for using templates

### Requirement: Platform-Specific Type Aliases
The system SHALL provide board-specific type aliases for convenient and type-safe device access.

#### Scenario: Board header defines device aliases
- **GIVEN** SAME70 Xplained board configuration
- **WHEN** defining devices
- **THEN** SHALL provide: `using uart0 = hal::same70::Uart<0x400E0800, ID_UART0>`
- **AND** user code SHALL use: `board::uart0 uart{}`
- **AND** type SHALL be resolved at compile-time

#### Scenario: Type aliases enable platform-agnostic code
- **GIVEN** application using board::uart0
- **WHEN** switching from SAME70 to STM32F4
- **THEN** SHALL only change board configuration header
- **AND** application code SHALL remain unchanged
- **AND** compiler SHALL resolve to new platform type

### Requirement: Result-Based Error Handling
The system SHALL use Alloy's existing Result<T, E> for error handling with no virtual function overhead.

#### Scenario: Template methods return Result
- **GIVEN** template Uart::write() method
- **WHEN** operation completes
- **THEN** SHALL return Result<size_t>
- **AND** Result SHALL be non-virtual template type
- **AND** no overhead compared to direct error codes

#### Scenario: Result enables monadic operations
- **GIVEN** UART write operation returning Result
- **WHEN** using map/and_then/or_else
- **THEN** operations SHALL be inlined
- **AND** no runtime overhead SHALL exist
- **AND** functional composition SHALL be zero-cost

### Requirement: Performance Parity with Direct Register Access
The system SHALL generate identical or better assembly code compared to manual register manipulation.

#### Scenario: Template UART equals manual register access
- **GIVEN** template Uart::write() vs manual UART0->UART_THR = data
- **WHEN** comparing generated assembly with -O2
- **THEN** SHALL generate identical instructions
- **AND** no extra loads/stores SHALL be present
- **AND** register allocation SHALL be optimal

#### Scenario: Template GPIO equals direct bit manipulation
- **GIVEN** template GpioPin::write() vs manual PIO->PIO_SODR = (1 << pin)
- **WHEN** comparing assembly
- **THEN** SHALL generate single instruction
- **AND** bit mask SHALL be compile-time constant
- **AND** no overhead compared to manual code

### Requirement: Documentation and Examples
The system SHALL provide comprehensive documentation explaining template-based design and zero-overhead guarantees.

#### Scenario: Documentation explains zero-overhead design
- **GIVEN** developer reading platform abstraction docs
- **WHEN** learning about template approach
- **THEN** SHALL find explanation of why no virtual functions
- **AND** SHALL see assembly code comparisons
- **AND** SHALL understand performance characteristics

#### Scenario: Examples demonstrate template usage
- **GIVEN** example code for UART
- **WHEN** reading examples
- **THEN** SHALL show template instantiation
- **AND** SHALL show type aliases
- **AND** SHALL show platform-agnostic usage

#### Scenario: Migration guide shows refactoring path
- **GIVEN** existing code with direct register access
- **WHEN** consulting migration guide
- **THEN** SHALL find step-by-step refactoring instructions
- **AND** SHALL show before/after comparisons
- **AND** SHALL demonstrate zero performance loss
