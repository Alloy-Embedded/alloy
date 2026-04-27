# Tasks: Extend Device Contract — QSPI, SDMMC, ETH

All tasks are host-testable (compile + static_assert). No hardware
required. Expected review size: ~80 lines of new code.

## 1. Verify generated artifacts exist in alloy-devices

Before editing anything in alloy, confirm the generated headers are
present in the current alloy-devices release artifact.

- [x] 1.1 Check that the installed alloy-devices release artifact
      contains:
      `generated/runtime/devices/driver_semantics/qspi.hpp`
      `generated/runtime/devices/driver_semantics/sdmmc.hpp`
      `generated/runtime/devices/driver_semantics/ethernet.hpp`
      for at least one foundational board (`nucleo_f401re` for QSPI,
      `same70_xplained` for SDMMC + ETH).
- [x] 1.2 Confirm the macro names emitted in `selected_config.hpp`:
      `ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE`,
      `ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE`,
      `ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE`.
      (If names differ from the assumed names, update this spec and
      `runtime.hpp` to match what alloy-devices actually emits.)
- [x] 1.3 Confirm the trait type names in those headers match:
      `QspiSemanticTraits<Id>`,
      `SdmmcSemanticTraits<Id>`,
      `EthSemanticTraits<Id>`,
      `kQspiSemanticPeripherals`,
      `kSdmmcSemanticPeripherals`,
      `kEthSemanticPeripherals`.

## 2. Wire traits into `src/device/runtime.hpp`

- [x] 2.1 Inside `namespace alloy::device::runtime` (the inner
      namespace), add the QSPI block immediately after the PWM block
      (alphabetical within the un-gated section makes no difference;
      placing it after the last existing class is cleanest):

      ```cpp
      #if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using QspiSemanticTraits =
          selected::runtime_driver_contract::QspiSemanticTraits<Id>;
      inline constexpr auto qspi_semantic_peripherals =
          std::span{selected::runtime_driver_contract::kQspiSemanticPeripherals};
      inline constexpr const auto& qspi_semantic_peripheral_ids =
          selected::runtime_driver_contract::kQspiSemanticPeripherals;
      #endif
      ```

- [x] 2.2 Add the SDMMC block (same pattern):

      ```cpp
      #if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using SdmmcSemanticTraits =
          selected::runtime_driver_contract::SdmmcSemanticTraits<Id>;
      inline constexpr auto sdmmc_semantic_peripherals =
          std::span{selected::runtime_driver_contract::kSdmmcSemanticPeripherals};
      inline constexpr const auto& sdmmc_semantic_peripheral_ids =
          selected::runtime_driver_contract::kSdmmcSemanticPeripherals;
      #endif
      ```

- [x] 2.3 Add the ETH block (same pattern):

      ```cpp
      #if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using EthSemanticTraits =
          selected::runtime_driver_contract::EthSemanticTraits<Id>;
      inline constexpr auto eth_semantic_peripherals =
          std::span{selected::runtime_driver_contract::kEthSemanticPeripherals};
      inline constexpr const auto& eth_semantic_peripheral_ids =
          selected::runtime_driver_contract::kEthSemanticPeripherals;
      #endif
      ```

- [x] 2.4 Mirror all three in the outer `alloy::device` namespace
      (the section below `#endif  // ALLOY_DEVICE_RUNTIME_AVAILABLE`
      where the twelve existing aliases are re-exported):

      ```cpp
      #if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using QspiSemanticTraits = runtime::QspiSemanticTraits<Id>;
      #endif

      #if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using SdmmcSemanticTraits = runtime::SdmmcSemanticTraits<Id>;
      #endif

      #if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE
      template <PeripheralId Id>
      using EthSemanticTraits = runtime::EthSemanticTraits<Id>;
      #endif
      ```

## 3. Compile tests

- [x] 3.1 Create `tests/compile_tests/test_qspi_device_contract.cpp`:
      Build target: `nucleo_f401re`.
      Content:
      ```cpp
      #include "device/runtime.hpp"
      static_assert(ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE,
                    "F401RE QSPI semantics must be available");
      using Traits = alloy::device::QspiSemanticTraits<
          alloy::device::PeripheralId::QUADSPI>;  // enum value per F401RE contract
      static_assert(Traits::kInstructionField.valid,
                    "instruction field must be valid");
      static_assert(Traits::kAddressField.valid,
                    "address field must be valid");
      static_assert(Traits::kDataField.valid,
                    "data field must be valid");
      ```

- [x] 3.2 Create `tests/compile_tests/test_sdmmc_device_contract.cpp`:
      Build target: `same70_xplained`.
      Content:
      ```cpp
      #include "device/runtime.hpp"
      static_assert(ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE,
                    "SAME70 SDMMC semantics must be available");
      using Traits = alloy::device::SdmmcSemanticTraits<
          alloy::device::PeripheralId::HSMCI>;
      static_assert(Traits::kBusWidthField.valid,
                    "bus width field must be valid");
      static_assert(Traits::kClockDividerField.valid,
                    "clock divider field must be valid");
      static_assert(Traits::kCommandIndexField.valid,
                    "command index field must be valid");
      ```

- [x] 3.3 Create `tests/compile_tests/test_eth_device_contract.cpp`:
      Build target: `same70_xplained`.
      Content:
      ```cpp
      #include "device/runtime.hpp"
      static_assert(ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE,
                    "SAME70 ETH semantics must be available");
      using Traits = alloy::device::EthSemanticTraits<
          alloy::device::PeripheralId::GMAC>;
      static_assert(Traits::kSpeedField.valid,
                    "speed field must be valid");
      static_assert(Traits::kFullDuplexField.valid,
                    "full-duplex field must be valid");
      static_assert(Traits::kRmiiEnableField.valid,
                    "RMII enable field must be valid");
      ```

- [x] 3.4 Wire all three compile-test TUs into
      `tests/compile_tests/CMakeLists.txt` using the existing
      pattern for `test_adc_api.cpp` / `test_can_api.cpp`.

## 4. CI

- [x] 4.1 Confirm the three new compile-test targets build green in CI
      for their respective boards (`nucleo_f401re`, `same70_xplained`).
- [x] 4.2 Confirm no regressions on the other foundational boards
      (`nucleo_g071rb`, `nucleo_f401re`, `esp32_devkit`,
      `esp32c3_devkitm`). The new `#if` guards mean those boards
      compile cleanly even when the macros are absent / `0`.

## 5. Documentation

- [x] 5.1 Add three rows to `docs/SUPPORT_MATRIX.md` under a new
      "Peripheral Contract" section (or extend the existing table):
      | Peripheral | SAME70 | STM32G0 | STM32F4 | ESP32 | ESP32-C3 |
      |------------|--------|---------|---------|-------|----------|
      | QSPI       | —      | —       | contract| —     | —        |
      | SDMMC      |contract| —       | —       | —     | —        |
      | ETH        |contract| —       | —       | —     | —        |
      ("contract" = device contract published; HAL pending in
      extend-qspi/sdmmc/eth-coverage)
- [x] 5.2 Update `docs/PERIPHERAL_CLASSES.md` (or equivalent index)
      with the three new peripheral classes and their status.

## 6. Out-of-scope follow-ups (filed but not done in this change)

- [x] 6.1 File `add-octospi-device-binding` once an STM32H7 board
      enters the foundational matrix.
- [x] 6.2 File `add-sdio-device-binding` once a Wi-Fi/BT SDIO module
      driver is needed.
- [x] 6.3 File `add-eth-gbe-device-binding` once a GbE device enters
      the matrix.
- [x] 6.4 Track QSPI / SDMMC / ETH for ESP32 / RP2040 / AVR-DA under
      `expand-chip-coverage`.
