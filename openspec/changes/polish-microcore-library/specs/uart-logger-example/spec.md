# Spec: UART Logger Example Quality

## MODIFIED Requirements

### Requirement: uart_logger uses HAL abstractions not raw registers
The uart_logger example MUST demonstrate proper HAL usage, not bypass abstractions.

#### Scenario: Developer reads uart_logger as learning example
```cpp
// examples/uart_logger/main.cpp
#include "board.hpp"
#include "ucore/platform/uart.hpp"

using namespace ucore;
using namespace board;

int main() {
    // ✅ GOOD: Uses HAL abstraction
    auto uart = platform::SimpleUartConfigTxOnly<
        uart::Uart1,
        uart::BaudRate::_115200,
        uart::DataBits::_8,
        uart::StopBits::_1
    >::create();

    if (!uart) {
        // Handle initialization error
        error_led::on();
        while(1);
    }

    uart->write("Hello, MicroCore!\r\n");

    // ❌ BAD: Raw register access (REMOVED)
    // USART1->DR = 'H';  // NO! This bypasses HAL
}
```

**Expected**: No direct register access (USART->DR, GPIO->BSRR, etc.)
**Rationale**: Examples teach best practices, not anti-patterns

#### Scenario: Compile uart_logger for different boards
```bash
$ ./ucore build nucleo_f401re uart_logger  # STM32F4
$ ./ucore build nucleo_g071rb uart_logger  # STM32G0
```

**Expected**: Same code compiles for both, no board-specific #ifdefs
**Rationale**: Demonstrates HAL portability

### Requirement: Example demonstrates error handling
UART initialization errors MUST be handled properly.

#### Scenario: UART initialization fails
```cpp
auto uart_result = SimpleUartConfigTxOnly<...>::create();

if (!uart_result) {
    // Error handling demonstration
    ErrorCode error = uart_result.error();

    switch(error) {
        case ErrorCode::HardwareNotReady:
            // Handle...
            break;
        case ErrorCode::InvalidConfiguration:
            // Handle...
            break;
    }

    while(1); // Halt on error
}

auto uart = uart_result.value();
```

**Expected**: Proper use of Result<T> pattern
**Rationale**: Teaches correct error handling

## ADDED Requirements

### Requirement: Example includes detailed comments
Code MUST explain HAL abstractions for educational purposes.

#### Scenario: Beginner reads uart_logger
```cpp
/**
 * UART Logger Example
 *
 * Demonstrates:
 * - SimpleUartConfigTxOnly abstraction (transmit-only UART)
 * - Result<T> error handling pattern
 * - HAL portability across platforms
 *
 * Hardware Setup:
 * - Connect USB-to-serial adapter to UART TX pin
 * - Board-specific pins defined in board.hpp
 * - 115200 baud, 8N1 configuration
 */
```

**Expected**: Clear educational value
**Rationale**: Examples serve as documentation

### Requirement: Example validated on hardware
Example MUST be tested on actual hardware before release.

#### Scenario: Run example on nucleo_f401re
```bash
$ ./ucore flash nucleo_f401re uart_logger
$ screen /dev/tty.usbmodem* 115200

Hello, MicroCore!
System initialized
Counter: 0
Counter: 1
...
```

**Expected**: Output appears on serial terminal
**Rationale**: Verified working code, not theoretical

## REMOVED Requirements

None. This improves existing example quality.
