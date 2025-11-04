"""
ESP32 Pin Generator

Generates GPIO pin definitions and GPIO Matrix helper code for ESP32 family.
ESP32 uses a flexible GPIO Matrix that allows routing any peripheral to any pin.
"""

import sys
from pathlib import Path

# Add parent directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger
from cli.vendors.espressif.esp32_pin_functions import ESP32_PIN_FUNCTIONS, GPIO_CAPABILITIES


# ESP32 variant configurations
ESP32_VARIANTS = {
    "ESP32": {
        "full_name": "ESP32 (Dual-core Xtensa LX6)",
        "num_gpios": 40,
        "has_gpio_matrix": True,
        "vendor_path": "espressif",
        "family": "esp32",
    },
}


def generate_pins_header(variant_name: str, config: dict, output_dir: Path) -> bool:
    """Generate pins.hpp for ESP32 variant"""
    
    try:
        num_gpios = config["num_gpios"]
        
        content = f'''#pragma once

#include <cstdint>

namespace alloy::hal::{config["vendor_path"]}::{config["family"]}::pins {{

// ============================================================================
// Pin Definitions for {variant_name}
// ============================================================================

'''

        # Generate GPIO pin constants
        for gpio_num in range(num_gpios):
            pin_name = f"GPIO{gpio_num}"
            if pin_name in ESP32_PIN_FUNCTIONS:
                functions = ESP32_PIN_FUNCTIONS[pin_name]
                # Add comment with main functions
                func_list = ", ".join([f.function_name for f in functions[:3]])
                content += f"constexpr uint8_t GPIO{gpio_num:2d} = {gpio_num:2d};  // {func_list}\n"
            else:
                content += f"constexpr uint8_t GPIO{gpio_num:2d} = {gpio_num:2d};\n"
        
        content += '''
// ============================================================================
// Pin Aliases (common names)
// ============================================================================

// LED pin (GPIO2 on most ESP32 DevKits)
constexpr uint8_t LED = GPIO2;

// UART0 (programming/debug)
constexpr uint8_t UART0_TX = GPIO1;
constexpr uint8_t UART0_RX = GPIO3;

// UART2 (user UART)
constexpr uint8_t UART2_TX = GPIO17;
constexpr uint8_t UART2_RX = GPIO16;

// I2C (default pins)
constexpr uint8_t I2C_SDA = GPIO21;
constexpr uint8_t I2C_SCL = GPIO22;

// SPI (VSPI - default user SPI)
constexpr uint8_t SPI_MOSI = GPIO23;
constexpr uint8_t SPI_MISO = GPIO19;
constexpr uint8_t SPI_CLK  = GPIO18;
constexpr uint8_t SPI_CS   = GPIO5;

// DAC
constexpr uint8_t DAC1 = GPIO25;
constexpr uint8_t DAC2 = GPIO26;

// ADC1 (can use while WiFi is active)
constexpr uint8_t ADC1_CH0 = GPIO36;  // VP (input-only)
constexpr uint8_t ADC1_CH3 = GPIO39;  // VN (input-only)
constexpr uint8_t ADC1_CH4 = GPIO32;
constexpr uint8_t ADC1_CH5 = GPIO33;
constexpr uint8_t ADC1_CH6 = GPIO34;  // (input-only)
constexpr uint8_t ADC1_CH7 = GPIO35;  // (input-only)

}  // namespace alloy::hal::espressif::esp32::pins
'''

        # Write file
        output_file = output_dir / "pins.hpp"
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(content)
        
        return True
        
    except Exception as e:
        logger.error(f"Failed to generate pins header: {e}")
        return False


def generate_gpio_matrix_header(variant_name: str, config: dict, output_dir: Path) -> bool:
    """Generate gpio_matrix.hpp with peripheral routing helpers"""
    
    try:
        content = f'''#pragma once

#include <cstdint>

namespace alloy::hal::{config["vendor_path"]}::{config["family"]} {{

// ============================================================================
// GPIO Matrix Configuration for {variant_name}
// ============================================================================
//
// ESP32 has a flexible GPIO Matrix that allows routing any peripheral signal
// to any GPIO pin. This is different from traditional microcontrollers where
// alternate functions are fixed per pin.
//
// Signal routing is controlled by:
// - GPIO_FUNCx_OUT_SEL_CFG (output signal selection)  
// - GPIO_FUNCy_IN_SEL_CFG (input signal selection)
//
// This header provides helper constants and types for GPIO Matrix configuration.
// ============================================================================

// GPIO Matrix signal indices (selected common ones)
namespace signal {{

// UART signals
constexpr uint32_t UART0_TXD = 14;
constexpr uint32_t UART0_RXD = 14;
constexpr uint32_t UART1_TXD = 15;
constexpr uint32_t UART1_RXD = 15;
constexpr uint32_t UART2_TXD = 16;
constexpr uint32_t UART2_RXD = 16;

// SPI signals (HSPI)
constexpr uint32_t HSPICLK  = 71;
constexpr uint32_t HSPIQ    = 72;  // MISO
constexpr uint32_t HSPID    = 73;  // MOSI
constexpr uint32_t HSPICS0  = 74;

// SPI signals (VSPI)
constexpr uint32_t VSPICLK  = 63;
constexpr uint32_t VSPIQ    = 64;  // MISO
constexpr uint32_t VSPID    = 65;  // MOSI
constexpr uint32_t VSPICS0  = 66;

// I2C signals
constexpr uint32_t I2C0_SCL_OUT = 29;
constexpr uint32_t I2C0_SDA_OUT = 30;
constexpr uint32_t I2C0_SCL_IN  = 29;
constexpr uint32_t I2C0_SDA_IN  = 30;

constexpr uint32_t I2C1_SCL_OUT = 31;
constexpr uint32_t I2C1_SDA_OUT = 32;
constexpr uint32_t I2C1_SCL_IN  = 31;
constexpr uint32_t I2C1_SDA_IN  = 32;

// PWM/LEDC signals
constexpr uint32_t LEDC_HS_SIG0 = 45;
constexpr uint32_t LEDC_HS_SIG1 = 46;
constexpr uint32_t LEDC_HS_SIG2 = 47;
constexpr uint32_t LEDC_HS_SIG3 = 48;
constexpr uint32_t LEDC_HS_SIG4 = 49;
constexpr uint32_t LEDC_HS_SIG5 = 50;
constexpr uint32_t LEDC_HS_SIG6 = 51;
constexpr uint32_t LEDC_HS_SIG7 = 52;

constexpr uint32_t LEDC_LS_SIG0 = 53;
constexpr uint32_t LEDC_LS_SIG1 = 54;
constexpr uint32_t LEDC_LS_SIG2 = 55;
constexpr uint32_t LEDC_LS_SIG3 = 56;
constexpr uint32_t LEDC_LS_SIG4 = 57;
constexpr uint32_t LEDC_LS_SIG5 = 58;
constexpr uint32_t LEDC_LS_SIG6 = 59;
constexpr uint32_t LEDC_LS_SIG7 = 60;

// Special signals
constexpr uint32_t SIG_GPIO = 0x100;  // Direct GPIO mode

}}  // namespace signal

// GPIO capabilities
struct GpioCapabilities {{
    bool can_output;
    bool can_input;
    bool has_pullup;
    bool has_pulldown;
    bool is_strapping;  // Used for boot mode selection
}};

// Get capabilities for a GPIO pin
constexpr GpioCapabilities get_gpio_capabilities(uint8_t gpio_num) {{
    // GPIO 34-39 are input-only
    if (gpio_num >= 34 && gpio_num <= 39) {{
        return GpioCapabilities{{
            .can_output = false,
            .can_input = true,
            .has_pullup = false,
            .has_pulldown = false,
            .is_strapping = false,
        }};
    }}
    
    // Strapping pins: 0, 2, 5, 12, 15
    bool is_strapping = (gpio_num == 0 || gpio_num == 2 || gpio_num == 5 || 
                        gpio_num == 12 || gpio_num == 15);
    
    // All other pins have full capabilities
    return GpioCapabilities{{
        .can_output = true,
        .can_input = true,
        .has_pullup = true,
        .has_pulldown = true,
        .is_strapping = is_strapping,
    }};
}}

}}  // namespace alloy::hal::espressif::esp32
'''

        # Write file
        output_file = output_dir / "gpio_matrix.hpp"
        output_file.write_text(content)
        
        return True
        
    except Exception as e:
        logger.error(f"Failed to generate GPIO matrix header: {e}")
        return False


def generate_variant(variant_name: str, config: dict) -> bool:
    """Generate all files for a variant"""
    
    logger.info(f"Generating {variant_name}...")
    
    # Determine output directory
    repo_root = CODEGEN_DIR.parent.parent
    output_dir = repo_root / "src" / "hal" / "vendors" / config["vendor_path"] / config["family"]
    
    # Generate files
    success = True
    success &= generate_pins_header(variant_name, config, output_dir)
    success &= generate_gpio_matrix_header(variant_name, config, output_dir)
    
    if success:
        print_success(f"  ✓ Generated {variant_name}")
        print_info(f"    - pins.hpp")
        print_info(f"    - gpio_matrix.hpp")
    else:
        print_error(f"  ✗ Failed to generate {variant_name}")
    
    return success


def main():
    """Main generator function"""
    
    print_header("ESP32 Pin Generator", "=")
    
    total_variants = len(ESP32_VARIANTS)
    success_count = 0
    
    for variant_name, config in ESP32_VARIANTS.items():
        if generate_variant(variant_name, config):
            success_count += 1
    
    print()
    print_header("Generation Summary", "-")
    print_info(f"Total variants: {total_variants}")
    print_success(f"Generated: {success_count}")
    
    if success_count < total_variants:
        print_error(f"Failed: {total_variants - success_count}")
        return 1
    
    print_success("All ESP32 variants generated successfully!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
