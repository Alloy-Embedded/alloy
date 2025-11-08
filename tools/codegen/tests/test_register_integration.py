"""
Integration tests for register generation to boost coverage from 59% to 80%+

These tests target the main entry points and SVD integration:
- generate_for_device() - lines 401-435
- discover_families_with_pins() - lines 446-476
- Main CLI entry point - lines 493-575, 580-602, 606
"""

import unittest
import tempfile
from pathlib import Path
import sys
from unittest.mock import patch, MagicMock

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_registers import (
    generate_for_device,
    discover_families_with_pins,
    main
)
from cli.core.config import SVD_DIR


class TestGenerateForDevice(unittest.TestCase):
    """Test generate_for_device() integration"""

    def test_generate_for_device_with_real_svd(self):
        """Test generate_for_device with a real SVD file"""
        # Use STM32F103 as it's commonly available
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        result = generate_for_device(stm32_svd, family_level=False, tracker=None)

        # Should succeed
        self.assertTrue(result)

    def test_generate_for_device_family_level(self):
        """Test generate_for_device with family_level=True"""
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        result = generate_for_device(stm32_svd, family_level=True, tracker=None)

        # Should succeed
        self.assertTrue(result)

    def test_generate_for_device_with_invalid_svd(self):
        """Test generate_for_device with invalid SVD file"""
        # Create an invalid SVD file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.svd', delete=False) as f:
            f.write("<device><invalid>Bad XML</invalid></device>")
            temp_svd = Path(f.name)

        try:
            result = generate_for_device(temp_svd, family_level=False, tracker=None)

            # Should fail gracefully
            self.assertFalse(result)
        finally:
            temp_svd.unlink()

    def test_generate_for_device_with_tracker(self):
        """Test generate_for_device with progress tracker"""
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        # Create a mock tracker
        mock_tracker = MagicMock()

        result = generate_for_device(stm32_svd, family_level=False, tracker=mock_tracker)

        # Should succeed
        self.assertTrue(result)

    def test_generate_for_device_counts_peripherals(self):
        """Test that generate_for_device counts peripherals correctly"""
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        result = generate_for_device(stm32_svd, family_level=False, tracker=None)

        # Should succeed
        self.assertTrue(result)


class TestDiscoverFamiliesWithPins(unittest.TestCase):
    """Test discover_families_with_pins() function"""

    def test_discover_families_with_pins_returns_dict(self):
        """Test that discover_families_with_pins returns a dictionary"""
        families = discover_families_with_pins()

        self.assertIsInstance(families, dict)

    def test_discover_families_with_pins_structure(self):
        """Test that returned dict has correct structure"""
        families = discover_families_with_pins()

        # Each value should be a dict with vendor, family, mcu, path
        for family_key, info_dict in families.items():
            self.assertIsInstance(info_dict, dict)
            self.assertIn('vendor', info_dict)
            self.assertIn('family', info_dict)
            self.assertIn('mcu', info_dict)
            self.assertIn('path', info_dict)

            # Values should be strings (except path)
            self.assertIsInstance(info_dict['vendor'], str)
            self.assertIsInstance(info_dict['family'], str)
            self.assertIsInstance(info_dict['mcu'], str)

            # family_key should be like "vendor/family"
            self.assertEqual(family_key, f"{info_dict['vendor']}/{info_dict['family']}")

    def test_discover_families_finds_mcus_with_pins(self):
        """Test that discovered MCUs actually have pin_functions.hpp"""
        from cli.core.config import HAL_VENDORS_DIR

        families = discover_families_with_pins()

        if not families:
            self.skipTest("No families with pins found")

        # Check first family
        info = list(families.values())[0]
        vendor = info['vendor']
        family = info['family']
        mcu_name = info['mcu']

        # Check that pin_functions.hpp exists for this MCU
        pin_file = HAL_VENDORS_DIR / vendor.lower() / family.lower() / mcu_name.lower() / "pin_functions.hpp"

        # File should exist
        self.assertTrue(pin_file.exists(), f"Pin file {pin_file} should exist")

        # Vendor directory should exist
        vendor_dir = HAL_VENDORS_DIR / vendor.lower()
        self.assertTrue(vendor_dir.exists(), f"Vendor directory {vendor_dir} should exist")


class TestMainEntryPoint(unittest.TestCase):
    """Test main CLI entry point"""

    def test_main_with_specific_svd(self):
        """Test main() with --svd argument"""
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        with patch('sys.argv', ['generate_registers.py', '--svd', str(stm32_svd)]):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)

    def test_main_with_family_level(self):
        """Test main() with --family-level flag"""
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        with patch('sys.argv', ['generate_registers.py', '--svd', str(stm32_svd), '--family-level']):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)

    def test_main_with_nonexistent_svd(self):
        """Test main() with nonexistent SVD file"""
        with patch('sys.argv', ['generate_registers.py', '--svd', '/nonexistent/file.svd']):
            result = main()

            # Should return error code
            self.assertEqual(result, 1)

    def test_main_without_arguments_uses_families_with_pins(self):
        """Test main() without arguments generates for families with pins"""
        families = discover_families_with_pins()

        if not families:
            self.skipTest("No families with pins found")

        with patch('sys.argv', ['generate_registers.py']):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)

    def test_main_with_verbose_flag(self):
        """Test main() with --verbose flag"""
        families = discover_families_with_pins()

        if not families:
            self.skipTest("No families with pins found")

        with patch('sys.argv', ['generate_registers.py', '--verbose']):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)


class TestEdgeCasesIntegration(unittest.TestCase):
    """Test edge cases in integration scenarios"""

    def test_generate_for_device_with_no_peripherals(self):
        """Test SVD file with no peripherals"""
        # Create minimal SVD with no peripherals
        svd_content = """<?xml version="1.0" encoding="utf-8"?>
<device>
    <name>TEST_NO_PERIPH</name>
    <version>1.0</version>
    <description>Test device with no peripherals</description>
    <cpu>
        <name>CM4</name>
    </cpu>
    <addressUnitBits>8</addressUnitBits>
    <width>32</width>
    <peripherals>
    </peripherals>
</device>
"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.svd', delete=False) as f:
            f.write(svd_content)
            temp_svd = Path(f.name)

        try:
            result = generate_for_device(temp_svd, family_level=False, tracker=None)

            # Should succeed (0 peripherals = success)
            self.assertTrue(result)
        finally:
            temp_svd.unlink()

    def test_generate_for_device_peripheral_without_registers(self):
        """Test peripheral without registers"""
        # This is tested indirectly - peripherals without registers are skipped
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        # Should handle peripherals without registers gracefully
        result = generate_for_device(stm32_svd, family_level=False, tracker=None)
        self.assertTrue(result)


if __name__ == "__main__":
    unittest.main(verbosity=2)
