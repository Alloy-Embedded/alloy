#pragma once

// Standalone multi-core declaration for the Raspberry Pi Pico.
//
// This header is intentionally minimal: it only declares board::launch_core1
// so bare-metal multi-core demos (examples/rp2040_dual_core) can avoid pulling
// in the full board.hpp include chain (which transitively requires the
// alloy-devices generated `selected_config.hpp`). The implementation lives
// in board_multicore.cpp.

namespace board {

// Launches the second Cortex-M0+ core (Core 1) using the SIO FIFO 5-word
// handshake (vector_table=0, stack=top-of-`.core1_stack`, entry=fn). Blocks
// until the ROM acknowledges the handshake.
//
// `fn` MUST be a free function (or a non-capturing lambda decayed to one)
// because we have no closure machinery on Core 1.
void launch_core1(void (*fn)());

}  // namespace board
