## Why

The `seed-driver-library` change shipped three device-class seeds (display, sensor,
flash) that each exercise one HAL surface. The SAME70 Xplained validation board — our
primary foundational board — also hosts three onboard ICs that today have no driver
coverage: an AT24MAC402 I2C EEPROM with pre-programmed EUI-48/EUI-64, an ISSI
IS42S16100F-5B SDRAM on the SDRAM controller, and a Micrel KSZ8081RNACA Ethernet PHY on
MDIO. Covering them lets adopters who pick up the SAME70 board out of the box run
end-to-end bring-up against real silicon instead of inventing register-level code.

Each of the three also stretches the driver convention in a useful way:

- **AT24MAC402** — I2C paged EEPROM with a second I2C sub-address block that exposes the
  factory-programmed EUI-48/EUI-64. Proves the driver pattern supports paged writes,
  16-byte page alignment, and a second logical sub-device at a different address.
- **KSZ8081RNACA** — Standards-based Ethernet PHY controlled via MDIO. Alloy does not yet
  ship an MDIO HAL, so the driver is templated over a user-supplied `MdioBus` handle
  exposing `read(phy, reg)` / `write(phy, reg, value)`. Proves the driver convention
  tolerates a HAL gap by parameterising the transport.
- **IS42S16100F-5B** — SDRAM. No bus; the chip is controlled by the SAME70 SDRAMC
  peripheral. The "driver" is a constexpr chip descriptor: timings (ns → cycles),
  geometry (rows/cols/banks/bus width), CAS latency. Proves the drivers/ tree can host
  "chip descriptor" headers that the board's memory-controller init consumes.

## What Changes

- Add `drivers/memory/at24mac402/at24mac402.hpp`, a templated I2C driver exposing:
  - `init()` — probe EEPROM + EUI sub-addresses
  - `read(offset, span)` / `write(offset, span)` — 16-byte page-aware write, arbitrary read
  - `read_eui48(array<uint8_t,6>&)` / `read_eui64(array<uint8_t,8>&)`
  - `read_serial_number(array<uint8_t,16>&)` (128-bit unique serial in the protected block)
- Add `drivers/net/ksz8081/ksz8081.hpp`, a templated Ethernet PHY driver exposing:
  - `init()` — verify PHY ID, soft-reset, enable auto-negotiation
  - `soft_reset()`
  - `restart_auto_negotiation()`
  - `read_link_status() -> LinkStatus { up, speed_mbps, full_duplex }`
- Add `drivers/memory/is42s16100f/is42s16100f.hpp`, a constexpr chip descriptor exposing:
  - `kRowCount`, `kColumnCount`, `kBankCount`, `kDataBusBits`, `kCasLatencyCycles`
  - `Timings` struct with t_rc, t_rcd, t_rp, t_ras, t_wr, t_rfc, refresh_period in ns
  - `timings_for_bus(uint32_t hclk_hz) -> TimingsCycles` — ns → cycles (ceil)
- Add compile tests `tests/compile_tests/test_driver_seed_{at24mac402,ksz8081,is42s16100f}.cpp`
  that instantiate each driver / descriptor and assert key constants via `static_assert`.
- Register the three new compile tests in `tests/compile/contract_smoke.cmake`.
- Update `drivers/README.md` with the three new drivers and a `drivers/net/` entry.

Out of scope for this change:

- Full IS42S16100F controller-init flow (the SAME70 SDRAMC driver itself is separate work).
- KSZ8081 interrupt / wake-on-LAN / MDI-X override — seed covers auto-negotiation only.
- AT24MAC402 write-protect pin handling — caller owns the WP GPIO.

## Outcome

The SAME70 Xplained foundational board has driver coverage for every onboard IC it ships
with. The driver library convention is extended with two new shapes (template over a non-HAL
handle; constexpr chip descriptor) without breaking the existing seed-driver pattern.

## Impact

- Affected specs:
  - modifies: `driver-seed-library` (extends with three more required drivers + the two
    new shapes the library supports)
- Affected code and docs:
  - `drivers/memory/at24mac402/at24mac402.hpp` (new)
  - `drivers/net/ksz8081/ksz8081.hpp` (new)
  - `drivers/memory/is42s16100f/is42s16100f.hpp` (new)
  - `drivers/README.md` (adds three rows + drivers/net entry)
  - `tests/compile_tests/test_driver_seed_at24mac402.cpp` (new)
  - `tests/compile_tests/test_driver_seed_ksz8081.cpp` (new)
  - `tests/compile_tests/test_driver_seed_is42s16100f.cpp` (new)
  - `tests/compile/contract_smoke.cmake` (registers the three new compile tests)
