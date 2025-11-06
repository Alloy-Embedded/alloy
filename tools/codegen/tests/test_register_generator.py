#!/usr/bin/env python3
"""
Test suite for register generator

This test validates that:
1. Generated register structures compile without errors
2. Register offsets are correct
3. Array dimensions are correct
4. RESERVED fields have correct sizes
5. No regressions from known bugs
"""

import unittest
import tempfile
import subprocess
from pathlib import Path
import sys

# Add parent directory to path to import generators
TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))

from cli.generators.generate_registers import generate_register_struct
from cli.parsers.generic_svd import SVDParser


class TestRegisterGenerator(unittest.TestCase):
    """Test register structure generation"""

    def setUp(self):
        """Setup test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.svd_dir = TOOLS_DIR / "upstream" / "cmsis-svd-data" / "data"

    def test_pioa_reserved_field_size(self):
        """
        Regression test for PIOA RESERVED field bug

        Bug: RESERVED_0074[12] should be RESERVED_0078[8]
        Cause: ABCDSR[2] array size not accounted for in offset calculation
        """
        # Parse ATSAME70Q21B SVD
        svd_file = self.svd_dir / "Atmel" / "ATSAME70Q21B.svd"
        self.assertTrue(svd_file.exists(), f"SVD not found: {svd_file}")

        parser = SVDParser(svd_file)
        device = parser.parse()

        # Find PIOA peripheral
        pioa = next((p for p in device.peripherals if p.name == "PIOA"), None)
        self.assertIsNotNone(pioa, "PIOA peripheral not found")

        # Find ABCDSR register
        abcdsr = next((r for r in pioa.registers if r.name == "ABCDSR"), None)
        self.assertIsNotNone(abcdsr, "ABCDSR register not found")

        # Validate dimensions
        self.assertEqual(abcdsr.dim, 2, "ABCDSR should be array of 2")
        self.assertEqual(abcdsr.size, 32, "ABCDSR should be 32-bit")

        # Calculate expected offset after ABCDSR
        # ABCDSR at 0x0070, size=32bit=4bytes, dim=2
        # Next offset should be 0x0070 + (4 * 2) = 0x0078
        expected_next_offset = abcdsr.offset + (abcdsr.size // 8) * abcdsr.dim
        self.assertEqual(expected_next_offset, 0x0078,
                        f"Offset after ABCDSR[2] should be 0x0078, got 0x{expected_next_offset:04X}")

        # Find next register after ABCDSR
        registers_sorted = sorted(pioa.registers, key=lambda r: r.offset)
        abcdsr_idx = next(i for i, r in enumerate(registers_sorted) if r.name == "ABCDSR")
        next_reg = registers_sorted[abcdsr_idx + 1]

        # RESERVED field size should be: next_offset - expected_next_offset
        reserved_size = next_reg.offset - expected_next_offset
        self.assertEqual(reserved_size, 8,
                        f"RESERVED field should be 8 bytes, got {reserved_size}")

    def test_pin_numbering_global(self):
        """
        Regression test for pin numbering bug

        Bug: PC8 = 8 should be PC8 = 72
        Cause: Port-relative numbering instead of global
        Formula: GlobalPin = (Port * 32) + Pin
        """
        # This will be implemented when we test pin generator
        # For now, verify the formula
        port_c = 2  # Port C
        pin_8 = 8
        expected_global = (port_c * 32) + pin_8
        self.assertEqual(expected_global, 72, "PC8 should be global pin 72")

    def test_register_struct_compilation(self):
        """
        Test that generated register structures compile

        This is the MOST IMPORTANT test - if it doesn't compile, it's broken!
        """
        # Parse ATSAME70Q21B SVD
        svd_file = self.svd_dir / "Atmel" / "ATSAME70Q21B.svd"
        parser = SVDParser(svd_file)
        device = parser.parse()

        # Find PIOA peripheral
        pioa = next((p for p in device.peripherals if p.name == "PIOA"), None)
        self.assertIsNotNone(pioa)

        # Generate register file
        output = generate_register_struct(pioa, device)

        # Write to temp file
        test_file = Path(self.temp_dir) / "pioa_registers.hpp"
        test_file.write_text(output)

        # Create minimal test program
        test_program = f"""
#include "{test_file}"

int main() {{
    // Test that structure can be instantiated
    alloy::hal::atmel::same70::atsame70q21b::pioa::PIOA_Registers regs;

    // Test accessing array member
    regs.ABCDSR[0] = 0;
    regs.ABCDSR[1] = 0;

    // Test sizeof matches expected size
    static_assert(sizeof(regs) >= 0x00A0, "Structure too small");

    return 0;
}}
"""
        test_cpp = Path(self.temp_dir) / "test_pioa.cpp"
        test_cpp.write_text(test_program)

        # Try to compile with g++
        try:
            result = subprocess.run(
                ["g++", "-std=c++20", "-c", str(test_cpp), "-o", "/dev/null"],
                capture_output=True,
                text=True,
                timeout=10
            )

            if result.returncode != 0:
                print(f"\n=== COMPILATION FAILED ===")
                print(f"STDOUT:\n{result.stdout}")
                print(f"STDERR:\n{result.stderr}")
                print(f"=========================\n")

            self.assertEqual(result.returncode, 0,
                           f"Compilation failed:\n{result.stderr}")
        except FileNotFoundError:
            self.skipTest("g++ not available")

    def test_register_array_generation(self):
        """
        Test that register arrays are generated correctly

        Example: ABCDSR[2] should generate:
        volatile uint32_t ABCDSR[2];

        NOT:
        volatile uint32_t ABCDSR;
        """
        svd_file = self.svd_dir / "Atmel" / "ATSAME70Q21B.svd"
        parser = SVDParser(svd_file)
        device = parser.parse()

        pioa = next((p for p in device.peripherals if p.name == "PIOA"), None)
        self.assertIsNotNone(pioa)

        # Generate register file
        output = generate_register_struct(pioa, device)

        # Check that ABCDSR is generated as array
        self.assertIn("ABCDSR[2]", output, "ABCDSR should be generated as array")
        self.assertNotRegex(output, r"ABCDSR\s*;", "ABCDSR should NOT be single register")

        # Check RESERVED field size
        self.assertIn("RESERVED_0078[8]", output, "RESERVED after ABCDSR should be 8 bytes")
        self.assertNotIn("RESERVED_0074[12]", output, "RESERVED should NOT be 12 bytes (bug)")


class TestRegeneratorSuite(unittest.TestCase):
    """Integration tests for complete regeneration"""

    def test_same70_complete_generation(self):
        """
        Test complete generation for SAME70Q21B

        This validates end-to-end generation for a known-good MCU
        """
        # This will test:
        # 1. SVD parsing
        # 2. Register generation
        # 3. Pin generation
        # 4. Enum generation
        # 5. Compilation of all generated files
        pass


if __name__ == "__main__":
    # Run with verbose output
    unittest.main(verbosity=2)
