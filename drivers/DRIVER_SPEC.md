# Alloy Driver Specification

Rules every driver in `drivers/` must follow. Use `alloyctl new-driver` to scaffold a
conforming stub. The CI job `driver-spec-lint` enforces the machine-checkable rules.

## File layout

```
drivers/<category>/<name>/
    <name>.hpp          # single header, all implementation inline
```

Categories: `sensor`, `display`, `memory`, `net`, `power`.  
One device = one directory. Sub-headers (font tables, calibration math) may live alongside.

## File header

Every driver opens with a block comment:

```cpp
// drivers/<category>/<name>/<name>.hpp
//
// Driver for <Vendor> <PartNumber> <brief description> over <bus>.
// Written against datasheet revision <rev> (<date>).
// Seed driver: <what init covers> + <primary operations>. See drivers/README.md.
```

## Namespace

```cpp
namespace alloy::drivers::<category>::<name> {
    // all public types and the Device class live here
    namespace detail { /* private helpers */ }
}
```

## Config struct

```cpp
struct Config {
    // all fields have defaults — construction via designated initializers
    std::uint16_t address = kDefaultAddress;  // I2C drivers
    // SPI drivers: CsPolicy is a template parameter, not a config field
};
```

No required fields. Every Config must be default-constructible.

## Device class

```cpp
template <typename BusHandle>               // I2C drivers
// or
template <typename BusHandle, typename CsPolicy = NoOpCsPolicy>  // SPI drivers
class Device {
public:
    explicit Device(BusHandle& bus, Config cfg = {})
        : bus_{&bus}, cfg_{cfg} {}

    [[nodiscard]] auto init()   -> core::Result<void, core::ErrorCode>;
    [[nodiscard]] auto read()   -> core::Result<Measurement, core::ErrorCode>;
    // ... other seed operations ...

private:
    BusHandle* bus_;
    Config     cfg_;
    // additional state (calibration data, etc.) goes here
};
```

Rules:
- Store the bus as a **pointer** (`BusHandle*`), never a reference or value.
- `init()` is always the first method called; it must verify the device is present
  (chip-ID check or equivalent) and return `core::ErrorCode::CommunicationError`
  if the device does not respond.
- All fallible methods are `[[nodiscard]]` and return `core::Result<T, core::ErrorCode>`.
- No virtual functions. No inheritance.

## Error handling

| Situation | Return |
|---|---|
| Device not found / chip-ID mismatch | `Err(ErrorCode::CommunicationError)` |
| Bus transfer fails | propagate the transport error unchanged |
| Argument out of range | `Err(ErrorCode::InvalidParameter)` |
| Polling timeout (WIP/BUSY bit) | `Err(ErrorCode::Timeout)` |
| Feature not supported by this device variant | `Err(ErrorCode::NotSupported)` |

Never `throw`. Never `assert`. Never call `std::terminate`.

## Forbidden patterns

| Pattern | Why |
|---|---|
| `new` / `malloc` / `delete` | no heap in embedded drivers |
| `std::string`, `std::vector`, `std::map` | allocating containers |
| `throw` / `try` / `catch` | no exceptions |
| `printf` / `std::cout` | no I/O side effects in drivers |
| `virtual` / inheritance | zero-overhead requirement |
| Global mutable state | thread-safety and re-entrancy |
| `float` in protocol layer | only allowed in physical-unit conversion |

## Required static_assert

Every driver header must end with a concept gate in an anonymous namespace:

```cpp
// Concept gate — fails at include time if the driver no longer satisfies its contract.
namespace {
struct _Mock<Bus>For<Name>Gate {
    // minimal mock implementing the bus concept
};
static_assert(
    /* relevant concept if one exists, otherwise just check Device compiles */
    sizeof(alloy::drivers::<category>::<name>::Device<_Mock<Bus>For<Name>Gate>) > 0,
    "<Name>Device must compile against the documented bus surface");
}  // namespace
```

For `BlockDevice`-implementing drivers use:
```cpp
static_assert(alloy::hal::filesystem::BlockDevice<MyDevice<MockSpi>>, "...");
```

## Required compile test

`tests/compile_tests/test_driver_<name>.cpp` must:
1. Define a `Mock<Bus>` that satisfies the bus surface (write/read/write_read for I2C;
   transfer for SPI; transmit+receive for UART).
2. Call `Device::init()` and at least one primary operation.
3. Be a single translation unit with no `main()` — the build system compiles it as a
   library target to catch API drift.

## Required probe example

`examples/driver_<name>_probe/` must contain:
- `main.cpp` — boots on a real board, calls `init()`, prints result to debug UART.
- `CMakeLists.txt` — follows the pattern in `examples/driver_at24mac402_probe/CMakeLists.txt`.

The probe must:
- Print a clear `PASS` / `FAIL` line to UART.
- Blink slow on PASS, fast on FAIL.
- Name UART prefix as `[<name>]` (e.g. `[sht40] init ok`).

## Bus surface contract

### I2C drivers
The `BusHandle` must provide:
```cpp
write(address, span<const uint8_t>) -> Result<void, ErrorCode>
read(address, span<uint8_t>)        -> Result<void, ErrorCode>
write_read(address, span<const uint8_t> tx, span<uint8_t> rx) -> Result<void, ErrorCode>
```

### SPI drivers
The `BusHandle` must provide:
```cpp
transfer(span<const uint8_t> tx, span<uint8_t> rx) -> Result<void, ErrorCode>
```
CS is managed by the `CsPolicy` template parameter (default `NoOpCsPolicy`).
Use `GpioCsPolicy<Pin>` for software GPIO CS.

### UART drivers
The `BusHandle` must provide:
```cpp
transmit(span<const uint8_t>) -> Result<void, ErrorCode>
receive(span<uint8_t>)        -> Result<void, ErrorCode>
```

## Scaffold

```bash
alloyctl new-driver --name sht40 --interface i2c --category sensor
```

Generates the three required files with correct stubs. Edit the generated header to add
the real register addresses, calibration math, and measurement struct.
