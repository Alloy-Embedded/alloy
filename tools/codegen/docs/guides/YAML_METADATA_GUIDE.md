# YAML Metadata Guide

**Alloy CLI Enhanced Metadata Format**

Version: 1.0
Last Updated: 2025-01-18
Status: Active

---

## Overview

Alloy CLI uses YAML as the primary format for all metadata files (MCUs, boards, peripherals, templates). YAML provides significant advantages over JSON:

- **25-30% smaller** file sizes
- **Inline comments** for documenting hardware quirks
- **Clean multiline strings** (no escape sequences)
- **Better readability** for humans
- **Superior git diffs** and merge conflict resolution

## File Structure

### Directory Layout

```
tools/codegen/database/
├── mcus/                    # MCU family definitions
│   ├── stm32f4.yaml
│   ├── same70.yaml
│   └── ...
├── boards/                  # Board configurations
│   ├── nucleo_f401re.yaml
│   ├── stm32f4_discovery.yaml
│   └── ...
├── peripherals/             # Peripheral implementation status
│   ├── uart.yaml
│   ├── spi.yaml
│   ├── i2c.yaml
│   └── ...
├── templates/               # Project templates (owned by CLI)
│   ├── blinky.yaml
│   ├── uart_logger.yaml
│   ├── rtos.yaml
│   └── ...
└── schema/                  # YAML schemas (owned by CLI)
    ├── mcu.schema.yaml
    ├── board.schema.yaml
    ├── peripheral.schema.yaml
    └── template.schema.yaml
```

## MCU Metadata Format

### Basic Structure

```yaml
schema_version: "1.0"

family:
  id: stm32f4                # Lowercase identifier
  vendor: st                 # Vendor: st, atmel, nordic, etc.
  display_name: STM32F4 Series

  # Multiline description (no escaping needed!)
  description: |
    The STM32F4 series offers high performance with FPU.
    Perfect for motor control and audio processing.

  core: Cortex-M4F
  features:
    - FPU              # Inline comment explaining feature
    - DSP Instructions
    - MPU

mcus:
  - part_number: STM32F401RET6
    display_name: STM32F401RE
    core: Cortex-M4F
    max_freq_mhz: 84

    memory:
      flash_kb: 512
      sram_kb: 96
      eeprom_kb: 0     # STM32 uses FLASH emulation for EEPROM

    peripherals:
      uart:
        count: 3
        instances: [USART1, USART2, USART6]
        # IMPORTANT: USART1/6 on APB2 (faster), USART2 on APB1
        max_baud_rate:
          USART1: 5250000
          USART2: 2625000  # Limited by APB1 clock
```

### Documenting Hardware Quirks

Use inline comments to document:

**Performance characteristics**:
```yaml
spi:
  max_speed_mbps: 42  # Up to 42 MHz SPI clock (APB2 / 2)
  # NOTE: SPI1 and SPI4 on APB2 (faster), SPI2/SPI3 on APB1
```

**Limitations**:
```yaml
adc:
  count: 1
  channels: 16
  # NOTE: All channels share one ADC, multiplex for multi-channel
  # PERFORMANCE: Single ADC limits simultaneous sampling
```

**Conflicts**:
```yaml
gpio:
  ports: [GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOH]
  # QUIRK: GPIOH has fewer pins (PH0-PH1 only on F401)
```

## Board Metadata Format

### Basic Structure

```yaml
schema_version: "1.0"

board:
  id: nucleo_f401re
  display_name: Nucleo-F401RE
  vendor: st
  url: https://www.st.com/...

mcu:
  part_number: STM32F401RET6
  family: stm32f4

pinout:
  leds:
    - name: LD2
      color: green
      gpio: PA5
      active: high
      # WARNING: PA5 shared with SPI1_SCK - using SPI causes LED flicker

  buttons:
    - name: B1
      gpio: PC13
      active: low
      # NOTE: Requires 10kΩ pull-up resistor
```

### Documenting Pin Conflicts

**Use comments to warn about conflicts**:

```yaml
peripherals:
  spi:
    - instance: SPI1
      pins:
        # Default - conflicts with LED!
        - sck: PA5      # Arduino D13 / LED LD2
          miso: PA6
          mosi: PA7
          af: 5
          note: SCK shared with LED LD2 - will cause LED flicker
          # CONFLICT WARNING: LED blinks during SPI transfers

        # Recommended - no conflicts
        - sck: PB3      # Morpho connector
          miso: PA6
          mosi: PA7
          af: 5
          recommended: true
          note: Avoids LED conflict
```

## Peripheral Metadata Format

```yaml
schema_version: "1.0"

peripheral:
  id: uart
  display_name: UART/USART
  type: communication

implementations:
  - family: stm32f4
    status: implemented     # implemented | in_progress | planned
    api_levels:
      - simple     # Simple blocking API
      - fluent     # Fluent builder pattern
      - expert     # Direct register access

    limitations:
      - DMA not implemented yet
      - LIN mode not exposed
      # PLANNED: DMA support in v2.1

    quirks:
      - description: "Overrun error at high baud without buffering"
        impact: "Data loss if RX buffer overflows"
        workaround: "Use interrupt or DMA mode"
```

## Project Template Format

```yaml
schema_version: "1.0"

template:
  id: blinky
  display_name: "LED Blink"
  type: project      # Owned by CLI spec
  difficulty: beginner

files:
  - path: src/main.cpp
    template: |
      #include "alloy/hal/gpio.hpp"

      int main() {
          auto led = Gpio::create()
              .with_pin({{ board.led.gpio }})
              .as_output()
              .build();

          while (true) {
              led.toggle();
              delay_ms(500);
          }
      }
```

## Best Practices

### 1. Use Inline Comments Liberally

**Good**:
```yaml
uart:
  count: 3
  instances: [USART1, USART2, USART6]
  # IMPORTANT: USART1/6 support 5.25 Mbps, USART2 limited to 2.625 Mbps
```

**Bad** (JSON - no comments allowed):
```json
{
  "uart": {
    "count": 3,
    "instances": ["USART1", "USART2", "USART6"]
  }
}
```

### 2. Multiline Descriptions

**Good**:
```yaml
description: |
  The STM32F4 series offers high performance.
  Perfect for:
  - Motor control
  - Audio processing
  - Sensor fusion
```

**Bad** (JSON - requires escaping):
```json
{
  "description": "The STM32F4 series offers high performance.\\nPerfect for:\\n- Motor control\\n- Audio processing"
}
```

### 3. Document WHY, Not Just WHAT

**Good**:
```yaml
flash_kb: 512
sram_kb: 96
eeprom_kb: 0     # STM32 doesn't have EEPROM - use FLASH emulation instead
```

**Bad**:
```yaml
flash_kb: 512
sram_kb: 96
eeprom_kb: 0
```

### 4. Mark Conflicts and Quirks Clearly

Use prefixes like:
- `# WARNING:` - Critical conflicts
- `# QUIRK:` - Hardware peculiarities
- `# NOTE:` - Important information
- `# PERFORMANCE:` - Performance implications
- `# PLANNED:` - Future improvements

## Migration from JSON

### Using the Migration Script

Convert single file:
```bash
python scripts/migrate_json_to_yaml.py database/mcus/stm32f4.json
```

Convert all files:
```bash
python scripts/migrate_json_to_yaml.py --all
```

Dry run (preview only):
```bash
python scripts/migrate_json_to_yaml.py --all --dry-run
```

### Manual Conversion Tips

1. **Remove JSON syntax**:
   - Delete trailing commas
   - Remove quotes from keys (except special chars)
   - Use `true/false` instead of `true/false`

2. **Add comments**:
   - Document every quirk you know
   - Explain performance characteristics
   - Note conflicts and workarounds

3. **Format multiline strings**:
   - Use `|` for literal block (preserves newlines)
   - Use `>` for folded block (folds newlines to spaces)

4. **Validate**:
   ```bash
   python -c "import yaml; yaml.safe_load(open('file.yaml'))"
   ```

## Schema Validation

All YAML files must have `schema_version` field:

```yaml
schema_version: "1.0"   # Required at top of every file
```

Validate against schema:
```bash
# Using jsonschema (works with YAML too)
jsonschema -i database/mcus/stm32f4.yaml database/schema/mcu.schema.yaml
```

## Ownership and Responsibilities

| Component | Owner | Notes |
|-----------|-------|-------|
| **YAML Schemas** | `enhance-cli-professional-tool` (CLI Spec) | Defines structure |
| **MCU/Board Database** | `enhance-cli-professional-tool` (CLI Spec) | Data files |
| **Peripheral Metadata** | `enhance-cli-professional-tool` (CLI Spec) | Implementation status |
| **Peripheral Templates** | `library-quality-improvements` | Jinja2 templates |
| **Project Templates** | `enhance-cli-professional-tool` (CLI Spec) | Wizard templates |

See `openspec/changes/INTEGRATION_LIBRARY_CLI.md` for full coordination details.

## Examples

Full examples available in:
- `database/mcus/stm32f4.yaml` - MCU metadata
- `database/boards/nucleo_f401re.yaml` - Board pinout with conflicts
- `database/peripherals/uart.yaml` - Peripheral implementation
- `database/templates/blinky.yaml` - Project template

## See Also

- [Metadata Architecture](../architecture/METADATA.md)
- [OpenSpec: enhance-cli-professional-tool](../../../openspec/changes/enhance-cli-professional-tool/proposal.md)
- [Integration Guide](../../../openspec/changes/INTEGRATION_LIBRARY_CLI.md)
