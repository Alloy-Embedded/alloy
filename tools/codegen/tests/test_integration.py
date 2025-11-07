"""
Integration tests for the complete code generation pipeline

These tests validate that all generators work together correctly,
producing valid C++ code that compiles and integrates properly.
"""

import unittest
import tempfile
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_registers import generate_register_struct
from cli.generators.generate_enums import generate_enums_header
from cli.generators.generate_pin_functions import (
    PinFunctionDatabase,
    generate_pin_functions_header
)
from cli.generators.generate_register_map import (
    generate_register_map_header,
    get_generated_peripheral_files
)
from tests.test_helpers import (
    create_test_device,
    create_test_peripheral,
    create_test_register,
    create_test_field,
    AssertHelpers
)


class TestFullPipeline(unittest.TestCase):
    """Test complete code generation pipeline"""

    def test_registers_and_enums_integration(self):
        """Test that registers and enums can be generated and used together"""
        # Create device with peripheral that has enumerated fields
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"INPUT": 0, "OUTPUT": 1, "ALTERNATE": 2}

        register = create_test_register("MODER", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIOA", 0x40020000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIOA": peripheral}

        # Generate both headers
        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        # Both should generate valid code
        self.assertIsInstance(register_code, str)
        self.assertIsInstance(enum_code, str)

        # Register code should have the register
        self.assertIn("MODER", register_code)

        # Enum code should have the enum
        self.assertIn("GPIOA_MODER_MODE", enum_code)

    def test_multiple_peripherals_generation(self):
        """Test generating code for multiple peripherals"""
        device = create_test_device()

        # Create multiple peripherals
        rcc = create_test_peripheral("RCC", 0x40021000, registers=[
            create_test_register("CR", 0x0000),
            create_test_register("CFGR", 0x0004),
        ])

        gpio = create_test_peripheral("GPIOA", 0x40020000, registers=[
            create_test_register("MODER", 0x0000),
            create_test_register("ODR", 0x0014),
        ])

        device.peripherals = {"RCC": rcc, "GPIOA": gpio}

        # Generate registers for both
        rcc_code = generate_register_struct(rcc, device)
        gpio_code = generate_register_struct(gpio, device)

        # Should have distinct code for each
        self.assertIn("RCC_Registers", rcc_code)
        self.assertIn("GPIOA_Registers", gpio_code)
        self.assertIn("CR", rcc_code)
        self.assertIn("MODER", gpio_code)

    def test_register_map_includes_all_generated_files(self):
        """Test that register_map includes all generated peripheral files"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers and bitfields are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            bitfields_dir = family_dir / "bitfields"
            bitfields_dir.mkdir()

            # Create peripheral files
            peripherals = ["rcc", "gpio", "usart", "spi"]
            for periph in peripherals:
                (registers_dir / f"{periph}_registers.hpp").touch()
                (bitfields_dir / f"{periph}_bitfields.hpp").touch()

            # Create optional files (MCU-level)
            (mcu_dir / "enums.hpp").touch()
            (mcu_dir / "pin_functions.hpp").touch()

            # Generate register map
            device = create_test_device()
            register_map = generate_register_map_header(device, mcu_dir)

            # Should include all peripherals with family-level path
            for periph in peripherals:
                self.assertIn(f"../registers/{periph}_registers.hpp", register_map)
                self.assertIn(f"../bitfields/{periph}_bitfields.hpp", register_map)

            # Should include optional files (MCU-level)
            self.assertIn("enums.hpp", register_map)
            self.assertIn("pin_functions.hpp", register_map)

    def test_namespace_consistency_across_generators(self):
        """Test that all generators use consistent namespaces (family-level for registers)"""
        device = create_test_device(
            name="STM32F103C8",
            vendor="ST",
            family="STM32F1"
        )

        peripheral = create_test_peripheral("RCC", 0x40021000, registers=[
            create_test_register("CR", 0x0000)
        ])
        device.peripherals = {"RCC": peripheral}

        # Generate from different generators
        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        # Registers now use family-level namespace (no MCU name)
        self.assertIn("namespace alloy::hal::st::stm32f1::rcc", register_code)
        # Enums remain MCU-specific (MCU-level namespace)
        self.assertIn("namespace alloy::hal::st::stm32f1::stm32f103c8", enum_code)


class TestEndToEndWorkflow(unittest.TestCase):
    """Test complete end-to-end workflows"""

    def test_simple_mcu_complete_generation(self):
        """Test generating all code for a simple MCU"""
        device = create_test_device(
            name="TEST_MCU",
            vendor="TestVendor",
            family="TestFamily"
        )

        # Create a peripheral with register, fields, and enums
        field1 = create_test_field("ENABLE", 0, 1)
        field1.enum_values = {"DISABLED": 0, "ENABLED": 1}

        field2 = create_test_field("MODE", 1, 2)
        field2.enum_values = {"MODE_A": 0, "MODE_B": 1, "MODE_C": 2}

        register = create_test_register("CTRL", 0x0000, fields=[field1, field2])
        peripheral = create_test_peripheral("PERIPH", 0x40000000, registers=[register])
        device.peripherals = {"PERIPH": peripheral}

        # Generate all components
        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()
            (registers_dir / "periph_registers.hpp").touch()

            # Enums are at MCU level
            (mcu_dir / "enums.hpp").touch()

            register_map = generate_register_map_header(device, mcu_dir)

            # All components should be generated
            self.assertIsInstance(register_code, str)
            self.assertIsInstance(enum_code, str)
            self.assertIsInstance(register_map, str)

            # Register map should reference generated files with correct paths
            self.assertIn("../registers/periph_registers.hpp", register_map)
            self.assertIn("enums.hpp", register_map)

    def test_peripheral_with_all_features(self):
        """Test peripheral using all available features"""
        # Create comprehensive peripheral
        field1 = create_test_field("EN", 0, 1)
        field1.enum_values = {"DISABLED": 0, "ENABLED": 1}

        field2 = create_test_field("MODE", 1, 3)
        field2.enum_values = {
            "MODE0": 0, "MODE1": 1, "MODE2": 2,
            "MODE3": 3, "MODE4": 4
        }

        registers = [
            create_test_register("CR", 0x0000, fields=[field1, field2]),
            create_test_register("SR", 0x0004),
            create_test_register("DR", 0x0008),
            create_test_register("ARR", 0x0010, dim=4),  # Array
        ]

        peripheral = create_test_peripheral("TIMER", 0x40000000, registers=registers)
        device = create_test_device()
        device.peripherals = {"TIMER": peripheral}

        # Generate code
        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        # Should have all components
        AssertHelpers.assert_contains_all(
            register_code,
            "TIMER_Registers",
            "CR", "SR", "DR", "ARR[4]"
        )

        AssertHelpers.assert_contains_all(
            enum_code,
            "TIMER_CR_EN",
            "TIMER_CR_MODE"
        )


class TestCrossDependencies(unittest.TestCase):
    """Test cross-dependencies between generated components"""

    def test_bitfields_reference_enums(self):
        """Test that bitfields can reference enum values"""
        # Create field with enum
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"INPUT": 0, "OUTPUT": 1}

        register = create_test_register("MODER", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        # Generate code
        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        # Enum should exist
        self.assertIn("GPIO_MODER_MODE", enum_code)
        self.assertIn("INPUT", enum_code)
        self.assertIn("OUTPUT", enum_code)

        # Register should have the field
        self.assertIn("MODER", register_code)

    def test_register_map_aggregates_all(self):
        """Test that register_map properly aggregates all components"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Create full directory structure at family level
            for subdir in ["registers", "bitfields"]:
                (family_dir / subdir).mkdir()

            # Create various peripheral files at family level
            peripherals = ["gpio", "usart", "spi", "i2c", "timer"]
            for periph in peripherals:
                (family_dir / "registers" / f"{periph}_registers.hpp").touch()
                (family_dir / "bitfields" / f"{periph}_bitfields.hpp").touch()

            # Create all optional files at MCU level
            (mcu_dir / "enums.hpp").touch()
            (mcu_dir / "pin_functions.hpp").touch()
            (mcu_dir / "bitfield_utils.hpp").touch()

            device = create_test_device()
            register_map = generate_register_map_header(device, mcu_dir)

            # Should include everything with correct paths
            for periph in peripherals:
                self.assertIn(f"../registers/{periph}_registers.hpp", register_map)
                self.assertIn(f"../bitfields/{periph}_bitfields.hpp", register_map)

            self.assertIn("enums.hpp", register_map)
            self.assertIn("pin_functions.hpp", register_map)


class TestRealWorldScenarios(unittest.TestCase):
    """Test realistic scenarios similar to actual MCUs"""

    def test_stm32_like_gpio(self):
        """Test GPIO peripheral similar to STM32"""
        # STM32 GPIO has MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, etc.
        registers = [
            create_test_register("MODER", 0x0000),    # Mode register
            create_test_register("OTYPER", 0x0004),   # Output type
            create_test_register("OSPEEDR", 0x0008),  # Output speed
            create_test_register("PUPDR", 0x000C),    # Pull-up/down
            create_test_register("IDR", 0x0010),      # Input data
            create_test_register("ODR", 0x0014),      # Output data
            create_test_register("BSRR", 0x0018),     # Bit set/reset
            create_test_register("LCKR", 0x001C),     # Lock register
            create_test_register("AFRL", 0x0020),     # Alternate func low
            create_test_register("AFRH", 0x0024),     # Alternate func high
        ]

        peripheral = create_test_peripheral("GPIOA", 0x40020000, registers=registers)
        device = create_test_device(name="STM32F4", vendor="ST", family="STM32F4")

        register_code = generate_register_struct(peripheral, device)

        # Should have all registers
        for reg in ["MODER", "OTYPER", "OSPEEDR", "PUPDR", "IDR", "ODR", "BSRR", "LCKR", "AFRL", "AFRH"]:
            self.assertIn(reg, register_code)

    def test_atmel_like_pio(self):
        """Test PIO peripheral similar to Atmel SAME70"""
        # SAME70 PIO has various registers including arrays
        registers = [
            create_test_register("PER", 0x0000),      # PIO Enable
            create_test_register("PDR", 0x0004),      # PIO Disable
            create_test_register("PSR", 0x0008),      # PIO Status
            create_test_register("OER", 0x0010),      # Output Enable
            create_test_register("ODR", 0x0014),      # Output Disable
            create_test_register("OSR", 0x0018),      # Output Status
            create_test_register("IFER", 0x0020),     # Glitch Filter Enable
            create_test_register("IFDR", 0x0024),     # Glitch Filter Disable
            create_test_register("IFSR", 0x0028),     # Glitch Filter Status
            create_test_register("SODR", 0x0030),     # Set Output Data
            create_test_register("CODR", 0x0034),     # Clear Output Data
            create_test_register("ODSR", 0x0038),     # Output Data Status
            create_test_register("PDSR", 0x003C),     # Pin Data Status
            create_test_register("ABCDSR", 0x0070, dim=2),  # Peripheral Select (array)
        ]

        peripheral = create_test_peripheral("PIOA", 0x400E0E00, registers=registers)
        device = create_test_device(name="ATSAME70Q21", vendor="Atmel", family="SAME70")

        register_code = generate_register_struct(peripheral, device)

        # Should have array register
        self.assertIn("ABCDSR[2]", register_code)
        # Should have single registers
        self.assertIn("PER", register_code)
        self.assertIn("SODR", register_code)

    def test_multiple_peripherals_with_shared_enums(self):
        """Test multiple peripherals that might have similar enum names"""
        device = create_test_device()

        # Create two peripherals with similar field names
        for periph_name in ["USART1", "USART2"]:
            field = create_test_field("MODE", 0, 2)
            field.enum_values = {"ASYNC": 0, "SYNC": 1, "SPI": 2}

            register = create_test_register("CR1", 0x0000, fields=[field])
            peripheral = create_test_peripheral(periph_name, 0x40000000, registers=[register])
            device.peripherals[periph_name] = peripheral

        enum_code = generate_enums_header(device)

        # Should have separate enums for each peripheral
        self.assertIn("USART1_CR1_MODE", enum_code)
        self.assertIn("USART2_CR1_MODE", enum_code)


class TestEdgeCasesIntegration(unittest.TestCase):
    """Test edge cases in integration scenarios"""

    def test_empty_device(self):
        """Test generating code for device with no peripherals"""
        device = create_test_device()
        device.peripherals = {}

        enum_code = generate_enums_header(device)

        # Should generate valid but minimal code
        self.assertIn("#pragma once", enum_code)
        self.assertIn("namespace alloy::hal", enum_code)

    def test_peripheral_without_enums(self):
        """Test peripheral that has no enumerated fields"""
        registers = [
            create_test_register("DATA", 0x0000),
            create_test_register("STATUS", 0x0004),
        ]

        peripheral = create_test_peripheral("SIMPLE", 0x40000000, registers=registers)
        device = create_test_device()
        device.peripherals = {"SIMPLE": peripheral}

        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        # Register code should work
        self.assertIn("SIMPLE_Registers", register_code)

        # Enum code should be minimal
        self.assertIn("No enumerated values found", enum_code)


if __name__ == "__main__":
    unittest.main(verbosity=2)
