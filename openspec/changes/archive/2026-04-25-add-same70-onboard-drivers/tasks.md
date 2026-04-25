## 1. OpenSpec Baseline

- [x] 1.1 Add delta requirements extending `driver-seed-library` with the three SAME70
      onboard drivers and the two new driver shapes (templated over non-HAL handle,
      constexpr chip descriptor)

## 2. AT24MAC402 I2C EEPROM + EUI

- [x] 2.1 Add `drivers/memory/at24mac402/at24mac402.hpp` with paged write, arbitrary read,
      `read_eui48`, `read_eui64`, `read_serial_number`
- [x] 2.2 Add compile test `tests/compile_tests/test_driver_seed_at24mac402.cpp`

## 3. KSZ8081RNACA Ethernet PHY

- [x] 3.1 Add `drivers/net/ksz8081/ksz8081.hpp` templated over an `MdioBus` handle with
      `init`, `soft_reset`, `restart_auto_negotiation`, `read_link_status`
- [x] 3.2 Add compile test `tests/compile_tests/test_driver_seed_ksz8081.cpp`

## 4. IS42S16100F-5B SDRAM Descriptor

- [x] 4.1 Add `drivers/memory/is42s16100f/is42s16100f.hpp` with constexpr geometry +
      `Timings` ns values + `timings_for_bus(hclk_hz) -> TimingsCycles`
- [x] 4.2 Add compile test `tests/compile_tests/test_driver_seed_is42s16100f.cpp`
      (static_assert geometry + 150 MHz HCLK timings)

## 5. Docs

- [x] 5.1 Update `drivers/README.md` — add the three rows + `drivers/net/` subtree mention

## 6. Validation

- [x] 6.1 Register the three compile tests in `tests/compile/contract_smoke.cmake`
- [x] 6.2 `openspec validate add-same70-onboard-drivers --strict`
