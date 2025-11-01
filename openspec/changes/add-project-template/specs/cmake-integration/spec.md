# Spec: CMake Integration

## ADDED Requirements

### Requirement: Root CMake Configuration
**ID**: TEMPLATE-CMAKE-001
**Priority**: P0 (Critical)

The system SHALL provide root CMakeLists.txt that includes Alloy and user targets.

#### Scenario: Configure project with board selection
```bash
# Given template project
# When configuring CMake
cmake -B build -DBOARD=stm32f407vg -DTARGET=application

# Then Alloy is included
# And target is configured
# And board settings applied
cmake --build build
```

---

### Requirement: Custom Board Discovery
**ID**: TEMPLATE-CMAKE-002
**Priority**: P0 (Critical)

The system SHALL search custom boards before Alloy boards.

#### Scenario: Custom board overrides Alloy board
```bash
# Given custom board with same name
mkdir -p boards/stm32f407vg
cat > boards/stm32f407vg/board.hpp <<EOF
// Custom version
EOF

# When building
cmake -B build -DBOARD=stm32f407vg

# Then custom board is used
grep "Using custom board" build/CMakeCache.txt
```

---

### Requirement: Per-Target CMakeLists
**ID**: TEMPLATE-CMAKE-003
**Priority**: P1 (High)

The system SHALL allow each target to have its own CMakeLists.txt.

#### Scenario: Independent target configuration
```cmake
# bootloader/CMakeLists.txt
add_executable(bootloader main.cpp)
target_link_libraries(bootloader common alloy::hal)
alloy_target_board_settings(bootloader ${BOARD})

# application/CMakeLists.txt
add_executable(application main.cpp tasks/*.cpp)
target_link_libraries(application common alloy::rtos)
alloy_target_board_settings(application ${BOARD})
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
