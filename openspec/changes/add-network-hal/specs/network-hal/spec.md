# Spec: Network HAL

## Scope

This spec covers the contracts (concepts, types, error codes) for the network HAL
layers. It does not specify internal register sequences of the SAME70 GMAC or the
KSZ8081 PHY; those are implementation details of the concrete drivers.

---

## 1. MDIO Bus — `alloy::hal::mdio`

### 1.1 Concept

```cpp
// src/hal/mdio/mdio.hpp
namespace alloy::hal::mdio {

template <typename T>
concept MdioBus = requires(T& bus,
                            std::uint8_t phy, std::uint8_t reg,
                            std::uint16_t value) {
    { bus.read(phy, reg)        } -> std::same_as<core::Result<std::uint16_t, core::ErrorCode>>;
    { bus.write(phy, reg, value)} -> std::same_as<core::Result<void, core::ErrorCode>>;
};

} // namespace alloy::hal::mdio
```

- `phy`: Clause-22 PHY address (0–31).
- `reg`: Clause-22 register address (0–31).
- `read`/`write` return `core::ErrorCode::Timeout` on MDIO busy-timeout.
- Both must be `noexcept`.

### 1.2 `Same70Mdio` driver

```cpp
// src/hal/mdio/same70_mdio.hpp
namespace alloy::hal::mdio {

class Same70Mdio {
public:
    explicit Same70Mdio(std::uint32_t mdc_divider = 4u) noexcept;

    [[nodiscard]] core::Result<std::uint16_t, core::ErrorCode>
    read(std::uint8_t phy, std::uint8_t reg) const noexcept;

    [[nodiscard]] core::Result<void, core::ErrorCode>
    write(std::uint8_t phy, std::uint8_t reg, std::uint16_t value) const noexcept;
};

static_assert(MdioBus<Same70Mdio>);

} // namespace alloy::hal::mdio
```

- Wraps `GMAC->GMAC_NCR` (MPEN bit) and `GMAC->GMAC_MAN` (Clause-22 frame).
- `mdc_divider`: maps to `GMAC_NCFGR.CLK` field; default `0b100` = MCK/64.
  At MCK = 150 MHz this gives MDC = 2.34 MHz (KSZ8081 max is 2.5 MHz).
- Busy-wait on `GMAC_NSR.IDLE` with a 10 000-iteration cap; returns `Timeout` on expiry.
- Does not claim ownership of the GMAC peripheral; `Same70Gmac` (Section 2) also
  touches GMAC registers, so both must be initialised in the correct order
  (GMAC clock first, then `Same70Mdio`, then `Same70Gmac`).

---

## 2. Ethernet MAC — `alloy::hal::ethernet`

### 2.1 Concept

```cpp
// src/hal/ethernet/ethernet_mac.hpp
namespace alloy::hal::ethernet {

template <typename T>
concept EthernetMac = requires(T& mac,
                                std::span<const std::byte> tx,
                                std::span<std::byte>       rx) {
    { mac.send_frame(tx)     } -> std::same_as<core::Result<void, MacError>>;
    { mac.recv_frame(rx)     } -> std::same_as<core::Result<std::size_t, MacError>>;
    { mac.get_mac_address()  } -> std::same_as<std::array<std::uint8_t, 6u>>;
    { mac.link_up()          } -> std::same_as<bool>;
};

enum class MacError : std::uint8_t {
    Busy,          // TX descriptor ring full
    FrameTooLarge, // frame exceeds 1518 B
    NoFrame,       // RX ring empty
    HardwareError,
};

} // namespace alloy::hal::ethernet
```

### 2.2 `Same70Gmac<TxN, RxN>` driver

```cpp
// src/hal/ethernet/same70_gmac.hpp
namespace alloy::hal::ethernet {

template <std::size_t TxN = 4u, std::size_t RxN = 8u>
class Same70Gmac {
public:
    // mac_address: 6-byte EUI-48 (e.g. from AT24MAC402 OUI store).
    explicit Same70Gmac(std::span<const std::uint8_t, 6u> mac_address) noexcept;

    void init() noexcept;  // enables GMAC clock, sets up descriptors

    [[nodiscard]] core::Result<void, MacError>
    send_frame(std::span<const std::byte> frame) noexcept;

    [[nodiscard]] core::Result<std::size_t, MacError>
    recv_frame(std::span<std::byte> buf) noexcept;

    [[nodiscard]] std::array<std::uint8_t, 6u> get_mac_address() const noexcept;

    [[nodiscard]] bool link_up() const noexcept;

private:
    // TX/RX descriptor rings — stored inline, no heap.
    alignas(8) std::array<GmacTxDescriptor, TxN> tx_desc_{};
    alignas(8) std::array<GmacRxDescriptor, RxN> rx_desc_{};
    alignas(8) std::array<std::array<std::byte, 1536u>, TxN> tx_buf_{};
    alignas(8) std::array<std::array<std::byte, 1536u>, RxN> rx_buf_{};
    // ...
};

static_assert(EthernetMac<Same70Gmac<>>);

} // namespace alloy::hal::ethernet
```

- `TxN`/`RxN`: descriptor counts, template params — user trades RAM for throughput.
- Default `TxN=4, RxN=8` ≈ 18 KB of buffer RAM (1536 B × 12 descriptors).
- `init()` must be called after clocks and PHY are up; it starts DMA.
- `link_up()` reads `GMAC_NSR.LINK` — no MDIO access, just register read.
- `send_frame` / `recv_frame` are cooperative (poll-based), not interrupt-driven.
  Interrupt-driven path is a later extension; the cooperative path suffices for
  Modbus TCP at 115 200 baud equivalent data rates.

---

## 3. Network Interface — `alloy::net`

### 3.1 `NetworkInterface` concept

```cpp
// drivers/net/network_interface.hpp
namespace alloy::net {

template <typename T>
concept NetworkInterface = requires(T& iface,
                                     std::span<const std::byte> tx,
                                     std::span<std::byte>       rx) {
    { iface.send_packet(tx) } -> std::same_as<core::Result<void, NetError>>;
    { iface.recv_packet(rx) } -> std::same_as<core::Result<std::size_t, NetError>>;
    { iface.link_up()       } -> std::same_as<bool>;
    { iface.mac_address()   } -> std::same_as<std::array<std::uint8_t, 6u>>;
};

enum class NetError : std::uint8_t {
    Busy, NoPacket, FrameTooLarge, HardwareError, NotConnected,
};

} // namespace alloy::net
```

Intentionally minimal. MAC/PHY details are invisible above this line.

### 3.2 `EthernetInterface<Mac, Phy>` adapter

```cpp
// drivers/net/ethernet_interface.hpp
namespace alloy::net {

template <hal::ethernet::EthernetMac Mac, typename Phy>
class EthernetInterface {
public:
    EthernetInterface(Mac& mac, Phy& phy) noexcept;

    void poll_link() noexcept; // call periodically; runs auto-neg if link changes

    // NetworkInterface concept:
    core::Result<void, NetError>        send_packet(std::span<const std::byte>) noexcept;
    core::Result<std::size_t, NetError> recv_packet(std::span<std::byte>)       noexcept;
    bool                                link_up()    const noexcept;
    std::array<std::uint8_t, 6u>        mac_address() const noexcept;
};

static_assert(NetworkInterface<EthernetInterface<hal::ethernet::Same70Gmac<>, /* PHY */>>);

} // namespace alloy::net
```

`Phy` is constrained to a concept `PhyDriver` (Section 3.3).

### 3.3 `PhyDriver` concept

```cpp
// drivers/net/phy_driver.hpp
namespace alloy::net {

template <typename T>
concept PhyDriver = requires(T& phy) {
    { phy.reset()            } -> std::same_as<core::Result<void, core::ErrorCode>>;
    { phy.auto_negotiate()   } -> std::same_as<core::Result<LinkStatus, core::ErrorCode>>;
    { phy.link_status()      } -> std::same_as<LinkStatus>;
};

struct LinkStatus {
    bool     up        = false;
    bool     full_duplex = false;
    uint16_t speed_mbps  = 0u;   // 10 or 100
};

} // namespace alloy::net
```

The existing `KSZ8081<MdioBus>` driver in `drivers/net/ksz8081/` gains a static
assert `static_assert(PhyDriver<KSZ8081<Same70Mdio>>)` with no API change.

---

## 4. TCP/IP Integration — `LwipAdapter`

### 4.1 lwIP dependency

lwIP is pulled via CMake `FetchContent` at a pinned commit (no submodule).
An Alloy-supplied `lwipopts.h` configures it for no-OS raw API:

```c
// drivers/net/lwip/lwipopts.h  (subset)
#define NO_SYS          1
#define MEM_ALIGNMENT   4
#define LWIP_NETIF_API  0
#define LWIP_TCP        1
#define LWIP_UDP        1
#define LWIP_DHCP       1
#define TCP_MSS         1460
#define LWIP_STATS      0
```

### 4.2 `LwipAdapter<Interface>`

```cpp
// drivers/net/lwip/lwip_adapter.hpp
namespace alloy::net {

template <NetworkInterface Interface>
class LwipAdapter {
public:
    explicit LwipAdapter(Interface& iface) noexcept;

    // Call at startup.
    void init(std::span<const std::uint8_t, 4u> static_ip   = {},
              std::span<const std::uint8_t, 4u> static_mask = {},
              bool use_dhcp = true) noexcept;

    // Call from the main cooperative loop (feeds lwIP's timers and rx queue).
    void poll() noexcept;

    // Returns a TcpStream connected to host:port; blocks until connect or timeout.
    [[nodiscard]] core::Result<TcpStream, NetError>
    connect(std::span<const std::uint8_t, 4u> host,
            std::uint16_t port, std::uint32_t timeout_us) noexcept;

    // Returns a TcpListener on the given port.
    [[nodiscard]] core::Result<TcpListener, NetError>
    listen(std::uint16_t port) noexcept;
};

} // namespace alloy::net
```

### 4.3 `TcpStream`

```cpp
// drivers/net/lwip/tcp_stream.hpp
namespace alloy::net {

class TcpStream {
public:
    // Satisfies alloy::modbus::ByteStream (used directly with Modbus TCP slave/master).
    [[nodiscard]] core::Result<std::size_t, modbus::StreamError>
    read(std::span<std::byte> buf, std::uint32_t timeout_us) noexcept;

    [[nodiscard]] core::Result<void, modbus::StreamError>
    write(std::span<const std::byte> buf) noexcept;

    [[nodiscard]] core::Result<void, modbus::StreamError>
    flush(std::uint32_t timeout_us) noexcept;

    [[nodiscard]] core::Result<void, modbus::StreamError>
    wait_idle(std::uint32_t silence_us) noexcept;
};

static_assert(modbus::ByteStream<TcpStream>);

} // namespace alloy::net
```

`TcpListener::accept(timeout_us) → Result<TcpStream, NetError>` is provided for
the Modbus TCP slave case (passive listen → accept → hand to Slave{tcp_stream, ...}).

---

## 5. WiFi Module Path

All-in-one WiFi modules (W5500 hardware TCP/IP, ESP-AT software stack) implement
`NetworkInterface` directly — they don't split into MAC + PHY.

### 5.1 `W5500Interface`

SPI-attached W5500 (hardware TCP/IP offload). Implements `NetworkInterface`; the
lwIP layer is not needed because the W5500 has its own TCP stack. A thin `TcpStream`
wrapper talks to the W5500 socket registers over SPI.

### 5.2 `EspAtInterface`

UART-AT module (ESP8266/ESP32 running ESP-AT firmware). Implements `NetworkInterface`
via AT commands. `TcpStream` wraps the AT+SEND/+IPD flow. Polling only; no DMA.

### 5.3 Scope gate

W5500 and EspAt drivers are scaffolded (headers + stub .cpp) in this spec phase.
Full implementation is deferred to a follow-on change; this spec just locks the
concept boundary so Ethernet and WiFi are guaranteed interchangeable.

---

## 6. Board wiring — SAME70 Xplained

`boards/same70_xplained/board_ethernet.hpp`:

```cpp
namespace board {

inline auto make_mdio() noexcept {
    return alloy::hal::mdio::Same70Mdio{/* MDC divider = 4 → MCK/64 */};
}

inline auto make_gmac(std::span<const std::uint8_t, 6u> mac) noexcept {
    return alloy::hal::ethernet::Same70Gmac<4u, 8u>{mac};
}

inline auto make_ksz8081(alloy::hal::mdio::Same70Mdio& mdio) noexcept {
    return alloy::net::KSZ8081{mdio, /* phy_addr = 0x00 */};
}

} // namespace board
```

This replaces the hand-rolled `Same70GmacMdio` in the existing KSZ8081 probe example
(`examples/driver_ksz8081_probe/main.cpp`).

---

## 7. Constraints

- No heap (`new`/`malloc`) in any HAL layer or driver. lwIP's `MEM_LIBC_MALLOC` is
  set to 0; its pool allocator is configured with a fixed-size buffer in `.bss`.
- No RTOS. All networking is cooperative (poll in main loop).
- `noexcept` throughout the HAL and driver layers.
- No global mutable state outside of `LwipAdapter` (which owns the lwIP `netif`).
- Descriptor buffers in `Same70Gmac` must be 8-byte aligned (GMAC DMA requirement);
  ensured with `alignas(8)`.
- `TcpStream` satisfies `alloy::modbus::ByteStream`; no specialisation or trait needed.
