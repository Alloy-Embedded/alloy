/**
 * @file dma.hpp
 * @brief SAME70 DMA Platform Integration
 */

#pragma once

#include "hal/vendors/atmel/same70/dma_hardware_policy.hpp"

namespace ucore::hal::same70 {

// Type alias for DMA
using Dma = DmaHardware;

}  // namespace ucore::hal::same70
