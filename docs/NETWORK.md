# Alloy Network HAL

Hardware-agnostic network support for bare-metal Alloy targets. The stack has five layers that can be used independently or together:

```
Application (Modbus TCP, custom protocol)
  └── TcpStream / UdpSocket       (alloy::net)
        └── LwipAdapter<Iface>    (alloy::net — lwIP no-OS raw API)
              └── NetworkInterface<T>   (concept)
                    ├── EthernetInterface<Mac, Phy>
                    │     ├── Same70Gmac<TxN, RxN>  (alloy::hal::ethernet)
                    │     └── KSZ8081<MdioBus>       (alloy::drivers::net)
                    │           └── Same70Mdio       (alloy::hal::mdio)
                    ├── W5500Interface<SpiHandle>    (stub)
                    └── EspAtInterface<UartHandle>   (stub)
```

All layers are zero-allocation: descriptors and buffers are inline template arrays.

---

## 1. Quick-start: SAME70 Xplained Ethernet

```cpp
#include "boards/same70_xplained/board_ethernet.hpp"
#include "drivers/net/lwip/lwip_adapter.hpp"

// 1. Bring up clocks and pins.
board::enable_ethernet_clocks();
board::mux_ethernet_pins();
board::release_phy_reset();

// 2. MDIO + PHY.
auto mdio = board::make_mdio();
mdio.enable_management();
auto phy = board::make_phy(mdio);
phy.init();                          // soft reset + auto-neg

// 3. MAC.
auto mac_bytes = board::mac_from_eui48();
auto gmac = board::make_gmac(std::span<const std::uint8_t, 6u>{mac_bytes});
gmac.init();

// 4. Network interface + lwIP adapter.
auto eth = board::make_ethernet_interface(gmac, phy);
alloy::net::LwipAdapter<board::BoardEth> adapter{eth};
adapter.init();                      // DHCP by default

// 5. Cooperative main loop.
while (true) {
    eth.poll_link();
    adapter.poll();
    // ... application work ...
}
```

---

## 2. MDIO Bus (`alloy::hal::mdio`)

### Concept

```cpp
template <typename T>
concept MdioBus = requires(T& bus, const T& cbus,
                            std::uint8_t phy, std::uint8_t reg,
                            std::uint16_t value) {
    { cbus.read(phy, reg)        } noexcept -> std::same_as<Result<uint16_t, ErrorCode>>;
    { bus.write(phy, reg, value) } noexcept -> std::same_as<Result<void, ErrorCode>>;
};
```

### `Same70Mdio`

```cpp
#include "src/hal/mdio/same70_mdio.hpp"

alloy::hal::mdio::Same70Mdio mdio{/* mdc_divider_bits */ 0b100u};  // MCK/64
mdio.enable_management();
auto r = mdio.read(0x00, 0x02);   // phy_addr=0, reg=PHY_ID1
```

**MDC clock divider** (`mdc_divider_bits`):

| Value | Divider | MDC @ 150 MHz MCK |
|-------|---------|-------------------|
| 0b000 | MCK/8   | 18.75 MHz — ❌ too fast for most PHYs |
| 0b010 | MCK/32  | 4.69 MHz |
| 0b100 | MCK/64  | 2.34 MHz — ✓ KSZ8081 default |
| 0b101 | MCK/96  | 1.56 MHz — conservative |

The KSZ8081RNA/RND maximum MDC frequency is 2.5 MHz. At 150 MHz MCK use `0b100`.

---

## 3. Ethernet MAC (`alloy::hal::ethernet`)

### Concept

```cpp
template <typename T>
concept EthernetMac = requires(...) {
    { mac.send_frame(span<const byte>) } -> Result<void, MacError>;
    { mac.recv_frame(span<byte>)       } -> Result<size_t, MacError>;
    { mac.get_mac_address()            } -> array<uint8_t, 6>;
    { mac.link_up()                    } -> bool;
};
```

### `Same70Gmac<TxN, RxN>`

```cpp
#include "src/hal/ethernet/same70_gmac.hpp"

alloy::hal::ethernet::Same70Gmac<4, 8> gmac{mac_span};
gmac.init();  // must be called after phy.init() and clocks/pins are up
```

RAM footprint: `(TxN + RxN) * (8 + 1536)` bytes  
Default (4+8): ≈ 18 KB

**Descriptor alignment:** GMAC DMA requires 8-byte aligned descriptors. `Same70Gmac` uses `alignas(8)` on both descriptor and buffer arrays. Place the `Same70Gmac` object in the default `.bss` section; do **not** put it in a section without alignment guarantees.

---

## 4. PHY Driver (`alloy::net::PhyDriver`)

### Concept

```cpp
template <typename T>
concept PhyDriver = requires(T& phy) {
    { phy.reset()          } -> Result<void, ErrorCode>;
    { phy.auto_negotiate() } -> Result<void, ErrorCode>;
    { phy.link_status()    } -> LinkStatus;
};
```

`KSZ8081::Device<MdioBus>` satisfies `PhyDriver`. The wrapper methods `reset()`, `auto_negotiate()`, and `link_status()` delegate to the existing seed driver API.

---

## 5. Network Interface (`alloy::net::NetworkInterface`)

### Concept

```cpp
template <typename T>
concept NetworkInterface = requires(...) {
    { iface.send_packet(span<const byte>) } -> Result<void, NetError>;
    { iface.recv_packet(span<byte>)       } -> Result<size_t, NetError>;
    { iface.link_up()                     } -> bool;
    { iface.mac_address()                 } -> array<uint8_t, 6>;
};
```

### `EthernetInterface<Mac, Phy>`

Wires `EthernetMac + PhyDriver` into `NetworkInterface`. Call `poll_link()` from the main loop.

### WiFi modules (scaffolded)

`W5500Interface<SpiHandle>` and `EspAtInterface<UartHandle>` satisfy the concept but return `HardwareError` until the full driver is implemented. Concept checks pass; hardware interaction is pending.

---

## 6. lwIP Integration (`alloy::net::LwipAdapter`)

### Configuration

`drivers/net/lwip/lwipopts.h` sets lwIP for no-OS cooperative mode:
- `NO_SYS = 1`
- Pool allocator (8 KB `MEM_SIZE`), 16 pbufs
- TCP + UDP + DHCP + ICMP
- No stats, no IPv6

Override any option by defining it before including lwIP headers or by creating a project-local `lwipopts.h` that is found first in the include path.

### `LwipAdapter<Interface>`

```cpp
alloy::net::LwipAdapter<board::BoardEth> adapter{eth};

// DHCP (default):
adapter.init();

// Static IP:
adapter.init({192,168,1,10}, {255,255,255,0}, {192,168,1,1}, /*use_dhcp=*/false);

// Main loop:
adapter.poll();          // feeds RX packets + ticks timers

// Check DHCP:
if (adapter.dhcp_bound()) {
    auto ip = adapter.ip_address();   // array<uint8_t, 4>
}
```

### `TcpStream` (modbus::ByteStream)

```cpp
// Client:
auto r = adapter.connect({192,168,1,5}, 502u, 5'000'000u);
// Server:
auto l = adapter.listen(502u);
auto conn = l.unwrap().accept(1'000'000u);
```

`TcpStream` satisfies `alloy::modbus::ByteStream`:

```cpp
static_assert(alloy::modbus::ByteStream<alloy::net::TcpStream>);
```

Pass it directly to `Slave` or `Master`:

```cpp
alloy::modbus::Slave slave{tcp_stream, slave_id, registry};
while (tcp_stream.connected()) {
    slave.poll(10'000u);
    adapter.poll();
}
```

---

## 7. Modbus TCP Integration

Closes the "pending network HAL" note in `tcp_frame.hpp`. The `TcpStream` wraps an lwIP PCB and satisfies `ByteStream` so the same `Slave<T>` / `Master<T>` templates work over TCP:

| Transport      | Stream type        | Slave / Master                   |
|----------------|--------------------|----------------------------------|
| RS-485 RTU     | `UartStream<H,N>`  | `Slave<UartStream<H,N>>`         |
| Modbus TCP     | `TcpStream`        | `Slave<TcpStream>`               |
| Loopback (test)| `LoopbackEndpoint` | `Slave<LoopbackEndpoint>`        |

See `examples/modbus_tcp_slave/` for a complete working example.

---

## 8. Common Gotchas

**Link-up race:** `eth.poll_link()` is cooperative. If `poll_link()` is not called between `phy.init()` and the first `adapter.poll()`, the EthernetInterface never learns the link came up. Call `poll_link()` from the same loop that calls `adapter.poll()`.

**Descriptor alignment:** `Same70Gmac` uses `alignas(8)` but the linker must place the object correctly. Globals in `.bss` are fine. Stack or heap allocation risks misalignment on some linker configurations.

**MDC clock:** Setting `mdc_divider_bits` too low (fast MDC) corrupts MDIO transactions silently. At 150 MHz MCK, use `0b100` (MCK/64 = 2.34 MHz).

**DHCP timeout:** DHCP requires a connected link partner and a DHCP server on the subnet. If `dhcp_bound()` never returns true, check the cable, the switch port, and whether a DHCP server is reachable.

**lwIP pool exhaustion:** If `pbuf_alloc` returns null in `adapter.poll()`, the RX packet is dropped. Increase `PBUF_POOL_SIZE` or `MEM_SIZE` in `lwipopts.h`.

**Modbus TCP framing:** The TCP slave uses raw `ByteStream` (no MBAP header framing). If you need standard Modbus/TCP MBAP framing (port 502, protocol 0x0000), wrap `TcpStream` in `tcp_frame::encode_tcp_frame` / `decode_tcp_frame`. A full MBAP-aware slave is a future extension.
