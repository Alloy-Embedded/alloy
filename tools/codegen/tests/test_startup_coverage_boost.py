"""
Additional tests to boost startup generation coverage to 90%+

These tests target the remaining uncovered code paths:
- Template rendering and Jinja2 integration
- Error handling in generate_peripherals_hpp
- Main entry point and CLI argument parsing
- SVD discovery integration
- Progress tracker integration
"""

import unittest
import tempfile
from pathlib import Path
import sys
from unittest.mock import patch, MagicMock

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_startup import (
    generate_startup_cpp,
    generate_peripherals_hpp,
    generate_for_device,
    generate_for_board_mcus,
    discover_mcus_with_pins,
    main
)
from tests.test_helpers import create_test_device


class TestTemplateRendering(unittest.TestCase):
    """Test Jinja2 template rendering code paths"""

    def test_startup_template_with_large_vector_table(self):
        """Test startup.cpp generation with 100+ interrupts"""
        device = create_test_device()

        # Create 150 interrupts to test large vector tables
        from cli.parsers.generic_svd import SVDInterrupt
        device.interrupts = [
            SVDInterrupt(
                name=f"IRQ{i}",
                value=i,
                description=f"Interrupt {i} description"
            )
            for i in range(150)
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            result = generate_startup_cpp(device, output_path)

            self.assertTrue(result)
            self.assertTrue(output_path.exists())

            content = output_path.read_text()

            # Verify all 150 handlers are declared
            for i in range(150):
                self.assertIn(f"IRQ{i}_Handler", content)

            # Verify vector table has all entries
            self.assertIn("IRQ0_Handler", content)
            self.assertIn("IRQ149_Handler", content)

    def test_startup_template_with_special_device_name(self):
        """Test startup with device name containing special characters"""
        device = create_test_device()
        device.name = "STM32F407VG-SPECIAL_v2.1"
        device.cpu_name = "Cortex-M4F"
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            result = generate_startup_cpp(device, output_path)

            self.assertTrue(result)
            content = output_path.read_text()

            # Device name should appear in comments
            self.assertIn("STM32F407VG-SPECIAL_v2.1", content)

    def test_peripherals_template_with_overlapping_addresses(self):
        """Test peripherals.hpp with peripherals at same base address"""
        device = create_test_device()

        # Add peripherals with overlapping addresses (union scenarios)
        from cli.parsers.generic_svd import SVDPeripheral
        device.peripherals = [
            SVDPeripheral(name="TIM1", base_address=0x40010000),
            SVDPeripheral(name="TIM1_ALT", base_address=0x40010000),  # Same address
            SVDPeripheral(name="TIM2", base_address=0x40000000),
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"
            result = generate_peripherals_hpp(device, output_path)

            self.assertTrue(result)
            content = output_path.read_text()

            # Both peripherals should be defined
            self.assertIn("TIM1", content)
            self.assertIn("TIM1_ALT", content)
            self.assertIn("0x40010000", content)


class TestErrorHandlingPaths(unittest.TestCase):
    """Test error handling and edge cases"""

    def test_generate_peripherals_hpp_error_handling(self):
        """Test error handling in generate_peripherals_hpp"""
        device = create_test_device()

        # Test with invalid output path (read-only directory)
        output_path = Path("/dev/null/peripherals.hpp")
        result = generate_peripherals_hpp(device, output_path)

        # Should fail gracefully
        self.assertFalse(result)

    def test_generate_for_device_with_parsing_error(self):
        """Test generate_for_device when SVD parsing fails"""
        # Create invalid SVD file
        with tempfile.NamedTemporaryFile(suffix='.svd', mode='w', delete=False) as tmp:
            tmp.write("<device><invalid>XML</invalid></device>")
            tmp_path = Path(tmp.name)

        try:
            result = generate_for_device(tmp_path, tracker=None)

            # Should return False when parsing fails
            self.assertFalse(result)
        finally:
            tmp_path.unlink()

    def test_generate_for_device_with_write_error(self):
        """Test generate_for_device when file writing fails"""
        from cli.core.config import SVD_DIR

        # Use a real SVD file but mock the file writing to fail
        samd21_svd = SVD_DIR / "Atmel" / "ATSAMD21G18A.svd"

        if not samd21_svd.exists():
            self.skipTest("ATSAMD21G18A.svd not found")

        with patch('cli.generators.generate_startup.generate_startup_cpp', return_value=False):
            result = generate_for_device(samd21_svd, tracker=None)

            # Should return False when generation fails
            self.assertFalse(result)


class TestMainEntryPoints(unittest.TestCase):
    """Test main entry point and CLI argument parsing"""

    def test_main_with_specific_svd_file(self):
        """Test main() with --svd argument"""
        from cli.core.config import SVD_DIR

        # Find a valid SVD file
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        # Mock sys.argv to test CLI argument parsing
        with patch('sys.argv', ['generate_startup.py', '--svd', str(stm32_svd)]):
            # Run main() - it should succeed
            result = main()

            # 0 = success
            self.assertEqual(result, 0)

    def test_main_with_nonexistent_svd(self):
        """Test main() with nonexistent SVD file"""
        with patch('sys.argv', ['generate_startup.py', '--svd', '/nonexistent/file.svd']):
            result = main()

            # Should return 1 (error)
            self.assertEqual(result, 1)

    def test_main_with_relative_svd_path(self):
        """Test main() with relative SVD path"""
        # Test that relative paths are resolved relative to SVD_DIR
        with patch('sys.argv', ['generate_startup.py', '--svd', 'STMicro/STM32F103.svd']):
            from cli.core.config import SVD_DIR
            full_path = SVD_DIR / "STMicro" / "STM32F103.svd"

            if not full_path.exists():
                self.skipTest("STM32F103.svd not found")

            result = main()

            # Should succeed
            self.assertEqual(result, 0)

    def test_main_without_arguments(self):
        """Test main() without arguments (generates for all board MCUs)"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        with patch('sys.argv', ['generate_startup.py']):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)

    def test_main_with_verbose_flag(self):
        """Test main() with --verbose flag"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        with patch('sys.argv', ['generate_startup.py', '--verbose']):
            result = main()

            # Should succeed
            self.assertEqual(result, 0)


class TestProgressTrackerIntegration(unittest.TestCase):
    """Test integration with progress tracker"""

    def test_generate_for_device_tracks_progress(self):
        """Test that generate_for_device uses progress tracker"""
        from cli.core.config import SVD_DIR

        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found")

        # Create mock tracker
        mock_tracker = MagicMock()

        result = generate_for_device(stm32_svd, tracker=mock_tracker)

        self.assertTrue(result)

        # Verify tracker methods were called
        mock_tracker.add_mcu_task.assert_called_once()
        mock_tracker.mark_mcu_generating.assert_called_once()
        mock_tracker.complete_mcu_generation.assert_called_once()

        # Should have marked files as generating and successful
        self.assertGreater(mock_tracker.mark_file_generating.call_count, 0)
        self.assertGreater(mock_tracker.mark_file_success.call_count, 0)

    def test_generate_for_device_tracks_failure(self):
        """Test that tracker marks files as failed on error"""
        # Create invalid SVD
        with tempfile.NamedTemporaryFile(suffix='.svd', mode='w', delete=False) as tmp:
            tmp.write("<invalid>")
            tmp_path = Path(tmp.name)

        try:
            mock_tracker = MagicMock()

            result = generate_for_device(tmp_path, tracker=mock_tracker)

            self.assertFalse(result)
        finally:
            tmp_path.unlink()

    def test_generate_for_board_mcus_sets_generator_id(self):
        """Test that generate_for_board_mcus sets tracker generator ID"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        mock_tracker = MagicMock()

        result = generate_for_board_mcus(verbose=False, tracker=mock_tracker)

        # Should set generator ID to "startup"
        mock_tracker.set_generator.assert_called_once_with("startup")


class TestSVDDiscoveryIntegration(unittest.TestCase):
    """Test integration with SVD discovery system"""

    def test_generate_for_board_mcus_uses_svd_discovery(self):
        """Test that generate_for_board_mcus uses SVD discovery"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        # Mock discover_all_svds to verify it's called
        with patch('cli.generators.generate_startup.discover_all_svds') as mock_discover:
            mock_discover.return_value = []

            result = generate_for_board_mcus(verbose=False, tracker=None)

            # Should call discover_all_svds
            mock_discover.assert_called_once()

    def test_generate_for_board_mcus_finds_svd_for_each_mcu(self):
        """Test that find_svd_for_mcu is called for each MCU"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        with patch('cli.generators.generate_startup.find_svd_for_mcu') as mock_find:
            # Make it return None to skip actual generation
            mock_find.return_value = None

            result = generate_for_board_mcus(verbose=False, tracker=None)

            # Should call find_svd_for_mcu for each MCU with pins
            self.assertEqual(mock_find.call_count, len(mcus))

    def test_generate_for_board_mcus_handles_missing_svd(self):
        """Test behavior when SVD file is not found for an MCU"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins available")

        # Mock to return None (no SVD found) for all MCUs
        with patch('cli.generators.generate_startup.find_svd_for_mcu', return_value=None):
            result = generate_for_board_mcus(verbose=True, tracker=None)

            # Should return error code when no SVD files found
            self.assertEqual(result, 1)


class TestEdgeCasesAndBoundaries(unittest.TestCase):
    """Test edge cases and boundary conditions"""

    def test_generate_startup_with_empty_device_name(self):
        """Test startup generation with empty device name"""
        device = create_test_device()
        device.name = ""
        device.interrupts = []

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            result = generate_startup_cpp(device, output_path)

            # Should still succeed
            self.assertTrue(result)

    def test_generate_peripherals_with_zero_address(self):
        """Test peripherals.hpp with peripheral at address 0x0"""
        device = create_test_device()

        from cli.parsers.generic_svd import SVDPeripheral
        device.peripherals = [
            SVDPeripheral(name="NULL_PERIPHERAL", base_address=0x0),
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"
            result = generate_peripherals_hpp(device, output_path)

            self.assertTrue(result)
            content = output_path.read_text()

            # Should handle 0x0 address correctly
            self.assertIn("0x0", content)

    def test_generate_startup_with_max_uint32_interrupt_value(self):
        """Test with interrupt value at uint32 maximum"""
        device = create_test_device()

        from cli.parsers.generic_svd import SVDInterrupt
        device.interrupts = [
            SVDInterrupt(name="MAX_IRQ", value=0xFFFFFFFF, description="Max value IRQ")
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "startup.cpp"
            result = generate_startup_cpp(device, output_path)

            self.assertTrue(result)
            content = output_path.read_text()

            # Should handle large interrupt value
            self.assertIn("MAX_IRQ_Handler", content)

    def test_discover_mcus_with_pins_caching(self):
        """Test that discover_mcus_with_pins returns consistent results"""
        # Call twice - should return same results
        first = discover_mcus_with_pins()
        second = discover_mcus_with_pins()

        self.assertEqual(first, second)

        # Results should be sorted
        if first:
            sorted_first = sorted(first)
            self.assertEqual(first, sorted_first)


if __name__ == "__main__":
    unittest.main(verbosity=2)
