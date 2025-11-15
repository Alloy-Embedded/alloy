# UART Logger Example

Demonstrates how to use the Alloy logger system with UART output for embedded systems.

## Overview

This example shows:

- ✅ Board initialization with clock and peripherals
- ✅ UART configuration for debug output (115200 baud)
- ✅ Logger setup with UART sink
- ✅ Multiple log levels (DEBUG, INFO, WARN, ERROR)
- ✅ Formatted logging with timestamps
- ✅ Periodic logging synchronized with LED blinking

## Hardware Requirements

- **Board:** SAME70 Xplained Ultra
- **UART:** Debug UART via EDBG (virtual COM port)
- **Connection:** USB cable to EDBG port (also provides power)
- **Serial Terminal:** 115200 baud, 8N1, no flow control

## Quick Start

### 1. Build

```bash
make
```

### 2. Flash to Board

```bash
make flash
```

### 3. View Output

```bash
make monitor
```

Or use any serial terminal:

```bash
# Using screen
screen /dev/ttyACM0 115200

# Using minicom
minicom -D /dev/ttyACM0 -b 115200

# Using PuTTY (Windows)
putty -serial COM3 -sercfg 115200,8,n,1,N
```

### 4. Build + Flash + Monitor (all in one)

```bash
make run
```

## Expected Output

```
========================================
UART Logger Example Started
Board: SAME70 Xplained Ultra
UART: 115200 baud, 8N1
========================================
[INFO] Loop 0 - Uptime: 1000 ms
[DEBUG] Debug message - loop count is divisible by 5
[INFO] Loop 1 - Uptime: 2000 ms
[INFO] Loop 2 - Uptime: 3000 ms
[INFO] Loop 3 - Uptime: 4000 ms
[INFO] Loop 4 - Uptime: 5000 ms
[DEBUG] Debug message - loop count is divisible by 5
[INFO] Loop 5 - Uptime: 6000 ms
...
[WARN] Warning: Loop count reached 10
...
[ERROR] Error demonstration - this is not a real error!
...
```

The LED will blink once per second, synchronized with the log messages.

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make` | Build the example |
| `make flash` | Build and flash to board |
| `make monitor` | Open serial monitor (115200 baud) |
| `make run` | Flash and immediately open monitor |
| `make clean` | Remove build artifacts |
| `make info` | Show configuration details |

## Serial Port Configuration

By default, the Makefile uses `/dev/ttyACM0`. To use a different port:

```bash
# Linux/macOS
make monitor SERIAL_PORT=/dev/ttyUSB0

# Windows (using Git Bash or WSL)
make monitor SERIAL_PORT=COM3
```

## Troubleshooting

### No output on serial terminal

1. **Check connection:** Ensure USB cable is connected to EDBG port (not native USB)
2. **Check port:** Run `ls /dev/tty*` (Linux/macOS) or check Device Manager (Windows)
3. **Check permissions:** `sudo chmod 666 /dev/ttyACM0` (Linux)
4. **Reset board:** Press reset button or power cycle

### Flash fails

1. **Check OpenOCD:** Ensure OpenOCD is installed and in PATH
2. **Check debugger:** Verify EDBG firmware is up to date
3. **Try alternative:** Use `make flash-jlink` if JLink is available

### Build errors

1. **Check toolchain:** Verify ARM GCC is installed (`arm-none-eabi-gcc --version`)
2. **Check CMake:** Ensure CMake 3.25+ is installed
3. **Clean build:** Run `make clean` then rebuild

## Code Structure

```cpp
// 1. Initialize board
board::init();

// 2. Configure UART (TX-only for logging)
auto uart = Uart<>::quick_setup_tx_only<TxPin>(BaudRate{115200});
uart.initialize();

// 3. Create UART sink for logger
UartSink<decltype(uart)> uart_sink(uart);
Logger::add_sink(&uart_sink);

// 4. Use logger macros
LOG_INFO("Application started");
LOG_DEBUG("Value: %d", value);
LOG_WARN("Warning message");
LOG_ERROR("Error occurred");
```

## Customization

### Change Log Level

Edit `main.cpp`:

```cpp
Logger::set_level(Level::Info);   // Only INFO, WARN, ERROR
Logger::set_level(Level::Debug);  // All levels including DEBUG
Logger::set_level(Level::Warn);   // Only WARN and ERROR
```

### Change UART Pins

For different boards or UART peripherals, modify the pin configuration:

```cpp
// Example: Using UART0 instead of UART1
using UartTxPin = GpioPin<peripherals::PIOA, 10>;  // UART0 TX
using UartPolicy = Same70UartHardwarePolicy<peripherals::UART0>;

auto uart = Uart<PeripheralId::UART0, UartPolicy>::quick_setup_tx_only<UartTxPin>(
    BaudRate{115200}
);
```

### Add Multiple Sinks

You can output logs to multiple destinations:

```cpp
// UART sink
UartSink<decltype(uart)> uart_sink(uart);
Logger::add_sink(&uart_sink);

// File sink (if filesystem available)
FileSink file_sink("/logs/system.log");
Logger::add_sink(&file_sink);

// Memory buffer sink
MemorySink<1024> memory_sink;
Logger::add_sink(&memory_sink);
```

## Performance Notes

- **UART is blocking:** `write_byte()` waits for TX ready
- **Logger is lightweight:** Disabled logs have zero runtime cost
- **Buffer size:** Logger uses 128-byte message buffer
- **Timestamps:** Automatically added via SysTick timer

## Related Examples

- `blink_led` - Basic board initialization and GPIO
- `uart_echo` - UART RX/TX with echo (coming soon)
- `logger_multi_sink` - Multiple logger outputs (coming soon)

## License

Part of the Alloy HAL Framework.
