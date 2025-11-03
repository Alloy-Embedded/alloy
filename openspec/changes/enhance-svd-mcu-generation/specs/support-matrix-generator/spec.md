# Spec: Support Matrix Generator

## ADDED Requirements

### Requirement: Automated MCU Discovery
**ID**: MATRIX-001
**Priority**: P1 (High)

The system SHALL automatically scan generated code to discover all supported MCUs and their capabilities.

#### Scenario: Scan finds all generated MCUs
```python
# Given generated MCU directories:
src/hal/st/stm32f1/generated/stm32f103c8/
src/hal/st/stm32f1/generated/stm32f103cb/
src/hal/esp32/generated/esp32c3/

# When running scanner:
mcus = scan_generated_mcus()

# Then all MCUs are discovered:
assert len(mcus) == 3
assert "stm32f103c8" in [m.name for m in mcus]
assert "stm32f103cb" in [m.name for m in mcus]
assert "esp32c3" in [m.name for m in mcus]

# And source paths are recorded:
assert mcus[0].path == "src/hal/st/stm32f1/generated/stm32f103c8"
```

#### Scenario: Extract MCU information from headers
```python
# Given generated pins.hpp and traits.hpp files
# When extracting information:
mcu_info = extract_mcu_info(
    pins_file="src/hal/st/stm32f1/generated/stm32f103c8/pins.hpp",
    traits_file="src/hal/st/stm32f1/generated/stm32f103c8/traits.hpp"
)

# Then all characteristics are extracted:
assert mcu_info.name == "STM32F103C8"
assert mcu_info.vendor == "STMicroelectronics"
assert mcu_info.flash_kb == 64
assert mcu_info.sram_kb == 20
assert mcu_info.gpio_pins == 37
assert mcu_info.uart_count == 3
assert mcu_info.has_usb == True
```

---

### Requirement: Markdown Table Generation
**ID**: MATRIX-002
**Priority**: P1 (High)

The system SHALL generate markdown tables showing all supported MCUs grouped by vendor and family.

#### Scenario: Generate table with all MCU details
```python
# Given list of MCU information objects
mcus = [
    MCUInfo(name="STM32F103C8", vendor="STMicroelectronics", family="STM32F1",
            package="LQFP48", flash_kb=64, sram_kb=20, gpio_pins=37,
            uart_count=3, i2c_count=2, spi_count=2, adc_channels=10,
            timer_count=7, has_usb=True, status="full")
]

# When generating markdown:
table = generate_markdown_table(mcus)

# Then table contains correct headers:
assert "| MCU | Package | Flash | RAM | GPIO |" in table

# And MCU row is formatted correctly:
assert "| STM32F103C8 | LQFP48 | 64KB | 20KB | 37 |" in table

# And status badge is shown:
assert "✅ Full" in table
```

#### Scenario: Group MCUs by vendor and family
```python
# Given MCUs from multiple vendors
mcus = [
    MCUInfo(name="STM32F103C8", vendor="STMicroelectronics", family="STM32F1"),
    MCUInfo(name="STM32F407VG", vendor="STMicroelectronics", family="STM32F4"),
    MCUInfo(name="ESP32-C3", vendor="Espressif", family="ESP32"),
]

# When generating markdown:
table = generate_markdown_table(mcus)

# Then vendors are separated:
assert "### STMicroelectronics" in table
assert "### Espressif" in table

# And families are grouped:
assert "#### STM32F1 Series" in table
assert "#### STM32F4 Series" in table
assert "#### ESP32 Series" in table

# And MCUs are sorted within families:
# (alphabetically: STM32F103C8 before STM32F407VG)
```

---

### Requirement: Support Status Tracking
**ID**: MATRIX-003
**Priority**: P1 (High)

The system SHALL determine and display support status for each MCU based on testing and examples.

#### Scenario: Full support status
```python
# Given MCU with hardware test and example
# hardware_tests/test_stm32f103c8.cpp exists
# examples/bluepill/ exists

# When determining status:
status = determine_status(mcu_dir="stm32f103c8")

# Then status is "full":
assert status == "full"

# And badge shows:
# ✅ Full
```

#### Scenario: Partial support status
```python
# Given MCU with example but no hardware test
# examples/custom_board/ exists
# No hardware test file

# When determining status:
status = determine_status(mcu_dir="stm32f103rc")

# Then status is "partial":
assert status == "partial"

# And badge shows:
# ⚠️ Partial
```

#### Scenario: WIP status
```python
# Given MCU with generated code only
# No examples or hardware tests

# When determining status:
status = determine_status(mcu_dir="stm32f103ve")

# Then status is "wip":
assert status == "wip"

# And badge shows:
# 🚧 WIP
```

---

### Requirement: README Integration
**ID**: MATRIX-004
**Priority**: P1 (High)

The system SHALL update README.md with the generated support matrix between marker comments.

#### Scenario: Insert matrix into README
```markdown
# Given README.md with markers:
<!-- SUPPORT_MATRIX_START -->
<!-- SUPPORT_MATRIX_END -->

# When updating README:
update_readme(markdown_content)

# Then content is inserted between markers:
<!-- SUPPORT_MATRIX_START -->
## 🎯 Supported MCUs

### STMicroelectronics
| MCU | Package | Flash | ...
<!-- SUPPORT_MATRIX_END -->

# And existing content outside markers is preserved
```

#### Scenario: Append matrix if no markers exist
```markdown
# Given README.md without markers
# When updating README:
update_readme(markdown_content)

# Then markers and content are appended:
(existing README content)

<!-- SUPPORT_MATRIX_START -->
## 🎯 Supported MCUs
...
<!-- SUPPORT_MATRIX_END -->
```

---

### Requirement: CLI Commands
**ID**: MATRIX-005
**Priority**: P1 (High)

The system SHALL provide make targets to display and update the support matrix.

#### Scenario: List MCUs in terminal
```bash
# Given generated MCUs
# When running list command:
make list-mcus

# Then output shows formatted table:
# 🔍 Scanning for supported MCUs...
#    Found 5 MCUs
#
# 📋 Supported MCUs by Vendor:
#
# STMicroelectronics - STM32F1
#   ✅ STM32F103C6  (LQFP48)  32KB Flash, 10KB RAM   [Full Support]
#   ✅ STM32F103C8  (LQFP48)  64KB Flash, 20KB RAM   [Full Support]
#
# Total: 5 MCUs (3 full, 1 partial, 1 WIP)
```

#### Scenario: Update documentation
```bash
# Given generated MCUs
# When running update command:
make update-docs

# Then README.md is updated:
# 📚 Updating documentation...
# ✅ Documentation updated

# And git diff shows changes:
git diff README.md
# Shows updated support matrix
```

---

### Requirement: Support Statistics
**ID**: MATRIX-006
**Priority**: P2 (Medium)

The system SHALL provide summary statistics about supported MCUs.

#### Scenario: Calculate support statistics
```python
# Given list of MCUs:
mcus = [
    MCUInfo(status="full"),    # 3 full
    MCUInfo(status="full"),
    MCUInfo(status="full"),
    MCUInfo(status="partial"), # 1 partial
    MCUInfo(status="wip"),     # 1 wip
]

# When generating statistics:
stats = calculate_statistics(mcus)

# Then counts are correct:
assert stats.total == 5
assert stats.full == 3
assert stats.partial == 1
assert stats.wip == 1

# And summary text is generated:
assert "Total: 5 MCUs (3 full, 1 partial, 1 WIP)" in stats.summary
```

#### Scenario: Statistics shown in README
```markdown
# When generating markdown table
# Then statistics are included at bottom:

**Total Supported MCUs**: 5 (3 fully tested, 1 partial, 1 WIP)

**Want to add support for a new MCU?** See [docs/MCU_SUPPORT.md](docs/MCU_SUPPORT.md)
```

---

### Requirement: Datasheet Links
**ID**: MATRIX-007
**Priority**: P2 (Medium)

The system SHALL include hyperlinks to official MCU datasheets in the support matrix.

#### Scenario: MCU name links to datasheet
```markdown
# Given MCU information with datasheet URL
mcu = MCUInfo(
    name="STM32F103C8",
    datasheet_url="https://www.st.com/resource/en/datasheet/stm32f103c8.pdf"
)

# When generating markdown row:
row = generate_table_row(mcu)

# Then MCU name is hyperlinked:
assert "[STM32F103C8](https://www.st.com/.../stm32f103c8.pdf)" in row

# And clicking name opens datasheet
```

#### Scenario: Fallback for missing datasheet URL
```python
# Given MCU without datasheet URL
mcu = MCUInfo(name="CustomMCU", datasheet_url="")

# When generating markdown row:
row = generate_table_row(mcu)

# Then plain text is shown:
assert "| CustomMCU |" in row
assert "[CustomMCU]" not in row  # No link
```

---

### Requirement: Timestamp Tracking
**ID**: MATRIX-008
**Priority**: P3 (Low)

The system SHALL include generation timestamp in the support matrix.

#### Scenario: Timestamp in README
```markdown
# When generating support matrix
# Then timestamp is included:

*Last updated: 2025-11-03 10:30:00*

# And shows when matrix was last generated
```

#### Scenario: CI detects outdated matrix
```bash
# Given README with old timestamp
# When new MCU is added and CI runs:

# Then check fails:
# ❌ Support matrix is outdated
#    Last updated: 2025-11-01
#    New MCUs found: stm32f103ve
#    Run: make update-docs

# And developer must update README
```
