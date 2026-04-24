## ADDED Requirements

### Requirement: Seed Library Shall Cover Every Onboard IC Of The SAME70 Foundational Board

The seed driver library SHALL ship a driver or chip descriptor for every peripheral IC
that the SAME70 Xplained foundational board hosts on-board beyond the SAME70 itself. That
set is the AT24MAC402 I2C EEPROM / EUI, the IS42S16100F-5B SDRAM, and the KSZ8081RNACA
Ethernet PHY.

#### Scenario: Adopter picks up the SAME70 Xplained foundational board

- **WHEN** a new adopter browses `drivers/` looking for the chips populated on the
  SAME70 Xplained evaluation board
- **THEN** there is a driver under `drivers/memory/at24mac402/` that reads and writes the
  EEPROM and reads the factory-programmed EUI-48/EUI-64/serial-number
- **AND** there is a chip descriptor under `drivers/memory/is42s16100f/` exposing the
  SDRAM geometry and timing parameters the SAME70 SDRAMC needs to bring the chip up
- **AND** there is a driver under `drivers/net/ksz8081/` that verifies the PHY ID,
  performs a soft reset, drives auto-negotiation, and reports link status

### Requirement: Seed Drivers Shall Support Transports Without An In-Tree HAL

The seed driver convention SHALL permit drivers to be templated over a transport handle
that the public alloy HAL does not ship today (for example, an MDIO bus for Ethernet PHYs),
provided the driver documents the minimal interface the handle must satisfy.

#### Scenario: Adopter uses a PHY driver without an alloy MDIO HAL

- **WHEN** an adopter instantiates the KSZ8081 driver with a user-supplied MDIO handle
  exposing `read(phy_address, register_address)` and
  `write(phy_address, register_address, value)` that both return
  `core::Result<T, core::ErrorCode>`
- **THEN** the driver compiles and operates without requiring an alloy-native MDIO HAL
- **AND** the driver header documents the handle interface contract so the adopter can
  plug in their own implementation

### Requirement: Seed Library Shall Permit Chip Descriptor Headers For Memory-Controller-Attached Parts

The seed driver convention SHALL allow "chip descriptor" entries that ship geometry and
timing constants for memory parts attached to a memory controller rather than to a
byte-oriented bus. Chip descriptor headers are header-only, allocate no memory, expose
values as `constexpr`, and document the datasheet revision they were written against.

#### Scenario: Adopter brings up the SAME70 SDRAMC against the IS42S16100F-5B

- **WHEN** an adopter configures the SAME70 SDRAMC using values from
  `drivers/memory/is42s16100f/is42s16100f.hpp`
- **THEN** the header exposes row/column/bank counts, data-bus width, CAS latency, and
  the datasheet ns values for t_RC, t_RCD, t_RP, t_RAS, t_WR, t_RFC, refresh period
- **AND** the header provides a `timings_for_bus(hclk_hz)` helper that converts the
  datasheet ns values into controller cycles using ceil-div so the returned cycles never
  violate the datasheet minimum
