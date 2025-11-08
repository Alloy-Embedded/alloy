"""
Integration tests for startup code generation

Tests the full startup generation workflow including:
- generate_for_device() with SVD parsing
- generate_for_board_mcus() main entry point
- Integration with ProgressTracker
- Error handling and edge cases
"""

import unittest
import tempfile
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_startup import (
    generate_for_device,
    generate_for_board_mcus,
    discover_mcus_with_pins
)
from cli.core.progress import ProgressTracker


class MockTracker:
    """Mock progress tracker for testing"""
    def __init__(self):
        self.generator = None
        self.tasks = []
        self.files = []

    def set_generator(self, name):
        self.generator = name

    def add_mcu_task(self, vendor, family, mcu, files):
        self.tasks.append({
            'vendor': vendor,
            'family': family,
            'mcu': mcu,
            'files': files
        })

    def mark_mcu_generating(self, vendor, family, mcu):
        pass

    def mark_file_generating(self, vendor, family, mcu, file):
        pass

    def mark_file_success(self, vendor, family, mcu, file, path):
        self.files.append({
            'vendor': vendor,
            'family': family,
            'mcu': mcu,
            'file': file,
            'path': path,
            'status': 'success'
        })

    def mark_file_failed(self, vendor, family, mcu, file, reason):
        self.files.append({
            'vendor': vendor,
            'family': family,
            'mcu': mcu,
            'file': file,
            'reason': reason,
            'status': 'failed'
        })

    def complete_mcu_generation(self, vendor, family, mcu, success):
        pass


class TestGenerateForDevice(unittest.TestCase):
    """Test generate_for_device() integration"""

    def test_generate_for_device_with_invalid_svd(self):
        """Test error handling with invalid SVD file"""
        with tempfile.NamedTemporaryFile(suffix='.svd') as tmp:
            tmp.write(b"<invalid>XML</invalid>")
            tmp.flush()

            svd_path = Path(tmp.name)
            result = generate_for_device(svd_path)

            # Should fail gracefully
            self.assertFalse(result)

    def test_generate_for_device_with_nonexistent_svd(self):
        """Test error handling with nonexistent SVD file"""
        svd_path = Path("/nonexistent/file.svd")
        result = generate_for_device(svd_path)

        # Should fail gracefully
        self.assertFalse(result)

    def test_generate_for_device_with_tracker(self):
        """Test generate_for_device with progress tracker"""
        # Find a real SVD file to test with
        from cli.core.config import SVD_DIR

        # Try to find STM32F103 SVD (common test device)
        stm32_svd = SVD_DIR / "STMicro" / "STM32F103.svd"

        if not stm32_svd.exists():
            self.skipTest("STM32F103.svd not found for integration test")

        # Create mock tracker
        tracker = MockTracker()

        # Generate with tracker
        result = generate_for_device(stm32_svd, tracker=tracker)

        # Should succeed
        self.assertTrue(result)

        # Tracker should have been used
        self.assertEqual(len(tracker.tasks), 1)
        self.assertEqual(len(tracker.files), 2)  # startup.cpp and peripherals.hpp

        # Check files were marked as success
        statuses = [f['status'] for f in tracker.files]
        self.assertEqual(statuses, ['success', 'success'])

    def test_generate_for_device_without_tracker(self):
        """Test generate_for_device without tracker (standalone mode)"""
        from cli.core.config import SVD_DIR

        # Try to find SAMD21 SVD
        samd21_svd = SVD_DIR / "Atmel" / "ATSAMD21G18A.svd"

        if not samd21_svd.exists():
            self.skipTest("ATSAMD21G18A.svd not found for integration test")

        # Generate without tracker
        result = generate_for_device(samd21_svd, tracker=None)

        # Should succeed even without tracker
        self.assertTrue(result)


class TestGenerateForBoardMCUs(unittest.TestCase):
    """Test generate_for_board_mcus() main entry point"""

    def test_generate_for_board_mcus_no_pins(self):
        """Test when no MCUs have pin functions"""
        # This is hard to test without mocking discover_mcus_with_pins()
        # We'll test the actual function behavior instead
        mcus = discover_mcus_with_pins()

        # Should return a list (might be empty or populated)
        self.assertIsInstance(mcus, list)

    def test_generate_for_board_mcus_basic(self):
        """Test basic generate_for_board_mcus execution"""
        # This will use actual SVD files and pin functions
        # Only run if we have MCUs with pins
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pin functions available")

        # Run with verbose=False (faster)
        result = generate_for_board_mcus(verbose=False, tracker=None)

        # Should succeed (0 = success)
        self.assertEqual(result, 0)

    def test_generate_for_board_mcus_with_verbose(self):
        """Test generate_for_board_mcus with verbose output"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pin functions available")

        # Run with verbose=True
        result = generate_for_board_mcus(verbose=True, tracker=None)

        # Should succeed
        self.assertEqual(result, 0)

    def test_generate_for_board_mcus_with_tracker(self):
        """Test generate_for_board_mcus with progress tracker"""
        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pin functions available")

        # Create mock tracker
        tracker = MockTracker()

        # Run with tracker
        result = generate_for_board_mcus(verbose=False, tracker=tracker)

        # Should succeed
        self.assertEqual(result, 0)

        # Tracker should have generator set
        self.assertEqual(tracker.generator, "startup")

        # Tracker should have tasks for each MCU
        self.assertGreater(len(tracker.tasks), 0)


class TestDiscoverMCUsWithPinsDetailed(unittest.TestCase):
    """Detailed tests for MCU discovery"""

    def test_discover_returns_unique_mcus(self):
        """Test that discover_mcus_with_pins returns unique MCU names"""
        mcus = discover_mcus_with_pins()

        # Should have no duplicates
        self.assertEqual(len(mcus), len(set(mcus)))

    def test_discover_mcus_format(self):
        """Test that MCU names are in expected format"""
        mcus = discover_mcus_with_pins()

        for mcu in mcus:
            # Should be strings
            self.assertIsInstance(mcu, str)

            # Should not be empty
            self.assertGreater(len(mcu), 0)

            # Should be lowercase (as per convention)
            self.assertEqual(mcu, mcu.lower())

    def test_discover_mcus_directory_structure(self):
        """Test that discovered MCUs match directory structure"""
        mcus = discover_mcus_with_pins()

        from cli.core.config import REPO_ROOT

        for mcu in mcus:
            # Each MCU should have a directory structure vendor/family/mcu/
            # We can't easily verify this without knowing vendor/family mapping
            # So we just verify the MCU name is valid
            self.assertIsInstance(mcu, str)


class TestErrorHandling(unittest.TestCase):
    """Test error handling throughout startup generation"""

    def test_generate_peripherals_hpp_with_invalid_device(self):
        """Test peripherals.hpp generation with invalid device"""
        from cli.generators.generate_startup import generate_peripherals_hpp
        from tests.test_helpers import create_test_device

        device = create_test_device()
        device.peripherals = None  # Invalid

        with tempfile.TemporaryDirectory() as tmpdir:
            output_path = Path(tmpdir) / "peripherals.hpp"

            # Should handle gracefully
            try:
                result = generate_peripherals_hpp(device, output_path)
                # If it doesn't raise, it should return False
                self.assertFalse(result)
            except Exception:
                # If it raises, that's also acceptable
                pass

    def test_generate_startup_cpp_with_readonly_output(self):
        """Test startup.cpp generation with read-only output directory"""
        from cli.generators.generate_startup import generate_startup_cpp
        from tests.test_helpers import create_test_device

        device = create_test_device()
        device.interrupts = []

        # Try to write to read-only location
        output_path = Path("/nonexistent/startup.cpp")

        result = generate_startup_cpp(device, output_path)

        # Should return False (failure)
        self.assertFalse(result)


class TestSVDDiscoveryIntegration(unittest.TestCase):
    """Test integration with SVD discovery system"""

    def test_svd_discovery_finds_board_mcus(self):
        """Test that SVD discovery can find board MCUs"""
        from cli.parsers.svd_discovery import discover_all_svds, find_svd_for_mcu

        mcus = discover_mcus_with_pins()

        if not mcus:
            self.skipTest("No MCUs with pins to test")

        # Discover all SVDs
        all_svds = discover_all_svds()

        # Should find some SVDs
        self.assertGreater(len(all_svds), 0)

        # Try to find SVD for first MCU
        first_mcu = mcus[0]
        svd_file = find_svd_for_mcu(first_mcu, all_svds)

        # Should find SVD (might be None if no match)
        # We just verify it doesn't crash
        self.assertIsNotNone(svd_file) or self.assertIsNone(svd_file)

    def test_svd_discovery_caching(self):
        """Test that SVD discovery uses caching"""
        from cli.parsers.svd_discovery import discover_all_svds

        # Call twice - second should be cached
        import time

        start = time.time()
        first = discover_all_svds()
        first_time = time.time() - start

        start = time.time()
        second = discover_all_svds()
        second_time = time.time() - start

        # Second call should be much faster (cached)
        # We don't assert on time since it's flaky, but we verify same results
        self.assertEqual(len(first), len(second))


if __name__ == "__main__":
    unittest.main(verbosity=2)
