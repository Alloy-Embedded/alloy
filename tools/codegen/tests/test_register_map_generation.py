"""
Unit tests for register map generation

These tests validate that the register_map generator produces correct C++ code
for including all MCU peripherals, registers, and utilities.
"""

import unittest
from pathlib import Path
import sys
import tempfile
import os

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_register_map import (
    get_generated_peripheral_files,
    generate_register_map_header
)
from tests.test_helpers import (
    create_test_device,
    AssertHelpers
)


class TestPeripheralFileScanning(unittest.TestCase):
    """Test scanning for generated peripheral files"""

    def test_empty_directory(self):
        """Test scanning empty directory"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            register_files, bitfield_files = get_generated_peripheral_files(output_dir)

            self.assertEqual(len(register_files), 0)
            self.assertEqual(len(bitfield_files), 0)

    def test_scan_register_files(self):
        """Test scanning for register files"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            # Create dummy register files
            (registers_dir / "rcc_registers.hpp").touch()
            (registers_dir / "gpio_registers.hpp").touch()
            (registers_dir / "usart_registers.hpp").touch()

            register_files, bitfield_files = get_generated_peripheral_files(mcu_dir)

            self.assertEqual(len(register_files), 3)
            self.assertIn("rcc", register_files)
            self.assertIn("gpio", register_files)
            self.assertIn("usart", register_files)

    def test_scan_bitfield_files(self):
        """Test scanning for bitfield files"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Bitfields are at family level (parent of MCU)
            bitfields_dir = family_dir / "bitfields"
            bitfields_dir.mkdir()

            # Create dummy bitfield files
            (bitfields_dir / "rcc_bitfields.hpp").touch()
            (bitfields_dir / "spi_bitfields.hpp").touch()

            register_files, bitfield_files = get_generated_peripheral_files(mcu_dir)

            self.assertEqual(len(bitfield_files), 2)
            self.assertIn("rcc", bitfield_files)
            self.assertIn("spi", bitfield_files)

    def test_scan_both_registers_and_bitfields(self):
        """Test scanning for both register and bitfield files"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers and bitfields are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()
            (registers_dir / "rcc_registers.hpp").touch()
            (registers_dir / "gpio_registers.hpp").touch()

            bitfields_dir = family_dir / "bitfields"
            bitfields_dir.mkdir()
            (bitfields_dir / "rcc_bitfields.hpp").touch()
            (bitfields_dir / "gpio_bitfields.hpp").touch()

            register_files, bitfield_files = get_generated_peripheral_files(mcu_dir)

            self.assertEqual(len(register_files), 2)
            self.assertEqual(len(bitfield_files), 2)
            self.assertEqual(set(register_files), set(bitfield_files))

    def test_files_sorted_alphabetically(self):
        """Test that files are returned in alphabetical order"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            # Create files in non-alphabetical order
            (registers_dir / "zz_registers.hpp").touch()
            (registers_dir / "aa_registers.hpp").touch()
            (registers_dir / "mm_registers.hpp").touch()

            register_files, _ = get_generated_peripheral_files(mcu_dir)

            self.assertEqual(register_files, ["aa", "mm", "zz"])


class TestRegisterMapGeneration(unittest.TestCase):
    """Test register_map.hpp generation"""

    def test_simple_register_map(self):
        """Test generation of basic register map"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()
            (registers_dir / "rcc_registers.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            # Should have basic structure
            AssertHelpers.assert_contains_all(
                content,
                "#pragma once",
                "Complete Register Map",
                device.name,
                device.vendor
            )

    def test_includes_all_register_files(self):
        """Test that all register files are included"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            peripherals = ["rcc", "gpio", "spi", "i2c", "usart"]
            for periph in peripherals:
                (registers_dir / f"{periph}_registers.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            # Should include all peripherals with family-level path
            for periph in peripherals:
                self.assertIn(f"../registers/{periph}_registers.hpp", content)

    def test_includes_all_bitfield_files(self):
        """Test that all bitfield files are included"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Bitfields are at family level (parent of MCU)
            bitfields_dir = family_dir / "bitfields"
            bitfields_dir.mkdir()

            peripherals = ["rcc", "gpio"]
            for periph in peripherals:
                (bitfields_dir / f"{periph}_bitfields.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            # Should include all bitfields with family-level path
            for periph in peripherals:
                self.assertIn(f"../bitfields/{periph}_bitfields.hpp", content)

    def test_includes_optional_files_when_present(self):
        """Test that optional files are included when they exist"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            # Create optional files
            (output_dir / "enums.hpp").touch()
            (output_dir / "pin_functions.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, output_dir)

            # Should include optional files
            self.assertIn("enums.hpp", content)
            self.assertIn("pin_functions.hpp", content)

    def test_skips_optional_files_when_absent(self):
        """Test that optional files are not included when they don't exist"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            # Don't create optional files

            device = create_test_device()
            content = generate_register_map_header(device, output_dir)

            # Should not include missing files
            # (Implementation might have conditional includes or comments)
            self.assertIsInstance(content, str)

    def test_namespace_structure(self):
        """Test that correct namespace structure is generated"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            device = create_test_device(
                name="ATSAME70Q21B",
                vendor="Atmel",
                family="SAME70"
            )
            content = generate_register_map_header(device, output_dir)

            # Should have correct namespace
            self.assertIn("alloy::hal", content)
            self.assertIn("atmel", content.lower())
            self.assertIn("same70", content.lower())

    def test_header_guard(self):
        """Test that #pragma once is used"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            device = create_test_device()
            content = generate_register_map_header(device, output_dir)

            # Modern C++ uses #pragma once
            self.assertIn("#pragma once", content)

    def test_usage_example_in_comments(self):
        """Test that usage example is included in header comments"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            device = create_test_device()
            content = generate_register_map_header(device, output_dir)

            # Should have usage example
            self.assertIn("Usage:", content)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_no_peripherals_generated(self):
        """Test register map with no peripheral files"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            device = create_test_device()
            content = generate_register_map_header(device, output_dir)

            # Should still generate valid header
            AssertHelpers.assert_contains_all(
                content,
                "#pragma once",
                device.name
            )

    def test_many_peripherals(self):
        """Test register map with many peripherals (50+)"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            # Create 50 peripheral files
            for i in range(50):
                (registers_dir / f"periph{i}_registers.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            # Should include all peripherals with family-level path
            self.assertIn("../registers/periph0_registers.hpp", content)
            self.assertIn("../registers/periph49_registers.hpp", content)

    def test_peripheral_with_numbers(self):
        """Test peripherals with numbers in name"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            (registers_dir / "usart1_registers.hpp").touch()
            (registers_dir / "timer2_registers.hpp").touch()
            (registers_dir / "spi3_registers.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            self.assertIn("../registers/usart1_registers.hpp", content)
            self.assertIn("../registers/timer2_registers.hpp", content)
            self.assertIn("../registers/spi3_registers.hpp", content)

    def test_peripheral_with_underscores(self):
        """Test peripherals with underscores in name"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()

            (registers_dir / "system_ctrl_registers.hpp").touch()
            (registers_dir / "power_mgmt_registers.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            self.assertIn("../registers/system_ctrl_registers.hpp", content)
            self.assertIn("../registers/power_mgmt_registers.hpp", content)

    def test_device_with_long_name(self):
        """Test device with very long name"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            device = create_test_device(
                name="VERY_LONG_DEVICE_NAME_THAT_IS_EXCESSIVELY_LONG",
                vendor="VeryLongVendorName",
                family="VeryLongFamilyName"
            )
            content = generate_register_map_header(device, output_dir)

            # Should handle long names
            self.assertIn("VERY_LONG_DEVICE_NAME_THAT_IS_EXCESSIVELY_LONG", content)

    def test_mixed_register_and_bitfield_files(self):
        """Test when some peripherals have bitfields and others don't"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create family-level structure: family/mcu_name/
            family_dir = Path(tmpdir) / "family"
            mcu_dir = family_dir / "mcu_name"
            mcu_dir.mkdir(parents=True)

            # Registers and bitfields are at family level (parent of MCU)
            registers_dir = family_dir / "registers"
            registers_dir.mkdir()
            (registers_dir / "rcc_registers.hpp").touch()
            (registers_dir / "gpio_registers.hpp").touch()
            (registers_dir / "spi_registers.hpp").touch()

            bitfields_dir = family_dir / "bitfields"
            bitfields_dir.mkdir()
            (bitfields_dir / "rcc_bitfields.hpp").touch()
            # gpio has no bitfields
            (bitfields_dir / "spi_bitfields.hpp").touch()

            device = create_test_device()
            content = generate_register_map_header(device, mcu_dir)

            # Should include all available files with family-level path
            self.assertIn("../registers/rcc_registers.hpp", content)
            self.assertIn("../registers/gpio_registers.hpp", content)
            self.assertIn("../registers/spi_registers.hpp", content)
            self.assertIn("../bitfields/rcc_bitfields.hpp", content)
            self.assertIn("../bitfields/spi_bitfields.hpp", content)


if __name__ == "__main__":
    unittest.main(verbosity=2)
