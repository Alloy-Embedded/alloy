# Spec: Custom SVD Repository

## ADDED Requirements

### Requirement: Custom SVD Directory Structure
**ID**: SVD-CUSTOM-001
**Priority**: P0 (Critical)

The system SHALL provide a dedicated directory for user-contributed SVD files separate from upstream sources.

#### Scenario: User adds custom SVD file
```bash
# Given the codegen directory structure
tools/codegen/
├── upstream/cmsis-svd-data/  # Existing
└── custom-svd/               # NEW

# When user downloads a custom SVD
cd tools/codegen/custom-svd/vendors/STMicro
wget https://example.com/STM32F103C8.svd

# Then the file is placed in the correct location
# And it will be discovered by the codegen pipeline
```

#### Scenario: Vendor subdirectories required
```bash
# Given custom-svd directory
tools/codegen/custom-svd/

# When adding SVD files
# Then they MUST be organized by vendor:
custom-svd/vendors/STMicro/STM32F103C8.svd
custom-svd/vendors/NXP/LPC1768.svd
custom-svd/vendors/Community/CustomBoard.svd

# And flat structure is not allowed:
# custom-svd/STM32F103C8.svd  ❌ Invalid
```

---

### Requirement: SVD Merge Policy Configuration
**ID**: SVD-CUSTOM-002
**Priority**: P0 (Critical)

The system SHALL use a JSON configuration file to define priority rules and conflict resolution strategies when merging custom and upstream SVDs.

#### Scenario: Custom SVD overrides upstream
```python
# Given both files exist:
# - tools/codegen/upstream/cmsis-svd-data/STMicro/STM32F103C8.svd
# - tools/codegen/custom-svd/vendors/STMicro/STM32F103C8.svd

# When merge_policy.json specifies:
{
  "priority": {
    "order": ["custom-svd", "upstream"]
  },
  "conflict_resolution": {
    "duplicate_files": "prefer_custom"
  }
}

# Then the custom SVD is used
# And a warning is logged:
# ⚠️  Using custom-svd/vendors/STMicro/STM32F103C8.svd (overrides upstream)
```

#### Scenario: Version-based conflict resolution
```python
# Given two SVD files with version info:
# - upstream/STM32F103C8.svd (version 1.0)
# - custom-svd/STM32F103C8.svd (version 1.2)

# When merge policy specifies:
{
  "conflict_resolution": {
    "version_mismatch": "use_newer_version"
  }
}

# Then version 1.2 (custom) is used
# And log shows:
# ℹ️  Using STM32F103C8.svd v1.2 (newer than upstream v1.0)
```

---

### Requirement: SVD Discovery Algorithm
**ID**: SVD-CUSTOM-003
**Priority**: P0 (Critical)

The system SHALL scan both upstream and custom-svd directories and merge them according to the defined priority rules.

#### Scenario: Discover SVDs from both sources
```python
# Given directory structure:
upstream/cmsis-svd-data/STMicro/STM32F103CB.svd
custom-svd/vendors/STMicro/STM32F103C8.svd
custom-svd/vendors/NXP/LPC1768.svd

# When running discovery:
svd_files = discover_svd_files()

# Then all three SVDs are found:
assert "STM32F103CB" in svd_files  # From upstream
assert "STM32F103C8" in svd_files  # From custom
assert "LPC1768" in svd_files      # From custom

# And source is tracked:
assert svd_files["STM32F103CB"]["source"] == "upstream"
assert svd_files["STM32F103C8"]["source"] == "custom-svd"
```

#### Scenario: Detect and resolve conflicts
```python
# Given duplicate device in both sources
upstream/STM32F103C8.svd
custom-svd/vendors/STMicro/STM32F103C8.svd

# When running discovery
svd_files = discover_svd_files()

# Then conflict is detected and resolved per policy
# And warning is shown:
# ⚠️  WARNING: Duplicate SVD detected for STM32F103C8
#     Using: custom-svd (higher priority)

# And only custom version is in result
assert svd_files["STM32F103C8"]["source"] == "custom-svd"
assert svd_files["STM32F103C8"]["priority"] == 2
```

---

### Requirement: SVD Validation
**ID**: SVD-CUSTOM-004
**Priority**: P1 (High)

The system SHALL validate custom SVD files for XML syntax and required fields before processing.

#### Scenario: Detect invalid XML
```python
# Given invalid SVD file with XML syntax error
custom-svd/vendors/STMicro/BadDevice.svd:
  <device>
    <name>BadDevice
    <!-- Missing closing tag! -->

# When running validation
python tools/codegen/validate_svds.py

# Then error is reported:
# ❌ ERROR: Invalid SVD file
#    File: custom-svd/vendors/STMicro/BadDevice.svd
#    Issue: XML parsing error at line 3
#    Action: Fix XML syntax or remove file

# And file is skipped during codegen
# And codegen continues with valid files
```

#### Scenario: Detect missing required fields
```python
# Given SVD file missing device name
custom-svd/vendors/STMicro/Incomplete.svd:
  <device>
    <!-- Missing <name> tag -->
    <addressUnitBits>8</addressUnitBits>
  </device>

# When running validation
result = validate_svd_file("Incomplete.svd")

# Then validation fails:
assert result.is_error()
assert "Missing required field: device.name" in result.error_message()

# And file is skipped with warning:
# ⚠️  WARNING: Skipping Incomplete.svd - missing required field 'device.name'
```

---

### Requirement: Contribution Guidelines
**ID**: SVD-CUSTOM-005
**Priority**: P1 (High)

The system SHALL provide documentation for users to contribute SVD files correctly.

#### Scenario: README provides contribution guide
```bash
# Given custom-svd directory
# When user reads README.md
cat tools/codegen/custom-svd/README.md

# Then it contains:
# 1. File naming conventions (use official device name)
# 2. Vendor directory structure
# 3. Required README per vendor folder
# 4. License information requirements
# 5. SVD source attribution

# And examples are provided
```

#### Scenario: Vendor README template
```bash
# Given vendor directory without README
custom-svd/vendors/STMicro/  # No README.md

# When validation runs with policy:
{
  "validation": {
    "require_vendor_readme": true
  }
}

# Then warning is shown:
# ⚠️  WARNING: Missing README in custom-svd/vendors/STMicro/
#     Please add README.md with source information

# But codegen continues (soft requirement)
```

---

### Requirement: CLI Commands for SVD Management
**ID**: SVD-CUSTOM-006
**Priority**: P2 (Medium)

The system SHALL provide make targets for listing and validating SVD files.

#### Scenario: List all available SVDs
```bash
# Given SVDs from both sources
# When running list command
make list-svds

# Then output shows all SVDs grouped by source:
# 📦 Available SVD Files:
#
# Upstream (cmsis-svd-data):
#   ✓ STM32F103C6.svd
#   ✓ STM32F103CB.svd
#   ... (848 more)
#
# Custom (custom-svd/):
#   ✓ STMicro/STM32F103C8.svd  [Overrides upstream]
#   ✓ STMicro/STM32F103VE.svd  [New device]
#
# Total: 850 SVDs (848 upstream, 3 custom, 1 override)
```

#### Scenario: Validate custom SVDs only
```bash
# Given custom SVD files (some valid, some invalid)
# When running validation
make validate-svds

# Then only custom SVDs are checked
# And results show per-file status:
# ✅ STMicro/STM32F103C8.svd
# ✅ NXP/LPC1768.svd
# ❌ STMicro/BadDevice.svd (XML error)

# And exit code is 1 if any errors found
```

---

### Requirement: Backward Compatibility
**ID**: SVD-CUSTOM-007
**Priority**: P0 (Critical)

The system SHALL maintain backward compatibility with existing codegen pipeline.

#### Scenario: Existing codegen works without custom-svd
```bash
# Given project without custom-svd directory
# When running codegen
make codegen

# Then upstream SVDs are processed normally
# And no errors occur
# And generated code is identical to before
```

#### Scenario: Custom-svd is optional
```python
# Given upstream SVD discovery
# When custom-svd directory doesn't exist
if not Path("tools/codegen/custom-svd").exists():
    # Then only upstream SVDs are used
    svds = scan_upstream_only()

# And no errors are raised
assert len(svds) > 0
```
