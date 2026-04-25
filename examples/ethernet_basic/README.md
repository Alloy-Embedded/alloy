# ethernet_basic

Link up the SAME70 Xplained Ultra Ethernet port, obtain an IP via DHCP, and reply to pings.

## Hardware

| Signal   | SAME70 Pin | Connector |
|----------|------------|-----------|
| MDC      | PD8        | J6 (on-board, no external wiring needed) |
| MDIO     | PD9        | J6 |
| ETH_TX*  | PD0–PD5    | J6 |
| ETH_RX*  | PD6–PD7, PD11–PD14 | J6 |
| PHY nRST | PC10       | on-board |

Connect a standard RJ-45 Ethernet cable from J6 to a switch or router with DHCP.

## Build

```bash
alloy build --board same70_xplained --target ethernet_basic
```

> Requires `ALLOY_BUILD_NETWORK_LWIP=ON` (default). lwIP is fetched automatically via CMake FetchContent.

## Flash

```bash
alloy flash --board same70_xplained --target ethernet_basic
alloy monitor --board same70_xplained
```

## Expected output

```
ethernet_basic: ready
ethernet_basic: PHY init ok — link negotiating ...
ethernet_basic: link UP 100 Mbps full duplex
ethernet_basic: DHCP bound — IP 192.168.1.42
```

LED blinks at 500 ms once the stack is up. Ping from your PC:

```bash
ping 192.168.1.42
```

## Architecture

```
board::make_mdio()           → Same70Mdio        (alloy::hal::mdio)
board::make_phy(mdio)        → KSZ8081::Device   (alloy::drivers::net::ksz8081)
board::make_gmac(mac)        → Same70Gmac<4,8>   (alloy::hal::ethernet)
board::make_ethernet_interface(gmac, phy)
                             → EthernetInterface (alloy::net)
LwipAdapter<EthernetInterface> adapter{eth}
                             → TcpStream / DHCP  (alloy::net + lwIP)
```

See [docs/NETWORK.md](../../docs/NETWORK.md) for the full guide.
