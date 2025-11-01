# Spec: Custom Boards

## ADDED Requirements

### Requirement: C++ Header Board Definition
**ID**: TEMPLATE-BOARD-001
**Priority**: P0 (Critical)

The system SHALL allow custom boards via C++ headers in `boards/` directory.

#### Scenario: Define custom board inheriting from Alloy board
```cpp
// boards/my_board/board.hpp
#include "external/alloy/boards/stm32f407vg/board.hpp"

namespace Board {
    inline constexpr const char* name = "My Custom Board";

    using CustomLed = alloy::hal::stm32f4::GpioPin<48>;

    inline void initialize() {
        ::Board::initialize();  // Call base
        CustomLed led;
        led.configure(alloy::hal::PinMode::Output);
    }
}
```

---

### Requirement: Custom Linker Script Support
**ID**: TEMPLATE-BOARD-002
**Priority**: P1 (High)

The system SHALL support custom linker scripts per board.

#### Scenario: Use custom memory layout
```bash
# Given custom linker script
cat > boards/my_board/linker.ld <<EOF
MEMORY {
    FLASH : ORIGIN = 0x08008000, LENGTH = 960K  /* After bootloader */
    RAM   : ORIGIN = 0x20000000, LENGTH = 128K
}
EOF

# When building
cmake -B build -DBOARD=my_board

# Then custom linker script used
grep "0x08008000" build/application/application.map
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
