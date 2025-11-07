"""
Unit tests for register generation

These tests validate that the register generator produces correct C++ code
for various register configurations.
"""

import unittest
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_registers import generate_register_struct
from tests.test_helpers import (
    create_test_register,
    create_test_peripheral,
    create_test_device,
    create_pioa_test_peripheral,
    create_same70_test_device,
    AssertHelpers
)


class TestRegisterStructGeneration(unittest.TestCase):
    """Test generate_register_struct function"""

    def test_simple_register_generation(self):
        """Test generation of simple single register"""
        # Create minimal test data
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000, description="Test Register 1"),
            ]
        )
        device = create_test_device()

        # Generate code
        output = generate_register_struct(peripheral, device)

        # Validate output
        AssertHelpers.assert_contains_all(
            output,
            "struct TEST_Registers",
            "volatile uint32_t REG1",
            "Test Register 1",
            "Offset: 0x0000"
        )

    def test_register_array_generation(self):
        """Test that register arrays are generated correctly"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("ARRAY_REG", 0x0010, size=32, dim=4),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should generate array syntax
        AssertHelpers.assert_contains_all(
            output,
            "volatile uint32_t ARRAY_REG[4]"
        )

    def test_reserved_field_generation(self):
        """Test that RESERVED fields are generated for gaps"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000),
                create_test_register("REG2", 0x0010),  # Gap of 12 bytes
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should have RESERVED field for gap
        AssertHelpers.assert_contains_all(
            output,
            "RESERVED_0004[12]"  # Gap from 0x0004 to 0x0010
        )

    def test_register_with_different_sizes(self):
        """Test registers of different bit sizes"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG8", 0x0000, size=8),
                create_test_register("REG16", 0x0001, size=16),
                create_test_register("REG32", 0x0004, size=32),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "volatile uint8_t REG8",
            "volatile uint16_t REG16",
            "volatile uint32_t REG32"
        )


class TestPIOARegression(unittest.TestCase):
    """
    Regression tests for PIOA register generation bug

    Bug: RESERVED_0074[12] should be RESERVED_0078[8]
    Cause: ABCDSR[2] array dimension not accounted for
    """

    def test_pioa_reserved_field_size(self):
        """Test that PIOA RESERVED field has correct size"""
        peripheral = create_pioa_test_peripheral()
        device = create_same70_test_device()

        output = generate_register_struct(peripheral, device)

        # Should have correct RESERVED field
        AssertHelpers.assert_contains_all(
            output,
            "RESERVED_0078[8]"  # Correct size
        )

        # Should NOT have the bug
        AssertHelpers.assert_not_contains_any(
            output,
            "RESERVED_0074[12]"  # Old bug
        )

    def test_pioa_abcdsr_is_array(self):
        """Test that ABCDSR is generated as array"""
        peripheral = create_pioa_test_peripheral()
        device = create_same70_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "volatile uint32_t ABCDSR[2]"
        )

    def test_pioa_ower_offset(self):
        """Test that OWER register has correct offset comment"""
        peripheral = create_pioa_test_peripheral()
        device = create_same70_test_device()

        output = generate_register_struct(peripheral, device)

        # OWER should be at correct offset
        AssertHelpers.assert_contains_all(
            output,
            "Offset: 0x00A0",  # OWER offset
            "volatile uint32_t OWER"
        )


class TestRegisterArrays(unittest.TestCase):
    """Test various register array configurations"""

    def test_array_offset_calculation(self):
        """
        Test that offset after array is calculated correctly

        Formula: next_offset = current_offset + (size_bytes * dim)
        """
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                # Array of 3 x 32-bit registers at 0x0000
                create_test_register("ARRAY", 0x0000, size=32, dim=3),
                # Next register should be at 0x000C (0x0000 + 4*3)
                create_test_register("NEXT", 0x0010),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should have RESERVED from 0x000C to 0x0010 (4 bytes)
        AssertHelpers.assert_contains_all(
            output,
            "RESERVED_000C[4]"
        )

    def test_no_reserved_after_array_if_no_gap(self):
        """Test that no RESERVED is generated if array ends exactly at next register"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("ARRAY", 0x0000, size=32, dim=2),
                create_test_register("NEXT", 0x0008),  # Exactly after array
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should NOT have RESERVED between ARRAY and NEXT
        AssertHelpers.assert_not_contains_any(
            output,
            "RESERVED_0008"
        )


class TestNamespaceGeneration(unittest.TestCase):
    """Test namespace generation"""

    def test_namespace_from_device_info(self):
        """Test that namespaces are generated from device info (family-level)"""
        peripheral = create_test_peripheral("TEST", 0x40000000)
        device = create_test_device(
            name="ATSAME70Q21B",
            vendor="Atmel",
            family="SAME70"
        )

        output = generate_register_struct(peripheral, device)

        # Now generates family-level namespace (no MCU name)
        AssertHelpers.assert_contains_all(
            output,
            "namespace alloy::hal::atmel::same70::test"
        )

    def test_namespace_normalization(self):
        """Test that namespace names are normalized (lowercase, underscores)"""
        peripheral = create_test_peripheral("My_Test-Peripheral", 0x40000000)
        device = create_test_device(
            name="Test-MCU_123",
            vendor="Test Vendor",
            family="Test Family"
        )

        output = generate_register_struct(peripheral, device)

        # Should normalize names to valid C++ identifiers
        self.assertIn("namespace alloy::hal", output)


class TestStaticAssertions(unittest.TestCase):
    """Test that static assertions are generated"""

    def test_size_assertion_generated(self):
        """Test that sizeof static_assert is generated"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000),
                create_test_register("REG2", 0x0004),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "static_assert(sizeof(TEST_Registers)",
            "TEST_Registers size mismatch"
        )


class TestAccessorFunction(unittest.TestCase):
    """Test peripheral accessor function generation"""

    def test_accessor_function_generated(self):
        """Test that inline accessor function is generated"""
        peripheral = create_test_peripheral(
            name="PIOA",
            base_address=0x400E0E00,
            registers=[create_test_register("REG", 0x0000)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "inline PIOA_Registers* PIOA()",
            "return reinterpret_cast<PIOA_Registers*>(0x400E0E00)"
        )


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_empty_peripheral(self):
        """Test peripheral with no registers"""
        peripheral = create_test_peripheral("EMPTY", 0x40000000, registers=[])
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should still generate valid code with comment
        AssertHelpers.assert_contains_all(
            output,
            "namespace alloy::hal",
            "No registers defined"
        )

    def test_single_register(self):
        """Test peripheral with exactly one register"""
        peripheral = create_test_peripheral(
            name="SINGLE",
            base_address=0x40000000,
            registers=[create_test_register("REG", 0x0000)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "struct SINGLE_Registers",
            "volatile uint32_t REG"
        )

    def test_large_offset(self):
        """Test register with large offset"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000),
                create_test_register("REG2", 0x1000),  # Large gap
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should have large RESERVED field
        AssertHelpers.assert_contains_all(
            output,
            "RESERVED_0004[4092]"  # 0x1000 - 0x0004 = 4092 bytes
        )

    def test_contiguous_registers(self):
        """Test registers with no gaps"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000),
                create_test_register("REG2", 0x0004),
                create_test_register("REG3", 0x0008),
                create_test_register("REG4", 0x000C),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should have no RESERVED fields
        AssertHelpers.assert_not_contains_any(
            output,
            "RESERVED"
        )

    def test_register_at_offset_zero(self):
        """Test that first register at offset 0 works"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[create_test_register("REG0", 0x0000)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "Offset: 0x0000",
            "volatile uint32_t REG0"
        )

    def test_very_long_peripheral_name(self):
        """Test peripheral with very long name"""
        peripheral = create_test_peripheral(
            name="VERY_LONG_PERIPHERAL_NAME_THAT_IS_QUITE_EXCESSIVE",
            base_address=0x40000000,
            registers=[create_test_register("REG", 0x0000)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should handle long names
        self.assertIn("VERY_LONG_PERIPHERAL_NAME_THAT_IS_QUITE_EXCESSIVE", output)

    def test_register_with_no_description(self):
        """Test register without description"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[create_test_register("REG", 0x0000, description=None)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Should still generate code
        AssertHelpers.assert_contains_all(
            output,
            "volatile uint32_t REG"
        )

    def test_multiple_large_arrays(self):
        """Test multiple register arrays"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("ARRAY1", 0x0000, dim=8),
                create_test_register("ARRAY2", 0x0020, dim=4),
                create_test_register("ARRAY3", 0x0030, dim=16),
            ]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        AssertHelpers.assert_contains_all(
            output,
            "volatile uint32_t ARRAY1[8]",
            "volatile uint32_t ARRAY2[4]",
            "volatile uint32_t ARRAY3[16]"
        )

    def test_hex_base_address(self):
        """Test that base address is formatted as hex"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0xDEADBEEF,
            registers=[create_test_register("REG", 0x0000)]
        )
        device = create_test_device()

        output = generate_register_struct(peripheral, device)

        # Base address should be in hex format (lowercase)
        self.assertIn("0xdeadbeef", output.lower())


if __name__ == "__main__":
    unittest.main(verbosity=2)
