#pragma once

// Minimal synthetic board header used by the ALLOY_BOARD=custom smoke test.
// It does not implement a real board contract; it only proves that the runtime
// accepts a board header declared outside the runtime tree.

namespace board {

inline void init() {}

}  // namespace board
