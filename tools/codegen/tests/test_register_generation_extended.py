"""
Extended tests for register generation to boost coverage from 27% to 80%+

Tests cover:
- Sanitization functions (sanitize_description, sanitize_identifier, sanitize_namespace_name)
- Bitfield header generation
- Register header generation with various edge cases
- File generation with actual I/O
- Integration with peripherals and devices
"""

import unittest
import tempfile
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_registers import (
    sanitize_description,
    sanitize_identifier,
    sanitize_namespace_name,
    generate_bitfield_definitions,
    generate_register_struct,
    generate_for_peripheral
)
from cli.parsers.generic_svd import (
    Peripheral,
    Register,
    RegisterField
)
from tests.test_helpers import create_test_device


def create_test_peripheral(name="TEST_PERIPH"):
    """Create a test peripheral with registers and fields"""
    peripheral = Peripheral(name=name, base_address=0x40000000, description="Test Peripheral")

    # Add a register with fields
    reg = Register(
        name="CTRL",
        offset=0x0,
        size=32,
        access="read-write",
        description="Control Register"
    )

    # Add fields
    reg.fields = [
        RegisterField(
            name="ENABLE",
            bit_offset=0,
            bit_width=1,
            access="read-write",
            description="Enable bit"
        ),
        RegisterField(
            name="MODE",
            bit_offset=4,
            bit_width=3,
            access="read-write",
            description="Mode selection",
            enum_values={"MODE_0": 0, "MODE_1": 1, "MODE_2": 2}
        )
    ]

    peripheral.registers = [reg]
    return peripheral


def create_stm32_test_device():
    """Create a test device with STM32F4 attributes for namespace matching"""
    return create_test_device(
        name="STM32F407VG",
        vendor="ST",
        family="STM32F4"
    )


class TestSanitizationFunctions(unittest.TestCase):
    """Test sanitization helper functions"""

    def test_sanitize_description_removes_newlines(self):
        """Test that newlines are replaced with spaces"""
        desc = "This is a\nmultiline\ndescription"
        result = sanitize_description(desc)

        self.assertNotIn('\n', result)
        self.assertEqual(result, "This is a multiline description")

    def test_sanitize_description_removes_multiple_spaces(self):
        """Test that multiple spaces are collapsed"""
        desc = "This  has    too     many      spaces"
        result = sanitize_description(desc)

        self.assertEqual(result, "This has too many spaces")

    def test_sanitize_description_removes_comment_markers(self):
        """Test that comment markers are removed"""
        desc = "Description with /* comment */ markers */"
        result = sanitize_description(desc)

        self.assertNotIn('/*', result)
        self.assertNotIn('*/', result)

    def test_sanitize_description_empty_string(self):
        """Test handling of empty description"""
        result = sanitize_description("")
        self.assertEqual(result, "")

    def test_sanitize_description_none(self):
        """Test handling of None description"""
        result = sanitize_description(None)
        self.assertEqual(result, "")

    def test_sanitize_identifier_digit_prefix(self):
        """Test that identifiers starting with digit get underscore prefix"""
        result = sanitize_identifier("1_BANK")
        self.assertEqual(result, "_1_BANK")

        result = sanitize_identifier("8_BYTE")
        self.assertEqual(result, "_8_BYTE")

    def test_sanitize_identifier_invalid_chars(self):
        """Test that invalid characters are replaced with underscore"""
        result = sanitize_identifier("TEST-NAME")
        self.assertEqual(result, "TEST_NAME")

        result = sanitize_identifier("TEST.NAME")
        self.assertEqual(result, "TEST_NAME")

        result = sanitize_identifier("TEST NAME")
        self.assertEqual(result, "TEST_NAME")

    def test_sanitize_identifier_empty(self):
        """Test handling of empty identifier"""
        result = sanitize_identifier("")
        self.assertEqual(result, "INVALID")

    def test_sanitize_identifier_normal(self):
        """Test that normal identifiers pass through"""
        result = sanitize_identifier("NORMAL_NAME")
        self.assertEqual(result, "NORMAL_NAME")

    def test_sanitize_namespace_name_removes_array_syntax(self):
        """Test that array syntax is removed from namespace names"""
        result = sanitize_namespace_name("ABCDSR[2]")
        self.assertNotIn('[', result)
        self.assertNotIn(']', result)
        self.assertEqual(result.lower(), "abcdsr")

    def test_sanitize_namespace_name_lowercase_conversion(self):
        """Test that namespace names are converted to lowercase"""
        result = sanitize_namespace_name("TEST_REGISTER")
        self.assertEqual(result, "test_register")

    def test_sanitize_namespace_name_reserved_keywords(self):
        """Test that C++ reserved keywords get underscore suffix"""
        result = sanitize_namespace_name("class")
        self.assertEqual(result, "class_")

        result = sanitize_namespace_name("return")
        self.assertEqual(result, "return_")

        result = sanitize_namespace_name("namespace")
        self.assertEqual(result, "namespace_")


class TestBitfieldHeaderGeneration(unittest.TestCase):
    """Test bitfield header generation"""

    def test_generate_bitfields_basic(self):
        """Test basic bitfield generation"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should contain pragma once
        self.assertIn("#pragma once", content)

        # Should contain bitfield include
        self.assertIn("hal/utils/bitfield.hpp", content)

        # Should contain namespace
        self.assertIn("namespace alloy::hal::st::stm32f4::stm32f407vg::test_periph", content)

        # Should contain BitField definitions
        self.assertIn("using ENABLE = BitField<0, 1>", content)
        self.assertIn("using MODE = BitField<4, 3>", content)

    def test_generate_bitfields_family_level(self):
        """Test bitfield generation at family level"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_bitfield_definitions(peripheral, device, family_level=True)

        # Should not contain MCU name in namespace
        self.assertNotIn("stm32f407vg", content)
        self.assertIn("namespace alloy::hal::st::stm32f4::test_periph", content)

    def test_generate_bitfields_with_enums(self):
        """Test that enumerated values are generated"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should contain enum namespace
        self.assertIn("namespace mode", content)

        # Should contain enum values
        self.assertIn("constexpr uint32_t MODE_0 = 0", content)
        self.assertIn("constexpr uint32_t MODE_1 = 1", content)
        self.assertIn("constexpr uint32_t MODE_2 = 2", content)

    def test_generate_bitfields_cmsis_compatible(self):
        """Test that CMSIS-compatible constants are generated"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should contain CMSIS _Pos and _Msk constants
        self.assertIn("ENABLE_Pos = 0", content)
        self.assertIn("ENABLE_Msk", content)
        self.assertIn("MODE_Pos = 4", content)
        self.assertIn("MODE_Msk", content)

    def test_generate_bitfields_multiple_registers(self):
        """Test bitfield generation with multiple registers"""
        peripheral = create_test_peripheral()

        # Add another register
        reg2 = Register(
            name="STATUS",
            offset=0x4,
            size=32,
            access="read-only",
            description="Status Register"
        )
        reg2.fields = [
            RegisterField(
                name="READY",
                bit_offset=0,
                bit_width=1,
                access="read-only",
                description="Ready flag"
            )
        ]
        peripheral.registers.append(reg2)

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should contain both register namespaces
        self.assertIn("namespace ctrl", content)
        self.assertIn("namespace status", content)

        # Should contain fields from both registers
        self.assertIn("using ENABLE", content)
        self.assertIn("using READY", content)

    def test_generate_bitfields_register_without_fields(self):
        """Test that registers without fields are skipped"""
        peripheral = create_test_peripheral()

        # Add register without fields
        reg_no_fields = Register(
            name="RESERVED",
            offset=0x8,
            size=32,
            access="read-only"
        )
        reg_no_fields.fields = []
        peripheral.registers.append(reg_no_fields)

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should not contain the register without fields
        self.assertNotIn("namespace reserved", content)


class TestRegisterHeaderGeneration(unittest.TestCase):
    """Test register structure header generation"""

    def test_generate_registers_basic(self):
        """Test basic register structure generation"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_register_struct(peripheral, device, family_level=False)

        # Should contain pragma once
        self.assertIn("#pragma once", content)

        # Should contain register struct (uses peripheral name, not individual register names)
        self.assertIn("struct TEST_PERIPH_Registers", content)

        # Should contain volatile uint32_t
        self.assertIn("volatile uint32_t", content)

    def test_generate_registers_descriptions(self):
        """Test that register descriptions are included"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        content = generate_register_struct(peripheral, device, family_level=False)

        # Should contain description in comments
        self.assertIn("Control Register", content)

    def test_generate_registers_sorted_by_offset(self):
        """Test that registers are sorted by offset"""
        peripheral = create_test_peripheral()

        # Add registers in non-sequential order
        reg2 = Register(name="REG2", offset=0x8, size=32)
        reg1 = Register(name="REG1", offset=0x4, size=32)

        peripheral.registers = [reg2, reg1, peripheral.registers[0]]  # 0x8, 0x4, 0x0

        device = create_stm32_test_device()
        content = generate_register_struct(peripheral, device, family_level=False)

        # Find positions in content
        ctrl_pos = content.find("CTRL")
        reg1_pos = content.find("REG1")
        reg2_pos = content.find("REG2")

        # Should be in order: CTRL (0x0), REG1 (0x4), REG2 (0x8)
        self.assertLess(ctrl_pos, reg1_pos)
        self.assertLess(reg1_pos, reg2_pos)


class TestFileGeneration(unittest.TestCase):
    """Test actual file generation"""

    def test_generate_for_peripheral_creates_files(self):
        """Test that generate_for_peripheral creates both files"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            result = generate_for_peripheral(
                peripheral, device, output_dir,
                family_level=False, tracker=None
            )

            self.assertTrue(result)

            # Check bitfields file exists
            bitfields_dir = output_dir / "bitfields"
            bitfields_file = bitfields_dir / "test_periph_bitfields.hpp"
            self.assertTrue(bitfields_file.exists())

            # Check registers file exists
            registers_dir = output_dir / "registers"
            registers_file = registers_dir / "test_periph_registers.hpp"
            self.assertTrue(registers_file.exists())

    def test_generate_for_peripheral_file_contents(self):
        """Test that generated files have correct content"""
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            generate_for_peripheral(
                peripheral, device, output_dir,
                family_level=False, tracker=None
            )

            # Read and verify bitfields file
            bitfields_file = output_dir / "bitfields" / "test_periph_bitfields.hpp"
            content = bitfields_file.read_text()

            self.assertIn("BitField", content)
            self.assertIn("ENABLE", content)
            self.assertIn("MODE", content)

    def test_generate_for_peripheral_error_handling(self):
        """Test error handling when file writing fails"""
        from unittest.mock import patch, MagicMock
        peripheral = create_test_peripheral()
        device = create_stm32_test_device()

        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir)

            # Mock Path.write_text to raise an exception
            with patch('pathlib.Path.write_text', side_effect=IOError("Mock write error")):
                result = generate_for_peripheral(
                    peripheral, device, output_dir,
                    family_level=False, tracker=None
                )

                # Should return False on error
                self.assertFalse(result)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_peripheral_with_array_register_names(self):
        """Test registers with array syntax in names"""
        peripheral = Peripheral(name="DMA", base_address=0x40020000)

        reg = Register(
            name="S0CR[4]",  # Array register
            offset=0x10,
            size=32
        )
        reg.fields = [
            RegisterField(
                name="EN",
                bit_offset=0,
                bit_width=1,
                description="Enable"
            )
        ]
        peripheral.registers = [reg]

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Namespace should have array syntax removed
        self.assertIn("namespace s0cr", content)
        # But description in comments may still contain array syntax
        self.assertIn("S0CR[4]", content)  # In comment
        # Make sure the namespace itself doesn't have brackets
        self.assertIn("namespace s0cr {", content)

    def test_field_with_digit_starting_name(self):
        """Test field names starting with digits"""
        peripheral = create_test_peripheral()

        # Add field with digit-starting name
        peripheral.registers[0].fields.append(
            RegisterField(
                name="8BIT_MODE",
                bit_offset=8,
                bit_width=1,
                description="8-bit mode enable"
            )
        )

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Field names are used as-is (sanitization happens at template/usage level if needed)
        # The generator outputs the field name directly from SVD
        self.assertIn("8BIT_MODE", content)
        self.assertIn("Position: 8, Width: 1", content)

    def test_description_with_special_characters(self):
        """Test descriptions with special characters"""
        peripheral = create_test_peripheral()
        peripheral.registers[0].description = "Test /* comment */ with\nnewlines\tand  spaces"

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should be sanitized - check the description in the comment
        # Note: content will have newlines for code structure, but the description
        # itself should have newlines replaced with spaces
        self.assertIn("Test  comment  with newlines and spaces", content)
        self.assertNotIn('/*', content)
        self.assertNotIn('*/', content)

    def test_register_with_zero_width_field(self):
        """Test handling of fields with zero width (should still generate)"""
        peripheral = create_test_peripheral()

        # Add zero-width field (edge case)
        peripheral.registers[0].fields.append(
            RegisterField(
                name="ZERO",
                bit_offset=16,
                bit_width=0,
                description="Zero-width field"
            )
        )

        device = create_stm32_test_device()
        content = generate_bitfield_definitions(peripheral, device, family_level=False)

        # Should still generate (though semantically weird)
        self.assertIn("using ZERO = BitField<16, 0>", content)

    def test_register_with_large_offset(self):
        """Test registers with large offsets"""
        peripheral = create_test_peripheral()

        # Add register with large offset
        reg = Register(
            name="LARGE_OFFSET",
            offset=0xFFFF,
            size=32
        )
        reg.fields = [
            RegisterField(name="BIT", bit_offset=0, bit_width=1, description="Bit field")
        ]
        peripheral.registers.append(reg)

        device = create_stm32_test_device()
        content = generate_register_struct(peripheral, device, family_level=False)

        # Should handle large offset
        self.assertIn("LARGE_OFFSET", content)


if __name__ == "__main__":
    unittest.main(verbosity=2)
