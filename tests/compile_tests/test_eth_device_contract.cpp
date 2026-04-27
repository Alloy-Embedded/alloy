// Compile test: ETH device contract traits are accessible through the
// alloy::device namespace on boards that publish eth.hpp semantics.
// Targeted at same70_xplained (PeripheralId::GMAC populated with valid fields).
//
// Ref: openspec/changes/extend-device-contract-qspi-sdmmc-eth

#include "device/runtime.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

using Traits = alloy::device::EthSemanticTraits<alloy::device::PeripheralId::GMAC>;
static_assert(Traits::kPresent, "SAME70 GMAC must be present");
static_assert(Traits::kSpeedField.valid, "speed field must be valid");
static_assert(Traits::kFullDuplexField.valid, "full-duplex field must be valid");
static_assert(Traits::kRmiiEnableField.valid, "RMII enable field must be valid");

// Peripheral list must be non-empty.
static_assert(alloy::device::runtime::eth_semantic_peripheral_ids.size() >= 1u);

#endif
