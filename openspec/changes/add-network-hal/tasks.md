# Tasks: Network HAL

Tasks are ordered. Each phase leaves the tree in a working state and is independently
mergeable. Phases 1–4 are host-testable (concept checks + unit tests). Phases 5–8
require the SAME70 Xplained board.

## 1. MDIO HAL

- [x] 1.1 Create `src/hal/mdio/mdio.hpp`: `MdioBus<T>` concept with `read(phy, reg)`
      and `write(phy, reg, value)`, both returning `core::Result`. `noexcept`.
      No hardware dependency — pure concept header.
- [x] 1.2 Implement `src/hal/mdio/same70_mdio.hpp`: `Same70Mdio` concrete driver
      wrapping GMAC NCR/NCFGR/NSR/MAN registers. MDC divider configurable (default
      MCK/64). Busy-wait on `GMAC_NSR.IDLE` with 10 000-iteration cap → `Timeout`.
      `static_assert(MdioBus<Same70Mdio>)`.
- [x] 1.3 Added `PhyDriver` wrapper methods to `drivers/net/ksz8081/ksz8081.hpp`
      (`reset()`, `auto_negotiate()`, `link_status()`) and a `static_assert`
      confirming `Device<FakeBus>` satisfies `PhyDriver`. Concept is structurally
      compatible with the existing template parameter — no API removed.
- [x] 1.4 `tests/compile_tests/test_hal_mdio_concept.cpp`: concept-only compile check
      verifying a fake bus satisfies `MdioBus<T>`.

## 2. Ethernet MAC HAL

- [x] 2.1 Create `src/hal/ethernet/ethernet_mac.hpp`: `EthernetMac<T>` concept
      (`send_frame`, `recv_frame`, `get_mac_address`, `link_up`) and `MacError` enum.
- [x] 2.2 Implement `src/hal/ethernet/same70_gmac.hpp`: `Same70Gmac<TxN, RxN>` with
      inline aligned descriptor + buffer arrays (no heap). `init()`, `send_frame()`,
      `recv_frame()`, `get_mac_address()`, `link_up()`. Cooperative poll only.
      Header-only (matches HAL pattern); `static_assert` via template variable.
- [x] 2.3 `tests/compile_tests/test_hal_ethernet_mac_concept.cpp`: concept-only
      compile check with a fake MAC struct.

## 3. Network Interface abstraction

- [x] 3.1 Create `drivers/net/network_interface.hpp`: `NetworkInterface<T>` concept
      (`send_packet`, `recv_packet`, `link_up`, `mac_address`) and `NetError` enum.
- [x] 3.2 Create `drivers/net/phy_driver.hpp`: `PhyDriver<T>` concept (`reset`,
      `auto_negotiate`, `link_status`) and `LinkStatus` struct.
- [x] 3.3 Implement `drivers/net/ethernet_interface.hpp`:
      `EthernetInterface<Mac, Phy>` adapter wrapping an `EthernetMac` and a `PhyDriver`.
      Includes `poll_link()` for cooperative link-state detection.
      `static_assert(NetworkInterface<EthernetInterface<FakeMac, FakePhy>>)`.
- [x] 3.4 `drivers/net/ksz8081/ksz8081.hpp`: `PhyDriver` wrappers added and
      `static_assert(PhyDriver<Device<_FakeBus>>)` at namespace scope.
- [x] 3.5 `tests/compile_tests/test_net_interface_concept.cpp` and
      `test_net_phy_concept.cpp`: concept-only compile checks.

## 4. lwIP integration

- [x] 4.1 `drivers/net/CMakeLists.txt`: `alloy::net` INTERFACE target (concept headers)
      + `alloy::net_lwip` STATIC target (TcpStream + LwipAdapter). lwIP 2.2.0 fetched
      via `FetchContent` with `GIT_TAG STABLE-2_2_0_RELEASE`. `lwipopts.h` in
      `drivers/net/lwip/` configures no-OS raw API, pool allocator, TCP/UDP/DHCP.
      `drivers/CMakeLists.txt` updated to `add_subdirectory(net)`.
- [x] 4.2 `drivers/net/lwip/lwip_adapter.hpp`: `LwipAdapter<Interface>` with `init()`,
      `poll()`, `connect()`, `listen()`. Implementation gated on `ALLOY_LWIP_AVAILABLE`.
      `drivers/net/lwip/tcp_stream.cpp`: lwIP recv callback, push/pop ring buffer.
- [x] 4.3 `drivers/net/lwip/tcp_stream.hpp`: `TcpStream` satisfying
      `alloy::modbus::ByteStream`. `static_assert(modbus::ByteStream<TcpStream>)`.
- [x] 4.4 `drivers/net/lwip/tcp_listener.hpp` / `tcp_listener.cpp`: `TcpListener` with
      `accept(timeout_us) → Result<TcpStream, NetError>`.
- [x] 4.5 `tests/compile_tests/test_net_tcp_stream_concept.cpp`: `MockTcpStream`
      satisfies `modbus::ByteStream` (no lwIP dependency).
- [x] 4.6 `tests/net/test_lwip_loopback.cpp`: host-side lwIP loopback test
      (LWIP_HAVE_LOOPIF). Two test cases: TcpListener accept + round-trip message,
      TcpStream ByteStream concept assertion. Registered in `drivers/net/CMakeLists.txt`
      as `test_net_lwip_loopback` CTest target.

## 5. WiFi module stubs

- [x] 5.1 `drivers/net/w5500/w5500_interface.hpp`: stub `W5500Interface<SpiHandle>`
      implementing `NetworkInterface`. Returns `HardwareError` / `NoPacket` until the
      full SPI driver is implemented.
      `static_assert(NetworkInterface<W5500Interface<_FakeSpi>>)`.
- [x] 5.2 `drivers/net/esp_at/esp_at_interface.hpp`: stub `EspAtInterface<UartHandle>`
      implementing `NetworkInterface` via UART-AT.
      `static_assert(NetworkInterface<EspAtInterface<_FakeUart>>)`.
- [x] 5.3 `tests/compile_tests/test_net_w5500_concept.cpp` and
      `test_net_esp_at_concept.cpp`: concept-only compile checks for both stubs.

## 6. Board wiring — SAME70 Xplained

- [x] 6.1 `boards/same70_xplained/board_ethernet.hpp`: type aliases `BoardMdio`,
      `BoardGmac`, `BoardPhy`, `BoardEth`; factory helpers `make_mdio()`,
      `make_gmac(mac_span)`, `make_phy(mdio)`, `make_ethernet_interface(gmac, phy)`;
      bring-up helpers `enable_ethernet_clocks()`, `mux_ethernet_pins()`,
      `release_phy_reset()`, `mac_from_eui48()`.
- [x] 6.2 `examples/driver_ksz8081_probe/main.cpp`: replaced hand-rolled
      `Same70GmacMdio` struct and `gmac_enable_management()` function with
      `auto mdio = board::make_mdio(); mdio.enable_management()`. Logic unchanged.

## 7. Examples

- [x] 7.1 `examples/ethernet_basic/`: SAME70 link-up + DHCP + ping, cooperative loop.
      Only builds for `same70_xplained` with `ALLOY_BUILD_NETWORK_LWIP=ON`.
      README includes wiring table and architecture diagram.
- [x] 7.2 `examples/modbus_tcp_slave/`: Modbus TCP slave on port 502 using 10-var
      registry (same as `modbus_slave_basic`). `TcpStream` as `ByteStream`.
      README shows Python `pymodbus` test. Closes the "pending network HAL" note
      in `tcp_frame.hpp` and `docs/MODBUS.md`.

## 8. Documentation

- [x] 8.1 `docs/NETWORK.md`: user guide — quick-start, MDIO concept + MDC table,
      EthernetMac, PhyDriver, NetworkInterface, LwipAdapter API (DHCP/static IP,
      poll, connect/listen), TcpStream/ByteStream, Modbus TCP transport comparison
      table, WiFi module path, common gotchas.
- [x] 8.2 `docs/SUPPORT_MATRIX.md`: `ethernet` added at `representative` (concept
      checks + lwIP host loopback + hardware spot-check pending);
      `wifi` added at `scaffolded` (concept checks only).
- [x] 8.3 `docs/MODBUS.md`: removed "pending network HAL" note from TCP framing
      section; replaced with cross-link to `docs/NETWORK.md` and
      `examples/modbus_tcp_slave/`; added `modbus_tcp_slave` to Examples table.
- [x] 8.4 `openspec/changes/add-modbus-protocol/tasks.md` task 10.3 note resolved:
      the "pending network HAL" condition is satisfied by this change.
