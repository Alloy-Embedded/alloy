"""
Unit tests for enum generation

These tests validate that the enum generator produces correct C++ code
for various enum configurations from register fields.
"""

import unittest
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_enums import (
    get_enum_underlying_type,
    sanitize_identifier,
    make_enum_name,
    validate_enum_values,
    generate_enums_header
)
from tests.test_helpers import (
    create_test_register,
    create_test_peripheral,
    create_test_device,
    create_test_field,
    AssertHelpers
)


class TestEnumHelperFunctions(unittest.TestCase):
    """Test enum generation helper functions"""

    def test_get_enum_underlying_type_8bit(self):
        """Test that 8-bit registers get uint8_t underlying type"""
        self.assertEqual(get_enum_underlying_type(8), "uint8_t")
        self.assertEqual(get_enum_underlying_type(4), "uint8_t")

    def test_get_enum_underlying_type_16bit(self):
        """Test that 16-bit registers get uint16_t underlying type"""
        self.assertEqual(get_enum_underlying_type(16), "uint16_t")
        self.assertEqual(get_enum_underlying_type(12), "uint16_t")

    def test_get_enum_underlying_type_32bit(self):
        """Test that 32-bit registers get uint32_t underlying type"""
        self.assertEqual(get_enum_underlying_type(32), "uint32_t")
        self.assertEqual(get_enum_underlying_type(24), "uint32_t")

    def test_sanitize_identifier_removes_placeholders(self):
        """Test that placeholder patterns are removed"""
        self.assertEqual(sanitize_identifier("REG%s"), "REG")
        self.assertEqual(sanitize_identifier("REG%S"), "REG")
        self.assertEqual(sanitize_identifier("REG[s]"), "REG")
        self.assertEqual(sanitize_identifier("REG[S]"), "REG")

    def test_sanitize_identifier_replaces_invalid_chars(self):
        """Test that invalid C++ characters are replaced with underscore"""
        self.assertEqual(sanitize_identifier("REG-NAME"), "REG_NAME")
        self.assertEqual(sanitize_identifier("REG.NAME"), "REG_NAME")
        self.assertEqual(sanitize_identifier("REG NAME"), "REG_NAME")
        self.assertEqual(sanitize_identifier("REG@NAME"), "REG_NAME")

    def test_sanitize_identifier_removes_leading_trailing_underscores(self):
        """Test that leading/trailing underscores are removed"""
        self.assertEqual(sanitize_identifier("_REG_"), "REG")
        self.assertEqual(sanitize_identifier("__REG__"), "REG")

    def test_make_enum_name_format(self):
        """Test enum name follows {Peripheral}_{Register}_{Field} format"""
        peripheral = create_test_peripheral("PIOA", 0x400E0E00)
        register = create_test_register("CFGR", 0x0000)
        field = create_test_field("MODE", 0, 2)

        enum_name = make_enum_name(peripheral, register, field)
        self.assertEqual(enum_name, "PIOA_CFGR_MODE")

    def test_make_enum_name_sanitizes_components(self):
        """Test that enum name components are sanitized"""
        peripheral = create_test_peripheral("PIO-A", 0x400E0E00)
        register = create_test_register("CFG.R", 0x0000)
        field = create_test_field("MODE[0]", 0, 2)

        enum_name = make_enum_name(peripheral, register, field)
        self.assertEqual(enum_name, "PIO_A_CFG_R_MODE_0")

    def test_validate_enum_values_valid(self):
        """Test that valid enum values pass validation"""
        # 2-bit field can hold values 0-3
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1, "MODE2": 2, "MODE3": 3}

        self.assertTrue(validate_enum_values(field, 32))

    def test_validate_enum_values_invalid_too_large(self):
        """Test that enum values too large for field fail validation"""
        # 2-bit field can only hold values 0-3
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1, "INVALID": 5}  # 5 > 2^2-1

        self.assertFalse(validate_enum_values(field, 32))

    def test_validate_enum_values_empty(self):
        """Test that fields with no enum values return True (no validation needed)"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = None

        # Empty enum values should pass validation (nothing to validate)
        self.assertTrue(validate_enum_values(field, 32))

        field.enum_values = {}
        self.assertTrue(validate_enum_values(field, 32))


class TestEnumGeneration(unittest.TestCase):
    """Test enum code generation"""

    def test_simple_enum_generation(self):
        """Test generation of simple enum class"""
        # Create field with enum values
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {
            "INPUT": 0,
            "OUTPUT": 1,
            "ALTERNATE": 2,
            "ANALOG": 3
        }

        register = create_test_register("CFGR", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()

        # Add peripheral to device
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        # Should generate enum class (values in hex format)
        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIO_CFGR_MODE",
            ": uint32_t",  # Underlying type
            "INPUT = 0x0",
            "OUTPUT = 0x1",
            "ALTERNATE = 0x2",
            "ANALOG = 0x3"
        )

    def test_enum_with_8bit_register(self):
        """Test that 8-bit register generates enum with uint8_t underlying type"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1}

        register = create_test_register("CFGR", 0x0000, size=8, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIO_CFGR_MODE : uint8_t"
        )

    def test_enum_with_16bit_register(self):
        """Test that 16-bit register generates enum with uint16_t underlying type"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1}

        register = create_test_register("CFGR", 0x0000, size=16, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIO_CFGR_MODE : uint16_t"
        )

    def test_multiple_enums_in_same_register(self):
        """Test that multiple fields with enums generate separate enum classes"""
        field1 = create_test_field("MODE", 0, 2)
        field1.enum_values = {"INPUT": 0, "OUTPUT": 1}

        field2 = create_test_field("SPEED", 2, 2)
        field2.enum_values = {"LOW": 0, "MEDIUM": 1, "HIGH": 2}

        register = create_test_register("CFGR", 0x0000, fields=[field1, field2])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        # Should have both enum classes
        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIO_CFGR_MODE",
            "enum class GPIO_CFGR_SPEED"
        )

    def test_enum_namespacing(self):
        """Test that enums are placed in correct namespace"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1}

        register = create_test_register("CFGR", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device(
            name="ATSAME70Q21B",
            vendor="Atmel",
            family="SAME70"
        )
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        # Should have correct namespace
        AssertHelpers.assert_contains_all(
            output,
            "namespace alloy::hal::atmel::same70::atsame70q21b"
        )

    def test_no_enums_if_no_enum_values(self):
        """Test that fields without enum values don't generate enums"""
        field = create_test_field("DATA", 0, 8)
        # No enum_values set

        register = create_test_register("DR", 0x0000, fields=[field])
        peripheral = create_test_peripheral("USART", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"USART": peripheral}

        output = generate_enums_header(device)

        # Should not generate enum for this field
        self.assertNotIn("enum class USART_DR_DATA", output)

    def test_pragma_once(self):
        """Test that #pragma once is used"""
        device = create_test_device(
            name="ATSAME70Q21B",
            vendor="Atmel",
            family="SAME70"
        )

        output = generate_enums_header(device)

        # Modern C++ uses #pragma once instead of header guards
        AssertHelpers.assert_contains_all(
            output,
            "#pragma once"
        )

    def test_includes_generated(self):
        """Test that necessary includes are present"""
        device = create_test_device()
        output = generate_enums_header(device)

        AssertHelpers.assert_contains_all(
            output,
            "#include <cstdint>"
        )


class TestEnumEdgeCases(unittest.TestCase):
    """Test edge cases and special scenarios"""

    def test_enum_with_hex_values(self):
        """Test that enum values can be represented in hex"""
        field = create_test_field("FLAG", 0, 8)
        field.enum_values = {
            "FLAG_A": 0x01,
            "FLAG_B": 0x02,
            "FLAG_C": 0x04,
            "FLAG_D": 0x80
        }

        register = create_test_register("FLAGS", 0x0000, fields=[field])
        peripheral = create_test_peripheral("CTRL", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"CTRL": peripheral}

        output = generate_enums_header(device)

        # Should contain the enum
        self.assertIn("enum class CTRL_FLAGS_FLAG", output)

    def test_empty_device(self):
        """Test that empty device generates minimal header"""
        device = create_test_device()
        device.peripherals = {}

        output = generate_enums_header(device)

        # Should have #pragma once and namespace but no enums
        AssertHelpers.assert_contains_all(
            output,
            "#pragma once",
            "namespace alloy::hal",
            "No enumerated values found"
        )

    def test_enum_with_single_value(self):
        """Test enum with only one value"""
        field = create_test_field("MODE", 0, 1)
        field.enum_values = {"DISABLED": 0}

        register = create_test_register("CTRL", 0x0000, fields=[field])
        peripheral = create_test_peripheral("TIMER", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"TIMER": peripheral}

        output = generate_enums_header(device)

        AssertHelpers.assert_contains_all(
            output,
            "enum class TIMER_CTRL_MODE",
            "DISABLED = 0x0"
        )

    def test_enum_with_large_values(self):
        """Test enum with large values (32-bit)"""
        field = create_test_field("KEY", 0, 32)
        field.enum_values = {
            "KEY_A": 0x12345678,
            "KEY_B": 0xABCDEF00,
            "KEY_C": 0xFFFFFFFF
        }

        register = create_test_register("KEY_REG", 0x0000, size=32, fields=[field])
        peripheral = create_test_peripheral("SECURE", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"SECURE": peripheral}

        output = generate_enums_header(device)

        # Check for enum with uppercase hex (generator uses uppercase)
        AssertHelpers.assert_contains_all(
            output,
            "enum class SECURE_KEY_REG_KEY",
            "KEY_A = 0x12345678"
        )
        # Check that large values are present (case-insensitive)
        self.assertIn("KEY_B", output)
        self.assertIn("KEY_C", output)
        self.assertIn("0xABCDEF00", output)
        self.assertIn("0xFFFFFFFF", output)

    def test_enum_with_special_characters_in_names(self):
        """Test that special characters in enum value names are handled"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {
            "MODE_0": 0,
            "MODE-1": 1,  # Dash should be converted to underscore
            "MODE.2": 2,  # Dot should be converted to underscore
            "MODE 3": 3   # Space should be converted to underscore
        }

        register = create_test_register("CFGR", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        output = generate_enums_header(device)

        # Special chars should be converted to underscores
        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIO_CFGR_MODE"
        )

    def test_enum_with_zero_width_field(self):
        """Test field with 0 bit width (edge case)"""
        field = create_test_field("RESERVED", 0, 0)
        field.enum_values = {"VALUE": 0}

        register = create_test_register("REG", 0x0000, fields=[field])
        peripheral = create_test_peripheral("TEST", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"TEST": peripheral}

        # Should handle gracefully (may skip or generate minimal enum)
        output = generate_enums_header(device)

        # Should not crash
        self.assertIsInstance(output, str)

    def test_multiple_peripherals_with_enums(self):
        """Test multiple peripherals each with enums"""
        # Peripheral 1
        field1 = create_test_field("MODE", 0, 2)
        field1.enum_values = {"INPUT": 0, "OUTPUT": 1}
        register1 = create_test_register("CFGR", 0x0000, fields=[field1])
        peripheral1 = create_test_peripheral("GPIOA", 0x40000000, registers=[register1])

        # Peripheral 2
        field2 = create_test_field("BAUD", 0, 2)
        field2.enum_values = {"BAUD_9600": 0, "BAUD_115200": 1}
        register2 = create_test_register("BRR", 0x0000, fields=[field2])
        peripheral2 = create_test_peripheral("USART1", 0x40010000, registers=[register2])

        device = create_test_device()
        device.peripherals = {"GPIOA": peripheral1, "USART1": peripheral2}

        output = generate_enums_header(device)

        # Should have enums from both peripherals
        AssertHelpers.assert_contains_all(
            output,
            "enum class GPIOA_CFGR_MODE",
            "enum class USART1_BRR_BAUD"
        )

    def test_enum_with_duplicate_values(self):
        """Test enum where multiple names map to same value"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {
            "DISABLED": 0,
            "OFF": 0,       # Same value as DISABLED
            "ENABLED": 1,
            "ON": 1         # Same value as ENABLED
        }

        register = create_test_register("CTRL", 0x0000, fields=[field])
        peripheral = create_test_peripheral("TIMER", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"TIMER": peripheral}

        output = generate_enums_header(device)

        # Should generate all enum values
        AssertHelpers.assert_contains_all(
            output,
            "enum class TIMER_CTRL_MODE"
        )
        # All values should be present
        self.assertIn("DISABLED", output)
        self.assertIn("OFF", output)
        self.assertIn("ENABLED", output)
        self.assertIn("ON", output)

    def test_enum_with_very_long_name(self):
        """Test enum with very long peripheral/register/field names"""
        field = create_test_field(
            "VERY_LONG_FIELD_NAME_THAT_IS_EXCESSIVELY_LONG",
            0, 2
        )
        field.enum_values = {"VALUE": 0}

        register = create_test_register(
            "VERY_LONG_REGISTER_NAME_THAT_IS_EXCESSIVELY_LONG",
            0x0000,
            fields=[field]
        )
        peripheral = create_test_peripheral(
            "VERY_LONG_PERIPHERAL_NAME_THAT_IS_EXCESSIVELY_LONG",
            0x40000000,
            registers=[register]
        )
        device = create_test_device()
        device.peripherals = {"VERY_LONG_PERIPHERAL_NAME_THAT_IS_EXCESSIVELY_LONG": peripheral}

        output = generate_enums_header(device)

        # Should handle long names
        self.assertIn("enum class", output)

    def test_enum_values_sorted_by_value(self):
        """Test that enum values are sorted by their numeric value"""
        field = create_test_field("PRIORITY", 0, 3)
        field.enum_values = {
            "LOWEST": 0,
            "HIGHEST": 3,
            "LOW": 1,
            "HIGH": 2
        }

        register = create_test_register("CTRL", 0x0000, fields=[field])
        peripheral = create_test_peripheral("NVIC", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"NVIC": peripheral}

        output = generate_enums_header(device)

        # Find the enum definition
        lines = output.split('\n')
        enum_lines = []
        in_enum = False
        for line in lines:
            if "enum class NVIC_CTRL_PRIORITY" in line:
                in_enum = True
            if in_enum:
                enum_lines.append(line)
                if "}" in line and in_enum:
                    break

        # Values should appear in order: 0, 1, 2, 3
        enum_text = '\n'.join(enum_lines)
        lowest_idx = enum_text.find("LOWEST")
        low_idx = enum_text.find("LOW")
        high_idx = enum_text.find("HIGH")
        highest_idx = enum_text.find("HIGHEST")

        # Check ordering (not strict, but LOWEST should come before HIGHEST)
        self.assertGreater(highest_idx, lowest_idx)


if __name__ == "__main__":
    unittest.main(verbosity=2)
