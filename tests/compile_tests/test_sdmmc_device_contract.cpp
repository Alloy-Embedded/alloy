// Compile test: SDMMC device contract traits are accessible through the
// alloy::device namespace on boards that publish sdmmc.hpp semantics.
// Targeted at same70_xplained (PeripheralId::HSMCI populated with valid fields).
//
// Ref: openspec/changes/extend-device-contract-qspi-sdmmc-eth

#include "device/runtime.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

using Traits = alloy::device::SdmmcSemanticTraits<alloy::device::PeripheralId::HSMCI>;
static_assert(Traits::kPresent, "SAME70 HSMCI must be present");
static_assert(Traits::kBusWidthField.valid, "bus width field must be valid");
static_assert(Traits::kClockDividerField.valid, "clock divider field must be valid");
static_assert(Traits::kCommandIndexField.valid, "command index field must be valid");

// Peripheral list must be non-empty.
static_assert(alloy::device::runtime::sdmmc_semantic_peripheral_ids.size() >= 1u);

#endif
