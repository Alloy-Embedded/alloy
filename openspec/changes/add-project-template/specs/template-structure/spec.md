# Spec: Template Structure

## ADDED Requirements

### Requirement: Git Submodule Integration
**ID**: TEMPLATE-STRUCT-001
**Priority**: P0 (Critical)

The system SHALL integrate Alloy as a Git submodule in `external/alloy/`.

#### Scenario: Clone template with submodules
```bash
# Given template repository
# When cloning with --recursive
git clone --recursive https://github.com/user/alloy-project-template.git

# Then Alloy submodule is initialized
test -f external/alloy/CMakeLists.txt
# And specific version is checked out
cd external/alloy && git describe --tags
```

---

### Requirement: Multi-Target Directory Structure
**ID**: TEMPLATE-STRUCT-002
**Priority**: P0 (Critical)

The system SHALL support multiple firmware targets in separate directories.

#### Scenario: Build bootloader and application separately
```bash
# Given multi-target structure
ls bootloader/ application/

# When building each target
./scripts/build.sh bootloader stm32f407vg
./scripts/build.sh application stm32f407vg

# Then separate binaries created
test -f build/bootloader/bootloader.elf
test -f build/application/application.elf
```

---

### Requirement: Shared Common Library
**ID**: TEMPLATE-STRUCT-003
**Priority**: P1 (High)

The system SHALL provide a `common/` library shared across all targets.

#### Scenario: Use shared utility in multiple targets
```cpp
// common/include/utils.h
uint32_t calculate_crc(const uint8_t* data, size_t len);

// bootloader/main.cpp
#include "utils.h"
uint32_t crc = calculate_crc(data, len);

// application/main.cpp
#include "utils.h"
uint32_t crc = calculate_crc(data, len);  // Same function
```

## MODIFIED Requirements
None.

## REMOVED Requirements
None.
