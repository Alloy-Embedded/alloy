# Porting a New Board

## Goal

A board port should be mostly declarative.

The board layer should select:

- board name
- vendor/family/device from `alloy-devices`
- linker script
- board-level aliases such as LED pin and debug UART connector

## Rules

- Prefer consuming existing generated descriptors over handwritten board-specific register code.
- Keep bring-up explicit in board code.
- Avoid static initialization with side effects.
- Do not add new vendor-specific runtime APIs for a board port.

## Expected Steps

1. Add or update the board manifest entry.
2. Point the board to a published `alloy-devices` device.
3. Define board aliases in `board.hpp`.
4. Keep `board.cpp` limited to orchestration and board-specific choices.
5. Add compile-smoke coverage for the board path.

## References

- [ARCHITECTURE.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/ARCHITECTURE.md)
- [RUNTIME_DEVICE_BOUNDARY.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_DEVICE_BOUNDARY.md)
