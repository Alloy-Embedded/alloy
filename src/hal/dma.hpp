#pragma once

#include "hal/dma/dma.hpp"

// alloy.device.v2.1 lite driver — no descriptor-runtime required.
// controller<DmaBase>   : STM32 DMA v1 channel engine.
// dmamux<DmamuxBase>    : DMAMUX1 request router (G4 / L4 / WB / G0+).
#include "hal/dma/lite.hpp"
