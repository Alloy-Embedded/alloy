// Compile test: QSPI device contract traits are accessible through the
// alloy::device namespace on boards that publish qspi.hpp semantics.
// Targeted at same70_xplained (PeripheralId::QSPI populated with valid fields).
//
// Ref: openspec/changes/extend-device-contract-qspi-sdmmc-eth

#include "device/runtime.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

using Traits = alloy::device::QspiSemanticTraits<alloy::device::PeripheralId::QSPI>;
static_assert(Traits::kPresent, "SAME70 QSPI must be present");
static_assert(Traits::kInstructionField.valid, "instruction field must be valid");
static_assert(Traits::kAddressField.valid, "address field must be valid");
static_assert(Traits::kSerialClockBaudRateField.valid,
              "serial clock baud-rate field must be valid");
static_assert(Traits::kEnableField.valid, "enable field must be valid");
static_assert(Traits::kSoftwareResetField.valid, "software reset field must be valid");

// Peripheral list must be non-empty.
static_assert(alloy::device::runtime::qspi_semantic_peripheral_ids.size() >= 1u);

#endif
