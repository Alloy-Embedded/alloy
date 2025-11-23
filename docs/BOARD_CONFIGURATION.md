# Board Configuration System

MicroCore uses **declarative YAML configuration** to define board hardware, enabling:

✅ **Type-Safe**: JSON schema validation ensures correctness
✅ **Maintainable**: Single source of truth in human-readable format
✅ **Auto-Generated**: C++ code generated from YAML using templates
✅ **Portable**: Same schema works across all platforms
✅ **Documented**: Self-documenting with inline comments

## Quick Start

### 1. Create Board YAML

Create `boards/<board_name>/board.yaml`:

```yaml
board:
  name: "My Board"
  vendor: "Manufacturer"
  version: "1.0.0"

platform: stm32f4

mcu:
  part_number: "STM32F401RET6"
  architecture: cortex-m4
  frequency_mhz: 84

clock:
  source: PLL
  system_clock_hz: 84000000
  hse_hz: 8000000
  pll:
    m: 4
    n: 168
    p: 4
    q: 7

leds:
  - name: led_green
    port: GPIOA
    pin: 5
    active_high: true
```

### 2. Generate C++ Code

```bash
# Single board
python3 -m tools.codegen.cli.generators.board_generator \
    boards/nucleo_f401re/board.yaml

# All boards
python3 -m tools.codegen.cli.generators.board_generator \
    boards/ --all
```

### 3. Use in Code

```cpp
#include "boards/nucleo_f401re/board_config.hpp"

using namespace nucleo_f401re;

int main() {
    // Access board info
    const char* name = BoardInfo::name;  // "Nucleo-F401RE"

    // Use LED configuration
    using Led = LedConfig::led_green;  // GpioPin<GPIOA, 5>
    bool active_high = LedConfig::led_green_active_high;  // true

    // Clock configuration
    uint32_t freq = ClockConfig::system_clock_hz;  // 84000000
}
```

## YAML Schema

### Board Metadata

```yaml
board:
  name: "Nucleo-F401RE"           # Required: Board name
  vendor: "STMicroelectronics"    # Required: Manufacturer
  version: "1.0.0"                # Required: Hardware revision (semver)
  description: "Optional description"
  url: "https://..."              # Optional: Product page URL
```

### Platform

```yaml
platform: stm32f4  # Required: stm32f0|stm32f1|stm32f4|stm32f7|stm32g0|stm32g4|stm32h7|same70|host
```

### MCU

```yaml
mcu:
  part_number: "STM32F401RET6"   # Required: Full part number
  architecture: cortex-m4         # Required: cortex-m0|m0+|m3|m4|m7|m33
  flash_kb: 512                   # Optional: Flash size in KB
  ram_kb: 96                      # Optional: RAM size in KB
  frequency_mhz: 84               # Optional: Max frequency in MHz
```

### Clock Configuration

#### Basic Clock

```yaml
clock:
  source: HSI                     # HSI|HSE|PLL|RC
  system_clock_hz: 16000000       # Required: System clock in Hz
```

#### PLL Clock (STM32)

```yaml
clock:
  source: PLL
  system_clock_hz: 84000000
  hse_hz: 8000000                 # External crystal frequency
  pll:
    m: 4      # PLL input divider (HSE / M)
    n: 168    # PLL VCO multiplier (input × N)
    p: 4      # System clock divider (VCO / P)
    q: 7      # USB clock divider (VCO / Q)
  ahb_prescaler: 1                # AHB prescaler
  apb1_prescaler: 2               # APB1 prescaler
  apb2_prescaler: 1               # APB2 prescaler
  flash_latency: 2                # Flash wait states
```

**Calculations:**
- PLL Input: `hse_hz / pll.m` = 8 MHz / 4 = 2 MHz
- VCO: `pll_input × pll.n` = 2 MHz × 168 = 336 MHz
- SYSCLK: `vco / pll.p` = 336 MHz / 4 = 84 MHz
- USB: `vco / pll.q` = 336 MHz / 7 = 48 MHz

#### PLL Clock (SAME70)

```yaml
clock:
  source: PLL
  system_clock_hz: 300000000
  hse_hz: 12000000
  pll:
    m: 1      # PLLA input divider
    n: 25     # PLLA multiplier (N = 24+1)
    p: 2      # MCK divider (PLLA/P)
  flash_latency: 6
```

**Calculations:**
- PLLA: `hse_hz × pll.n / pll.m` = 12 MHz × 25 / 1 = 300 MHz
- MCK: `plla / pll.p` = 300 MHz / 2 = 150 MHz

### LEDs

```yaml
leds:
  - name: led_green              # C++ identifier (snake_case)
    color: green                  # red|green|blue|yellow|orange|white
    port: GPIOA                   # GPIO port (GPIOA-GPIOK)
    pin: 5                        # Pin number (0-15)
    active_high: true             # true = on when HIGH, false = on when LOW
    description: "User LED LD2"   # Optional description
```

**Generated C++:**
```cpp
struct LedConfig {
    using led_green = GpioPin<peripherals::GPIOA, 5>;
    static constexpr bool led_green_active_high = true;
};
```

### Buttons

```yaml
buttons:
  - name: button_user
    port: GPIOC
    pin: 13
    active_high: false            # false = pressed when LOW
    pull: none                    # none|up|down
    description: "User button B1"
```

**Generated C++:**
```cpp
struct ButtonConfig {
    using button_user = GpioPin<peripherals::GPIOC, 13>;
    static constexpr bool button_user_active_high = false;
};
```

### UART

```yaml
uart:
  - name: console                 # C++ identifier
    instance: USART2              # UART/USART peripheral instance
    tx_port: GPIOA
    tx_pin: 2
    rx_port: GPIOA                # Optional for TX-only
    rx_pin: 3                     # Optional for TX-only
    baud_rate: 115200             # Default baud rate
    description: "ST-Link VCP"
```

**Generated C++:**
```cpp
struct UartConfig {
    using console_tx = GpioPin<peripherals::GPIOA, 2>;
    using console_rx = GpioPin<peripherals::GPIOA, 3>;
    static constexpr uint32_t console_baud_rate = 115200;
};
```

### SPI

```yaml
spi:
  - name: arduino_spi
    instance: SPI1
    sck_port: GPIOA
    sck_pin: 5
    mosi_port: GPIOA
    mosi_pin: 7
    miso_port: GPIOA
    miso_pin: 6
    description: "Arduino SPI"
```

### I2C

```yaml
i2c:
  - name: arduino_i2c
    instance: I2C1
    scl_port: GPIOB
    scl_pin: 8
    sda_port: GPIOB
    sda_pin: 9
    speed_hz: 100000              # 100kHz|400kHz|1MHz
    description: "Arduino I2C"
```

## Validation

### JSON Schema Validation

All YAML files are validated against `boards/board-schema.json`:

```bash
# Validate single board
python3 -m tools.codegen.cli.generators.board_generator \
    boards/nucleo_f401re/board.yaml --validate-only
```

**Common Validation Errors:**

1. **Invalid Platform**
   ```yaml
   platform: stm32f5  # ❌ Not in enum
   ```
   Fix: Use `stm32f4`, `stm32f7`, etc.

2. **Invalid Pin Number**
   ```yaml
   pin: 16  # ❌ Max is 15
   ```
   Fix: Use 0-15

3. **Invalid GPIO Port**
   ```yaml
   port: GPIO_A  # ❌ Wrong format
   ```
   Fix: Use `GPIOA`

4. **Invalid Name**
   ```yaml
   name: led-green  # ❌ Must be snake_case
   ```
   Fix: Use `led_green`

### Clock Calculation Validation

Generator automatically calculates and documents clock tree:

```cpp
/**
 * PLL Configuration:
 *   Input:  8000000 Hz (HSE)
 *   PLL input: HSE / M = 8000000 Hz / 4 = 2000000 Hz
 *   VCO:    2000000 Hz × 168 = 336000000 Hz
 *   SYSCLK: VCO / P = 336000000 Hz / 4 = 84000000 Hz
 */
```

## Complete Example

See `boards/nucleo_f401re/board.yaml` for a complete, real-world example.

## Migrating Existing Boards

### From board_config.hpp to YAML

1. **Extract LED Configuration**

   Old C++:
   ```cpp
   struct LedConfig {
       using led_green = GpioPin<peripherals::GPIOA, 5>;
       static constexpr bool led_green_active_high = true;
   };
   ```

   New YAML:
   ```yaml
   leds:
     - name: led_green
       port: GPIOA
       pin: 5
       active_high: true
   ```

2. **Extract Clock Configuration**

   Old C++:
   ```cpp
   struct ClockConfig {
       static constexpr uint32_t hse_hz = 8'000'000;
       static constexpr uint32_t system_clock_hz = 84'000'000;
       static constexpr uint32_t pll_m = 4;
       static constexpr uint32_t pll_n = 168;
   };
   ```

   New YAML:
   ```yaml
   clock:
     source: PLL
     system_clock_hz: 84000000
     hse_hz: 8000000
     pll: { m: 4, n: 168, p: 4, q: 7 }
   ```

3. **Generate New Header**

   ```bash
   python3 -m tools.codegen.cli.generators.board_generator \
       boards/my_board/board.yaml
   ```

4. **Verify Generated Code**

   Compare old and new `board_config.hpp` to ensure equivalence.

## Benefits

### Before (Manual C++)

❌ **Duplication**: Same info in multiple places
❌ **Error-Prone**: Manual clock calculations
❌ **Hard to Read**: C++ template syntax
❌ **No Validation**: Typos found at compile-time
❌ **Platform-Specific**: Different patterns per platform

### After (YAML)

✅ **Single Source**: One YAML file for all config
✅ **Auto-Calculated**: Clock tree computed automatically
✅ **Self-Documenting**: YAML is human-readable
✅ **Schema Validated**: Errors caught before code gen
✅ **Portable**: Same format for all platforms

## Advanced Usage

### Platform-Specific Configuration

Use YAML anchors for shared configuration:

```yaml
common: &common_uart
  baud_rate: 115200

uart:
  - <<: *common_uart
    name: console
    instance: USART2
    tx_port: GPIOA
    tx_pin: 2

  - <<: *common_uart
    name: debug
    instance: USART3
    tx_port: GPIOB
    tx_pin: 10
```

### Multi-LED Patterns

```yaml
leds:
  - name: led_red
    color: red
    port: GPIOA
    pin: 0
    active_high: true

  - name: led_green
    color: green
    port: GPIOA
    pin: 1
    active_high: true

  - name: led_blue
    color: blue
    port: GPIOA
    pin: 2
    active_high: true
```

Generated as RGB LED control:

```cpp
struct LedConfig {
    using led_red   = GpioPin<peripherals::GPIOA, 0>;
    using led_green = GpioPin<peripherals::GPIOA, 1>;
    using led_blue  = GpioPin<peripherals::GPIOA, 2>;
};
```

## Troubleshooting

### Schema Validation Fails

**Error:** `board.name is required`

**Fix:** Add required fields:
```yaml
board:
  name: "My Board"
  vendor: "Manufacturer"
  version: "1.0.0"
```

### Generated Code Won't Compile

**Error:** `'GPIOA' is not a member of 'peripherals'`

**Fix:** Ensure platform matches MCU. Check `src/hal/vendors/<platform>/<mcu>/peripherals.hpp` exists.

### Clock Calculations Wrong

**Error:** Generated comments show wrong frequency

**Fix:** Verify PLL parameters match datasheet. Example for STM32F4:
- VCO range: 100-432 MHz
- PLL input: 1-2 MHz (recommended)
- System clock max: 84 MHz (F401), 180 MHz (F429)

## See Also

- `boards/board-schema.json` - Complete JSON schema
- `boards/nucleo_f401re/board.yaml` - Complete example
- `boards/same70_xplained/board.yaml` - SAME70 example
- `tools/codegen/cli/loaders/board_yaml_loader.py` - YAML loader
- `tools/codegen/cli/generators/board_generator.py` - Code generator
- `tools/codegen/templates/board_config.hpp.j2` - Jinja2 template
