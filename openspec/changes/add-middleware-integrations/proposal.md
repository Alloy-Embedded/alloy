# Proposal: Middleware Integrations

## Status
`open` — required for real-world firmware (networking, TLS, RTOS, USB stack).

## Problem

alloy provides a clean HAL but no integration points for the middleware that
real firmware needs: TCP/IP stack, TLS, RTOS, USB device stack, CAN protocol
stack, printf/logging. Users must wire these manually, writing glue code that
is untested and often broken (wrong DMA alignment, wrong callback signature,
wrong heap configuration).

The HAL is designed for zero-overhead; middleware integrations must preserve that
property — no heap allocations imposed by the glue layer.

## Proposed Solution

### Integration pattern: `alloy::middleware::<stack>` namespace

Each integration lives in a separate header-only library under
`src/middleware/<stack>/`. It is **opt-in** — user includes the header and
provides the required HAL handles. No middleware is linked by default.

### lwIP (TCP/IP stack)

```cpp
// src/middleware/lwip/alloy_netif.hpp
#include "alloy/hal/eth/eth_handle.hpp"
#include <lwip/netif.h>

namespace alloy::middleware::lwip {

/// Binds an alloy eth_handle to a lwIP netif.
/// Called once at init; lwIP calls the output function via the netif.
template <typename EthHandle>
auto init_netif(netif& iface, EthHandle& eth,
                ip4_addr_t ip, ip4_addr_t mask, ip4_addr_t gw)
    -> core::Result<void, core::ErrorCode>;

/// Call from EthHandle RX ISR / DMA callback to push a received frame into lwIP.
template <typename EthHandle>
auto eth_input(netif& iface, EthHandle& eth) -> void;

}
```

Provides: Ethernet `netif` driver, PBUF-to-DMA buffer bridge.
Requires: `lwip` CMake target from lwip submodule.

### mbedTLS (TLS/cryptography)

```cpp
// src/middleware/mbedtls/alloy_entropy.hpp
namespace alloy::middleware::mbedtls {

/// Registers alloy RNG (hardware TRNG or PRNG) as mbedTLS entropy source.
template <typename RngHandle>
auto register_entropy(mbedtls_entropy_context& ctx, RngHandle& rng) -> int;

}
```

Provides: entropy source from hardware TRNG, time callback from SysTick.

### FreeRTOS

```cpp
// src/middleware/freertos/alloy_heap.hpp
namespace alloy::middleware::freertos {

/// Uses a statically-declared array as the FreeRTOS heap (heap_5).
/// Call before vTaskStartScheduler().
template <size_t HeapBytes>
struct StaticHeap {
    static void install();  // calls vPortDefineHeapRegions
private:
    alignas(8) static std::array<uint8_t, HeapBytes> _buf;
};

}
```

Provides: static heap configuration, SysTick tick hook, assert/panic hooks.

### SEGGER RTT (debug logging)

```cpp
// src/middleware/segger_rtt/alloy_rtt_log.hpp
namespace alloy::middleware::segger_rtt {

/// Zero-copy log sink writing to RTT channel 0.
struct RttSink {
    void write(std::string_view msg);  // wraps SEGGER_RTT_Write
};

}
```

### TinyUSB (USB device stack)

```cpp
// src/middleware/tinyusb/alloy_usb_hal.hpp
namespace alloy::middleware::tinyusb {

/// Provides tinyusb dcd_ port implementation backed by alloy USB HAL.
template <typename UsbHandle>
void init(UsbHandle& usb);

}
```

### CANopen (CiA 301)

```cpp
// src/middleware/canopen/alloy_can_transport.hpp
namespace alloy::middleware::canopen {

template <typename CanHandle>
struct AlloyCanTransport {
    // Implements canopen-stack CO_IF_DRV interface
};

}
```

### CMake integration

Each integration is a CMake interface target:

```cmake
# User CMakeLists.txt:
find_package(lwip REQUIRED)
target_link_libraries(my_firmware
    alloy::hal
    alloy::middleware::lwip   # pulls in alloy_netif.hpp
)
```

### Integration test matrix (host-based)

Each middleware integration has a compile-only smoke test in
`tests/compile_tests/middleware/` that verifies the glue headers compile
against mock HAL handles.

## Non-goals

- alloy does not vendor/fork any middleware library.
- alloy does not provide RTOS-level abstractions (tasks, semaphores) — only the
  glue to map alloy HAL → middleware interface.
