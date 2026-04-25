# Add Network HAL

## Why

The SAME70 Xplained board carries a KSZ8081 Ethernet PHY wired to the SAME70 GMAC
peripheral. Today there is no HAL layer for any of that: the `drivers/net/ksz8081/`
seed driver is templated over a user-supplied `MdioBus` type that the application must
hand-roll itself. The Modbus TCP framing layer (`tcp_frame.hpp`) explicitly defers to
"pending network HAL". Any project that wants Ethernet today writes the same boilerplate
from scratch.

More fundamentally, new designs may reach for WiFi (ESP8266/ESP32 over SPI/UART, W5500
all-in-one) instead of hard-wired Ethernet. If the HAL only knows about GMAC + KSZ8081,
WiFi projects rewrite everything from the socket API down.

The fix is a layered, hardware-agnostic network HAL:

1. **MDIO bus** — formalize the management bus that already exists implicitly in the
   KSZ8081 probe (`Same70GmacMdio`). PHY drivers depend only on this contract.
2. **Ethernet MAC driver** — a thin driver for the SAME70 GMAC that exposes raw frame
   send/receive and implements the `EthernetMac` concept. No lwIP dependency.
3. **`NetworkInterface` concept** — hardware-agnostic: an Ethernet MAC + PHY pair and a
   WiFi module both satisfy the same concept. The TCP/IP stack sees one interface type.
4. **TCP/IP integration** — lwIP in no-OS (raw API) mode, fed by `NetworkInterface`.
   Produces a `TcpStream` that satisfies the Modbus `ByteStream` concept, closing the
   Modbus TCP deferred task.
5. **WiFi module path** — initial support for SPI-attached all-in-one modules (W5500,
   WizFi360) and UART-AT modules (ESP-AT), both as `NetworkInterface` implementations.

## What Changes

### `src/hal/mdio/` — MDIO bus HAL

A `MdioBus<T>` concept (22-bit read/write, clause-22) and a `Same70Mdio` concrete
driver wrapping the GMAC's NCR/MAN registers. Satisfies the existing KSZ8081 PHY
dependency without changing the PHY driver.

### `src/hal/ethernet/` — Ethernet MAC HAL

`EthernetMac<T>` concept: `send_frame(span)`, `recv_frame(span) → Result<size_t>`,
`get_mac_address()`, `link_up() → bool`. `Same70Gmac` driver implements it, using
hardware descriptors for TX/RX without a heap allocation; buffer count is a template
parameter.

### `drivers/net/` — Network abstraction

`NetworkInterface<T>` concept: wraps an `EthernetMac` (or a WiFi module) and a
link-state callback. `LwipAdapter<Interface>` wires a `NetworkInterface` to lwIP's
`netif` low-level API, then exposes `TcpStream` and `UdpSocket` on top.

WiFi module drivers (`W5500Interface`, `EspAtInterface`) implement `NetworkInterface`
directly, bypassing the MAC/PHY split for integrated modules.

### `examples/ethernet_basic/` and `examples/modbus_tcp_slave/`

`ethernet_basic`: link up GMAC + KSZ8081, obtain an IP via DHCP, ping reply.
`modbus_tcp_slave`: Modbus slave (same variable registry as RTU examples) over TCP
port 502. Uses `LwipAdapter<Same70GmacInterface>` + `TcpStream` as the ByteStream.

### Documentation

`docs/NETWORK.md` — user guide: interface concept, board wiring, lwIP integration,
TCP stream usage, common gotchas (link-up timing, Ethernet descriptor alignment).

## What Does NOT Change

- The Modbus RTU slave/master — unchanged; existing `ByteStream` contract is reused.
- The KSZ8081 PHY driver — only gains a concrete `MdioBus` to depend on; its public
  API is unchanged.
- lwIP is **not** vendored; it is pulled in as a CMake FetchContent dependency with a
  pinned commit and a thin Alloy config shim (`lwipopts.h`).

## Alternatives Considered

**Lightweight custom TCP stack (uIP, picoTCP):** smaller footprint but no proven
ecosystem. lwIP is the de-facto bare-metal standard; every RTOS port, every NIC driver,
and every Modbus TCP implementation targets it.

**Full socket abstraction first (BSD sockets):** over-engineering for the immediate use
case. `TcpStream` satisfying `ByteStream` is all Modbus TCP needs. A proper BSD-socket
shim can be added later if the project ships a TCP-heavy application.

**WiFi-only (no Ethernet MAC HAL):** the SAME70 Xplained is the reference board and it
has hard-wired Ethernet. WiFi follows the same `NetworkInterface` path, so both are
implemented together rather than splitting the spec.
