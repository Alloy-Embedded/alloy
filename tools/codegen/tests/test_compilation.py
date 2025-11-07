"""
Compilation tests for generated code

These tests actually compile the generated C++ code to ensure it's valid.
This is the most important test - if it doesn't compile, it's broken!
"""

import unittest
import tempfile
import subprocess
from pathlib import Path
import sys

# Add parent to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from cli.generators.generate_registers import generate_register_struct
from cli.generators.generate_enums import generate_enums_header
from cli.generators.generate_pin_functions import (
    generate_pin_functions_header,
    PinFunctionDatabase,
    PinFunction
)
from tests.test_helpers import (
    create_test_register,
    create_test_peripheral,
    create_test_device,
    create_test_field,
    create_pioa_test_peripheral,
    create_same70_test_device,
    AssertHelpers
)


class TestRegisterCompilation(unittest.TestCase):
    """Test that generated register code compiles"""

    def setUp(self):
        """Create temporary directory for test files"""
        self.temp_dir = tempfile.mkdtemp()

    def test_simple_register_compiles(self):
        """Test that simple register struct compiles"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG1", 0x0000, description="Test Register 1"),
                create_test_register("REG2", 0x0004, description="Test Register 2"),
            ]
        )
        device = create_test_device()

        # Generate register header
        header_code = generate_register_struct(peripheral, device)

        # Create test program that uses the generated code
        test_program = f"""
{header_code}

int main() {{
    // Test that we can create an instance
    alloy::hal::testvendor::testfamily::test_mcu::test::TEST_Registers regs;

    // Test that we can access members
    regs.REG1 = 0x12345678;
    regs.REG2 = 0xABCDEF00;

    // Test sizeof
    static_assert(sizeof(regs) >= 8, "Structure too small");

    return 0;
}}
"""

        # Try to compile
        AssertHelpers.assert_compiles(test_program)

    def test_register_array_compiles(self):
        """Test that register arrays compile correctly"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("ARRAY_REG", 0x0000, size=32, dim=4),
                create_test_register("SINGLE_REG", 0x0010),
            ]
        )
        device = create_test_device()

        header_code = generate_register_struct(peripheral, device)

        test_program = f"""
{header_code}

int main() {{
    alloy::hal::testvendor::testfamily::test_mcu::test::TEST_Registers regs;

    // Test array access
    for (int i = 0; i < 4; i++) {{
        regs.ARRAY_REG[i] = i;
    }}

    // Test single register
    regs.SINGLE_REG = 42;

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)

    def test_pioa_registers_compile(self):
        """Test that PIOA registers with ABCDSR[2] compile"""
        peripheral = create_pioa_test_peripheral()
        device = create_same70_test_device()

        header_code = generate_register_struct(peripheral, device)

        test_program = f"""
{header_code}

int main() {{
    using namespace alloy::hal::atmel::same70::atsame70q21b::pioa;

    PIOA_Registers* pioa = PIOA();

    // Test ABCDSR array access
    pioa->ABCDSR[0] = 0;
    pioa->ABCDSR[1] = 0;

    // Test OWER register
    pioa->OWER = 0xFF;

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)

    def test_different_register_sizes_compile(self):
        """Test that different register sizes compile correctly"""
        peripheral = create_test_peripheral(
            name="TEST",
            base_address=0x40000000,
            registers=[
                create_test_register("REG8", 0x0000, size=8),
                create_test_register("REG16", 0x0002, size=16),
                create_test_register("REG32", 0x0004, size=32),
            ]
        )
        device = create_test_device()

        header_code = generate_register_struct(peripheral, device)

        test_program = f"""
{header_code}
#include <cstdint>

int main() {{
    alloy::hal::testvendor::testfamily::test_mcu::test::TEST_Registers regs;

    // Test type sizes
    static_assert(sizeof(regs.REG8) == 1, "REG8 should be 1 byte");
    static_assert(sizeof(regs.REG16) == 2, "REG16 should be 2 bytes");
    static_assert(sizeof(regs.REG32) == 4, "REG32 should be 4 bytes");

    // Test assignment
    regs.REG8 = 0xFF;
    regs.REG16 = 0xFFFF;
    regs.REG32 = 0xFFFFFFFF;

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)


class TestEnumCompilation(unittest.TestCase):
    """Test that generated enum code compiles"""

    def test_simple_enum_compiles(self):
        """Test that simple enum class compiles"""
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
        device.peripherals = {"GPIO": peripheral}

        header_code = generate_enums_header(device)

        test_program = f"""
{header_code}

int main() {{
    using namespace alloy::hal::testvendor::testfamily::test_mcu::enums;

    // Test enum usage
    GPIO_CFGR_MODE mode = GPIO_CFGR_MODE::INPUT;
    mode = GPIO_CFGR_MODE::OUTPUT;

    // Test underlying type
    static_assert(sizeof(GPIO_CFGR_MODE) == sizeof(uint32_t),
                  "Enum should have uint32_t underlying type");

    // Test values
    uint32_t value = static_cast<uint32_t>(GPIO_CFGR_MODE::INPUT);

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)

    def test_multiple_enums_compile(self):
        """Test that multiple enum classes in same file compile"""
        field1 = create_test_field("MODE", 0, 2)
        field1.enum_values = {"INPUT": 0, "OUTPUT": 1}

        field2 = create_test_field("SPEED", 2, 2)
        field2.enum_values = {"LOW": 0, "HIGH": 1}

        register = create_test_register("CFGR", 0x0000, fields=[field1, field2])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        header_code = generate_enums_header(device)

        test_program = f"""
{header_code}

int main() {{
    using namespace alloy::hal::testvendor::testfamily::test_mcu::enums;

    GPIO_CFGR_MODE mode = GPIO_CFGR_MODE::OUTPUT;
    GPIO_CFGR_SPEED speed = GPIO_CFGR_SPEED::HIGH;

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)

    def test_enum_with_8bit_underlying_type_compiles(self):
        """Test that 8-bit enum compiles correctly"""
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"MODE0": 0, "MODE1": 1}

        register = create_test_register("CFGR", 0x0000, size=8, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        header_code = generate_enums_header(device)

        test_program = f"""
{header_code}

int main() {{
    using namespace alloy::hal::testvendor::testfamily::test_mcu::enums;

    GPIO_CFGR_MODE mode = GPIO_CFGR_MODE::MODE0;

    static_assert(sizeof(GPIO_CFGR_MODE) == sizeof(uint8_t),
                  "8-bit register should have uint8_t enum");

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)


class TestPinFunctionCompilation(unittest.TestCase):
    """Test that generated pin function code compiles"""

    @unittest.skip("Pin function generator requires pin constants to be defined first")
    def test_simple_pin_functions_compile(self):
        """Test that pin function template code compiles"""
        # This test is skipped because the pin function generator
        # uses pin constants (like PA9, PA10) that need to be defined
        # in a pins.hpp header first. This requires integration with
        # the full pin generation pipeline.
        db = PinFunctionDatabase("STM32F103C8", "st", "stm32f1")
        db.pin_functions = [
            PinFunction("PA9", "USART1", "TX", 7),
            PinFunction("PA10", "USART1", "RX", 7),
        ]

        device = create_test_device(
            name="STM32F103C8",
            vendor="ST",
            family="STM32F1"
        )

        header_code = generate_pin_functions_header(db, device)

        test_program = f"""
{header_code}
#include <type_traits>

int main() {{
    using namespace alloy::hal::st::stm32f1::stm32f103c8::pin_functions;

    // Template code should compile even if not used
    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)


class TestIntegratedCompilation(unittest.TestCase):
    """Test that multiple generated headers work together"""

    def test_registers_and_enums_together(self):
        """Test that register and enum headers can be used together"""
        # Create field with enum
        field = create_test_field("MODE", 0, 2)
        field.enum_values = {"INPUT": 0, "OUTPUT": 1}

        register = create_test_register("CFGR", 0x0000, fields=[field])
        peripheral = create_test_peripheral("GPIO", 0x40000000, registers=[register])
        device = create_test_device()
        device.peripherals = {"GPIO": peripheral}

        register_code = generate_register_struct(peripheral, device)
        enum_code = generate_enums_header(device)

        test_program = f"""
{enum_code}
{register_code}

int main() {{
    using namespace alloy::hal::testvendor::testfamily::test_mcu;

    // Get pointer to GPIO registers
    gpio::GPIO_Registers* gpio = gpio::GPIO();

    // Use enum to set mode
    gpio->CFGR = static_cast<uint32_t>(enums::GPIO_CFGR_MODE::OUTPUT);

    return 0;
}}
"""

        AssertHelpers.assert_compiles(test_program)


if __name__ == "__main__":
    unittest.main(verbosity=2)
